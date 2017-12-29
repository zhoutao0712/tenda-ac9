
#include <linux/init.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/list.h>
#include <linux/random.h>
#include <linux/jhash.h>
#include <linux/bitops.h>
#include <linux/skbuff.h>
#include <linux/inet.h>
#include <linux/slab.h>
#include <net/net_namespace.h>
#include <net/netns/generic.h>

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_srd.h>

MODULE_AUTHOR("zhoutao0712 <846207054@qq.com>");
MODULE_DESCRIPTION("Xtables: smart routing when dns resolve");
MODULE_LICENSE("GPL");
MODULE_ALIAS("xt_srd");

#define DNS_HASH_SIZE	2048
#define MAX_HOST_LEN	128
#define DNS_PORT	53

#define DNS_FLAGS_MASK		0x8000
#define DNS_MAX_LABELS		8			//QNAME中最大labels数
#define DNS_MIN_LABELS		2
#define MAX_DNS_REQUESTS	15			//最大query数

extern int iphash_find_add(struct net *net, const char *name, const __be32 ip);			//EXPORT_SYMBOL in xt_iphash.c
//------------------------------------------------------------------------------------------------------------------------------------------------------
struct srd_entry {
	struct list_head	list;
	char			host[MAX_HOST_LEN];
	unsigned int		host_len;
};

struct srd_table {
	struct list_head	list;
	char			name[XT_SRD_NAME_LEN];
	unsigned int		refcnt;
	struct list_head	hash[0];
};


struct srd_net {
	struct list_head	tables;
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry	*xt_srd;
#endif
};

struct skb_info {
	struct sk_buff *skb;
	struct iphdr *iph;
	union {
		struct tcphdr *tcph;
		struct udphdr *udph;
	}l4;

	char *data;
	char *tail;
	int dlen;
};

struct dns_packet_header {
	u_int16_t tr_id;
	u_int16_t flags;
	u_int16_t num_queries;
	u_int16_t num_answers;
	u_int16_t authority_rrs;
	u_int16_t additional_rrs;
} __attribute__((packed));

struct dns_packet_query {
	char host[MAX_HOST_LEN];
	u_int16_t type;
	u_int16_t class;
};

struct dns_packet_answer {
	u_int16_t host_offset;
	u_int16_t type;
	u_int16_t class;
//	u_int32_t ttl;
	u_int16_t data_len;
//	char cname[MAX_HOST_LEN];
	__be32 ip;
};

struct dns_packet_proto {
	struct dns_packet_header header;
	struct dns_packet_query query;
	struct dns_packet_answer answer;
};

//---------------------------------------------------------------------------------------------------------------------------------------------------------
static bool get_skb_info(const struct sk_buff *skb, struct skb_info *K)
{
	K->skb = (struct sk_buff *)skb;
	K->iph = ip_hdr(skb);
	if(K->iph == NULL) return false;

	switch(K->iph->protocol) {
	case IPPROTO_TCP:
		skb_set_transport_header((struct sk_buff *)skb, sizeof(struct iphdr));
		K->l4.tcph = tcp_hdr(skb);		//tcph = (struct tcphdr *) ((u8 *) iph + (iph->ihl << 2));
		if(K->l4.tcph == NULL) return false;
		K->data = (void *)(K->l4.tcph) + (K->l4.tcph->doff * 4);
		K->dlen = ntohs(K->iph->tot_len) - (K->data - (char *)(K->iph));
		K->tail = K->data + K->dlen;
/*
printk(KERN_WARNING "tcp: src=%pI4:%u dst=%pI4:%u data length=%d\n"
		, &K->iph->saddr, ntohs(K->l4.tcph->source), &K->iph->daddr, ntohs(K->l4.tcph->dest), K->dlen);
*/
		break;

	case IPPROTO_UDP:
		skb_set_transport_header((struct sk_buff *)skb, sizeof(struct iphdr));
		K->l4.udph = udp_hdr(skb);		//udph = (struct udphdr *) ((u8 *) iph + (iph->ihl << 2));
		if(K->l4.udph == NULL) return false;
		K->data = (void *)(K->l4.udph) + sizeof(struct udphdr);
		K->dlen = ntohs(K->l4.udph->len) - sizeof(struct udphdr);
		K->tail = K->data + K->dlen;
/*
printk(KERN_WARNING "udp: src=%pI4:%u dst=%pI4:%u data length=%d\n"
		, &K->iph->saddr, ntohs(K->l4.udph->source), &K->iph->daddr, ntohs(K->l4.udph->dest), K->dlen);
*/
		break;

	default:
		return false;
	}

	return true;
}

static int srd_net_id;
static inline struct srd_net *srd_pernet(struct net *net)
{
	return net_generic(net, srd_net_id);
}

static DEFINE_SPINLOCK(srd_lock);
static DEFINE_MUTEX(srd_mutex);

#ifdef CONFIG_PROC_FS
static const struct file_operations srd_mt_fops;
#endif

static u_int32_t hash_rnd __read_mostly;
static bool hash_rnd_inited __read_mostly;

static inline u_int32_t srd_hash(const char *str, u_int32_t size)		// DJB hash
{
	u_int32_t hash = 5381;
	while (*str) {
		hash += (hash << 5) + (*str++);
	}
	return (hash & (size - 1));
}

static struct srd_entry *
srd_entry_lookup(const struct srd_table *table, const char *host)
{
	struct srd_entry *e;
	u_int32_t h = srd_hash(host, DNS_HASH_SIZE);

	list_for_each_entry(e, &table->hash[h], list)
		if (memcmp(e->host, host, e->host_len + 1) == 0)
			return e;

	return NULL;
}

static void srd_entry_remove(struct srd_table *t, struct srd_entry *e)
{
	list_del(&e->list);
	kfree(e);
}

static struct srd_entry *
srd_entry_add(struct srd_table *t, const char *host, unsigned int host_len)
{
	struct srd_entry *e;

	e = kmalloc(sizeof(*e), GFP_ATOMIC);
	if (e == NULL)
		return NULL;

	memcpy(e->host, host, host_len + 1);
	e->host_len = host_len;
	list_add_tail(&e->list, &t->hash[srd_hash(host, DNS_HASH_SIZE)]);

	return e;
}

static struct srd_table *
srd_table_lookup(struct srd_net *srd_net, const char *name)
{
	struct srd_table *t;

	list_for_each_entry(t, &srd_net->tables, list)
		if (!strcmp(t->name, name))
			return t;
	return NULL;
}

static void srd_table_flush(struct srd_table *t)
{
	struct srd_entry *e, *next;
	unsigned int i;

	for (i = 0; i < DNS_HASH_SIZE; i++)
		list_for_each_entry_safe(e, next, &t->hash[i], list)
			srd_entry_remove(t, e);
}

static struct srd_entry *
srd_find_host(const struct srd_table *table, const char *host, const int label_count)
{
	const char *p, *q;
	int count = 0;

	if(host == NULL) return NULL;
	if(label_count < 1) return NULL;

	p = host;
	q = host + strlen(host) - 1;

//printk(KERN_WARNING "\n%s %d\n", __FUNCTION__, __LINE__);

	while(*q) {
		if(q < p) return NULL;
		if(*q == '.') count++;
		if((q == p)&&(label_count == count)) break;
		if(label_count + 1 == count) {
			q++;
			break;
		}
		q--;
	}

//printk(KERN_WARNING "\n%s %d q=%s\n", __FUNCTION__, __LINE__, q);

	return srd_entry_lookup(table, q);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static inline int is_host_char(char c)
{
	return (((c >='a')&&(c <= 'z'))||((c >='A')&&(c <= 'Z'))||((c >='0')&&(c <= '9'))||(c == '-'));
}

//检查p和q之间的字符(不包含p和q)是否是域名字符
static int check_domain_char(char *p, char *q)
{
	if(p >= q) return -1;
	while(1) {
		++p;
		if(p == q) break;
		if(!is_host_char(*p)) return -2;
	}

	return 0;
}

static bool get_dns_query(struct skb_info *K, struct dns_packet_proto *proto, int *offset)
{
	struct dns_packet_query *Q = &(proto->query);
	char *p, *q;

	int off_labels[DNS_MAX_LABELS] = {0};			//labels相对偏移
	u_int8_t len_labels;
	int i = 0, label_count, host_len;

	p = K->data + *offset;
	while(1) {
		off_labels[i] = (int)(p - (K->data + *offset));
		if(p[0] == '\0') break;
		len_labels = (u_int8_t)p[0];
//printk(KERN_WARNING "%d %s\n", len_labels, p + 1);
		if((len_labels < 0)||(len_labels > 63)) return false;			// 0~63

		q = p + len_labels + 1;
		if(q >= K->tail) return false;
//printk(KERN_WARNING "check_domain_char=%d\n", check_domain_char(p, q));
		if(check_domain_char(p, q) != 0) return false;

		p = q;
		i++;
		if(i > DNS_MAX_LABELS) return false;
	}

	label_count = i;
	host_len = off_labels[i];

	if(i < DNS_MIN_LABELS) return false;		//label min count
	if(host_len > MAX_HOST_LEN) return false;

	p = Q->host;
	memcpy(p, (K->data + *offset + 1), host_len);	//完成memcpy后host已经有0结尾，无须再设置
	for(i = 1; i <= label_count; i++) {
		if(p[off_labels[i] - 1] == '\0') break;
		p[off_labels[i] - 1] = '.';
	}

	p = q + 1;
	Q->type = *(u_int16_t *)p;
	Q->type = ntohs(Q->type);

	p = p + 2;
	Q->class = *(u_int16_t *)p;
	Q->class = ntohs(Q->class);

//printk(KERN_WARNING "%s %d: host=%s type=%d class=%d label_count=%d host_len=%d\n", __FUNCTION__, __LINE__, Q->host, Q->type, Q->class, i, host_len);

	if(((Q->type != 0x0001)&&(Q->type != 0x0005))||(Q->class != 0x0001)) return false;

	p = p + 2;

	*offset = p - K->data;

	return true;
}

static bool get_dns_answer(struct net *net, const char *name, struct skb_info *K, struct dns_packet_proto *proto, int *offset)
{
	char *p, *q;
	struct dns_packet_answer *Q = &(proto->answer);

	p = K->data + *offset;

	Q->host_offset = *(u_int16_t *) p;
	Q->host_offset = ntohs(Q->host_offset);
	if((Q->host_offset & 0xC000) != 0XC000) return false;

	p = p + 2;
	Q->type = *(u_int16_t *) p;
	Q->type = ntohs(Q->type);

	p = p + 2;
	Q->class = *(u_int16_t *) p;
	Q->class = ntohs(Q->class);
	if(((Q->type != 0x0001)&&(Q->type != 0x0005))||(Q->class != 0x0001)) return false;

	p = p + 2;
/*
don't care ttl
	Q->ttl = *(u_int32_t *) p;
	Q->ttl = ntohl(Q->ttl);
*/

	p = p + 4;
	Q->data_len = *(u_int16_t *) p;
	Q->data_len = ntohs(Q->data_len);

//printk(KERN_WARNING "%s %d: type=%u class=%u data_len=%u\n", __FUNCTION__, __LINE__, Q->type, Q->class, Q->data_len);

	p = p + 2;
	*offset = p - K->data + Q->data_len;
	if(*offset > K->dlen) return false;

	if(Q->type == 0x0001) {			// type A
		if(Q->data_len != 4) return false;
		Q->ip = *(__be32 *)p;
//printk(KERN_WARNING "-------------------------------------------------%s %d: host=%s ip=%pI4\n", __FUNCTION__, __LINE__, proto->query.host, &Q->ip);
		iphash_find_add(net, name, Q->ip);
		return true;
	} else if(Q->type == 0x0005) {		// type cname
		return true;				//don't care cname
	} else {
		return false;
	}

	return true;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

static bool
srd_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	struct net *net = dev_net(par->in ? par->in : par->out);
	struct srd_net *srd_net = srd_pernet(net);
	const struct xt_srd_mtinfo *info = par->matchinfo;
	struct srd_table *t;
	struct srd_entry *e;

	struct skb_info K;
	struct dns_packet_proto proto;
	u_int16_t num;
	int offset_x = 0;

/*
printk(KERN_WARNING "\n%s %d: %d %d %d %d\n\n"
	, __FUNCTION__, __LINE__, sizeof(struct dns_packet_header), sizeof(struct dns_packet_query), sizeof(struct dns_packet_answer), sizeof(struct dns_packet_proto));
*/

	if(!get_skb_info(skb, &K)) return false;
	if(K.iph->protocol != IPPROTO_UDP) return false;
	if((ntohs(K.l4.udph->source) != DNS_PORT)&&(ntohs(K.l4.udph->dest) != DNS_PORT)) return false;
	if(K.dlen < sizeof(struct dns_packet_header)) return false;

// code from nDPI
	memcpy(&(proto.header), (struct dns_packet_header *)(K.data + offset_x), sizeof(struct dns_packet_header));
	proto.header.tr_id			= ntohs(proto.header.tr_id);
	proto.header.flags			= ntohs(proto.header.flags);
	proto.header.num_queries		= ntohs(proto.header.num_queries);
	proto.header.num_answers		= ntohs(proto.header.num_answers);
	proto.header.authority_rrs		= ntohs(proto.header.authority_rrs);
	proto.header.additional_rrs		= ntohs(proto.header.additional_rrs);

	offset_x += sizeof(struct dns_packet_header);
	if(ntohs(K.l4.udph->dest) == DNS_PORT) {
//printk(KERN_WARNING "\n%s %d: num_queries=%d num_answers=%d\n", __FUNCTION__, __LINE__, proto.header.num_queries, proto.header.num_answers);
		if(proto.header.flags != 0x0100) return false;		//only startdard request
		if(proto.header.num_queries != 1) return false;		// only 1 query
		if((proto.header.authority_rrs != 0)||(proto.header.additional_rrs != 0)) return false;

		if(!get_dns_query(&K, &proto, &offset_x)) return false;
//printk(KERN_WARNING "%s %d: host=%s type=%u class=%u\n", __FUNCTION__, __LINE__, proto.query.host, proto.query.type, proto.query.class);
		if(proto.query.class != 0x0001) return false;

		spin_lock_bh(&srd_lock);
		t = srd_table_lookup(srd_net, info->name);
		e = srd_find_host(t, proto.query.host, 1);			// x.com
		if(e == NULL) e = srd_find_host(t, proto.query.host, 2);	// y.x.com
		if(e == NULL) e = srd_entry_lookup(t, proto.query.host);		// whole host
		spin_unlock_bh(&srd_lock);

		if(e == NULL) return false;

	} else if(ntohs(K.l4.udph->source) == DNS_PORT) {
//printk(KERN_WARNING "\n%s %d: num_queries=%d num_answers=%d\n", __FUNCTION__, __LINE__, proto.header.num_queries, proto.header.num_answers);
		if(proto.header.flags != 0x8180) return false;		//only startdard response
		if(proto.header.num_queries != 1) return false;		// only 1 query
		if((proto.header.num_answers > 0) && (proto.header.num_answers <= MAX_DNS_REQUESTS)) {

			if(!get_dns_query(&K, &proto, &offset_x)) return false;
//printk(KERN_WARNING "%s %d: host=%s type=%u class=%u\n", __FUNCTION__, __LINE__, proto.query.host, proto.query.type, proto.query.class);
			if(proto.query.class != 0x0001) return false;

			spin_lock_bh(&srd_lock);
			t = srd_table_lookup(srd_net, info->name);
			e = srd_find_host(t, proto.query.host, 1);			// x.com
			if(e == NULL) e = srd_find_host(t, proto.query.host, 2);	// y.x.com
			if(e == NULL) e = srd_entry_lookup(t, proto.query.host);		// whole host
			spin_unlock_bh(&srd_lock);

			if(e == NULL) return false;

			for(num = 0; num < proto.header.num_answers; num++) {
				if(!get_dns_answer(net, info->name, &K, &proto, &offset_x)) return false;
			}
		}
	} else {
		printk(KERN_WARNING "something wrong\n");
		return false;
	}

	return true;
}

static int srd_mt_check(const struct xt_mtchk_param *par)
{
	struct srd_net *srd_net = srd_pernet(par->net);
	const struct xt_srd_mtinfo *info = par->matchinfo;
	struct srd_table *t;
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *pde;
#endif
	unsigned i;
	int ret = -EINVAL;

	if (unlikely(!hash_rnd_inited)) {
		get_random_bytes(&hash_rnd, sizeof(hash_rnd));
		hash_rnd_inited = true;
	}

	if (info->name[0] == '\0' || strnlen(info->name, XT_SRD_NAME_LEN) == XT_SRD_NAME_LEN)
		return -EINVAL;

	mutex_lock(&srd_mutex);
	t = srd_table_lookup(srd_net, info->name);
	if (t != NULL) {
		t->refcnt++;
		ret = 0;
		goto out;
	}

	t = kzalloc(sizeof(*t) + sizeof(t->hash[0]) * DNS_HASH_SIZE, GFP_KERNEL);
	if (t == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	t->refcnt = 1;
	strcpy(t->name, info->name);
	for (i = 0; i < DNS_HASH_SIZE; i++)
		INIT_LIST_HEAD(&t->hash[i]);
#ifdef CONFIG_PROC_FS
	pde = proc_create_data(t->name, 0644, srd_net->xt_srd, &srd_mt_fops, t);
	if (pde == NULL) {
		kfree(t);
		ret = -ENOMEM;
		goto out;
	}
#endif
	spin_lock_bh(&srd_lock);
	list_add_tail(&t->list, &srd_net->tables);
	spin_unlock_bh(&srd_lock);
	ret = 0;
out:
	mutex_unlock(&srd_mutex);
	return ret;
}

static void srd_mt_destroy(const struct xt_mtdtor_param *par)
{
	struct srd_net *srd_net = srd_pernet(par->net);
	const struct xt_srd_mtinfo *info = par->matchinfo;
	struct srd_table *t;

	mutex_lock(&srd_mutex);
	t = srd_table_lookup(srd_net, info->name);
	if (--t->refcnt == 0) {
		spin_lock_bh(&srd_lock);
		list_del(&t->list);
		spin_unlock_bh(&srd_lock);
#ifdef CONFIG_PROC_FS
		remove_proc_entry(t->name, srd_net->xt_srd);
#endif
		srd_table_flush(t);
		kfree(t);
	}
	mutex_unlock(&srd_mutex);
}

#ifdef CONFIG_PROC_FS
struct srd_iter_state {
	const struct srd_table *table;
	unsigned int		bucket;
};

static void *srd_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(srd_lock)
{
	struct srd_iter_state *st = seq->private;
	const struct srd_table *t = st->table;
	struct srd_entry *e;
	loff_t p = *pos;

	spin_lock_bh(&srd_lock);

	for (st->bucket = 0; st->bucket < DNS_HASH_SIZE; st->bucket++)
		list_for_each_entry(e, &t->hash[st->bucket], list)
			if (p-- == 0)
				return e;
	return NULL;
}

static void *srd_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct srd_iter_state *st = seq->private;
	const struct srd_table *t = st->table;
	const struct srd_entry *e = v;
	const struct list_head *head = e->list.next;

	while (head == &t->hash[st->bucket]) {
		if (++st->bucket >= DNS_HASH_SIZE)
			return NULL;
		head = t->hash[st->bucket].next;
	}
	(*pos)++;
	return list_entry(head, struct srd_entry, list);
}

static void srd_seq_stop(struct seq_file *s, void *v)
	__releases(srd_lock)
{
	spin_unlock_bh(&srd_lock);
}

static int srd_seq_show(struct seq_file *seq, void *v)
{
	const struct srd_entry *e = v;

	seq_printf(seq, "host=%s host_len=%u\n", e->host, e->host_len);

	return 0;
}

static const struct seq_operations srd_seq_ops = {
	.start		= srd_seq_start,
	.next		= srd_seq_next,
	.stop		= srd_seq_stop,
	.show		= srd_seq_show,
};

static int srd_seq_open(struct inode *inode, struct file *file)
{
	struct proc_dir_entry *pde = PDE(inode);
	struct srd_iter_state *st;

	st = __seq_open_private(file, &srd_seq_ops, sizeof(*st));
	if (st == NULL)
		return -ENOMEM;

	st->table    = pde->data;
	return 0;
}

static ssize_t
srd_mt_proc_write(struct file *file, const char __user *input,
		     size_t size, loff_t *loff)
{
	const struct proc_dir_entry *pde = PDE(file->f_path.dentry->d_inode);
	struct srd_table *t = pde->data;
	struct srd_entry *e;
	char buf[MAX_HOST_LEN];
	const char *c = buf;
	char *host;
	unsigned int host_len;
	bool add;


	if (size == 0)
		return 0;
	if (size > sizeof(buf) - 1) {
//printk("size too long, size=%u\n", size);
		return -EFAULT;
	}
	if (copy_from_user(buf, input, size) != 0)
		return -EFAULT;

	buf[size] = 0;

//printk("input buf=%s\n", buf);

	/* Strict protocol! */
	if (*loff != 0)
		return -ESPIPE;

	switch (*c) {
	case '/': /* flush table */
		spin_lock_bh(&srd_lock);
		srd_table_flush(t);
		spin_unlock_bh(&srd_lock);
		return size;
	case '-': /* remove host */
		add = false;
		break;
	case '+': /* add host */
		add = true;
		break;
	default:
		pr_info("Need \"+host\", \"-host\" or \"/\"\n");
		return -EINVAL;
	}

	++c;
	--size;

	host = (char *)c;
	host_len = 0;
	while(*host) {
		++host_len;
		++host;
		if((*host == '\r')||(*host == '\n')) {
			*host = 0;
			break;
		}
	}
	host = (char *)c;
//printk("host_len=%u host=%s\n", host_len, host);

	spin_lock_bh(&srd_lock);
	e = srd_entry_lookup(t, host);
	if (e == NULL) {
		if (add)
			srd_entry_add(t, host, host_len);
	} else {
		if (!add)
			srd_entry_remove(t, e);
	}
	spin_unlock_bh(&srd_lock);
	/* Note we removed one above */
	*loff += size + 1;
	return size + 1;
}

static const struct file_operations srd_mt_fops = {
	.open    = srd_seq_open,
	.read    = seq_read,
	.write   = srd_mt_proc_write,
	.release = seq_release_private,
	.owner   = THIS_MODULE,
};

static int __net_init srd_proc_net_init(struct net *net)
{
	struct srd_net *srd_net = srd_pernet(net);

	srd_net->xt_srd = proc_mkdir("xt_srd", net->proc_net);
	if (!srd_net->xt_srd)
		return -ENOMEM;
	return 0;
}

static void __net_exit srd_proc_net_exit(struct net *net)
{
	proc_net_remove(net, "xt_srd");
}
#else
static inline int srd_proc_net_init(struct net *net)
{
	return 0;
}

static inline void srd_proc_net_exit(struct net *net)
{
}
#endif /* CONFIG_PROC_FS */

static int __net_init srd_net_init(struct net *net)
{
	struct srd_net *srd_net = srd_pernet(net);

	INIT_LIST_HEAD(&srd_net->tables);
	return srd_proc_net_init(net);
}

static void __net_exit srd_net_exit(struct net *net)
{
	struct srd_net *srd_net = srd_pernet(net);

	BUG_ON(!list_empty(&srd_net->tables));
	srd_proc_net_exit(net);
}

static struct pernet_operations srd_net_ops = {
	.init	= srd_net_init,
	.exit	= srd_net_exit,
	.id	= &srd_net_id,
	.size	= sizeof(struct srd_net),
};

static struct xt_match srd_mt_reg[] __read_mostly = {
	{
		.name       = "srd",
		.revision   = 0,
		.family     = NFPROTO_IPV4,
		.match      = srd_mt,
		.matchsize  = sizeof(struct xt_srd_mtinfo),
		.checkentry = srd_mt_check,
		.destroy    = srd_mt_destroy,
		.me         = THIS_MODULE,
	},
};

static int __init srd_mt_init(void)
{
	int err;

	err = register_pernet_subsys(&srd_net_ops);
	if (err)
		return err;
	err = xt_register_matches(srd_mt_reg, ARRAY_SIZE(srd_mt_reg));
	if (err)
		unregister_pernet_subsys(&srd_net_ops);
	return err;
}

static void __exit srd_mt_exit(void)
{
	xt_unregister_matches(srd_mt_reg, ARRAY_SIZE(srd_mt_reg));
	unregister_pernet_subsys(&srd_net_ops);
}

module_init(srd_mt_init);
module_exit(srd_mt_exit);

