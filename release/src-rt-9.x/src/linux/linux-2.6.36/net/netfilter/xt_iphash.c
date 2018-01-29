
#include <linux/init.h>
#include <linux/ip.h>
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
#include <linux/netfilter/xt_iphash.h>

MODULE_AUTHOR("zhoutao0712 <846207054@qq.com>");
MODULE_DESCRIPTION("Xtables: hashtable for ip list matching");
MODULE_LICENSE("GPL");
MODULE_ALIAS("xt_iphash");

#define IP_HASH_SIZE	4096

struct iphash_entry {
	struct list_head	list;
	union nf_inet_addr	addr;
};

struct iphash_table {
	struct list_head	list;
	char			name[XT_IPHASH_NAME_LEN];
	unsigned int		refcnt;
	struct list_head	hash[0];
};

struct iphash_net {
	struct list_head	tables;
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry	*xt_iphash;
#endif
};

static int iphash_net_id;
static inline struct iphash_net *iphash_pernet(struct net *net)
{
	return net_generic(net, iphash_net_id);
}

static DEFINE_SPINLOCK(iphash_lock);
static DEFINE_MUTEX(iphash_mutex);

#ifdef CONFIG_PROC_FS
static const struct file_operations iphash_mt_fops;
#endif

static u_int32_t hash_rnd __read_mostly;
static bool hash_rnd_inited __read_mostly;

static inline unsigned int iphash_entry_hash4(const union nf_inet_addr *addr)
{
	return jhash_1word((__force u32)addr->ip, hash_rnd) & (IP_HASH_SIZE - 1);
}

static struct iphash_entry *
iphash_entry_lookup(const struct iphash_table *table, const union nf_inet_addr *addrp)
{
	struct iphash_entry *e;
	unsigned int h = iphash_entry_hash4(addrp);

	list_for_each_entry(e, &table->hash[h], list)
		if (memcmp(&e->addr, addrp, sizeof(e->addr)) == 0)
			return e;

	return NULL;
}

static void iphash_entry_remove(struct iphash_table *t, struct iphash_entry *e)
{
	list_del(&e->list);
	kfree(e);
}

static struct iphash_entry *
iphash_entry_add(struct iphash_table *t, const union nf_inet_addr *addr)
{
	struct iphash_entry *e;

	e = kmalloc(sizeof(*e), GFP_ATOMIC);
	if (e == NULL)
		return NULL;

	memcpy(&e->addr, addr, sizeof(e->addr));
	list_add_tail(&e->list, &t->hash[iphash_entry_hash4(addr)]);

	return e;
}

static struct iphash_table *
iphash_table_lookup(struct iphash_net *iphash_net, const char *name)
{
	struct iphash_table *t;

	list_for_each_entry(t, &iphash_net->tables, list)
		if (!strcmp(t->name, name))
			return t;
	return NULL;
}

static void iphash_table_flush(struct iphash_table *t)
{
	struct iphash_entry *e, *next;
	unsigned int i;

	for (i = 0; i < IP_HASH_SIZE; i++)
		list_for_each_entry_safe(e, next, &t->hash[i], list)
			iphash_entry_remove(t, e);
}

int iphash_find_add(struct net *net, const char *name, const __be32 ip)
{
	struct iphash_net *iphash_net;
	struct iphash_table *t;
	struct iphash_entry *e;
	union nf_inet_addr addr = {};

	addr.ip = ip;
	iphash_net = net_generic(net, iphash_net_id);
	e = NULL;

	spin_lock_bh(&iphash_lock);
	t = iphash_table_lookup(iphash_net, name);
	if(t != NULL) e = iphash_entry_lookup(t, &addr);
	spin_unlock_bh(&iphash_lock);

	if(t == NULL) return -1;

	if(e == NULL) e = iphash_entry_add(t, &addr);

	if(e == NULL) return -2;

	return 0;
}
EXPORT_SYMBOL(iphash_find_add);

static bool
iphash_mt(const struct sk_buff *skb, struct xt_action_param *par)
{
	struct net *net = dev_net(par->in ? par->in : par->out);
	struct iphash_net *iphash_net = iphash_pernet(net);
	const struct xt_iphash_mtinfo *info = par->matchinfo;
	struct iphash_table *t;
	struct iphash_entry *e;
	union nf_inet_addr addr = {};
	bool ret = info->invert;

	const struct iphdr *iph = ip_hdr(skb);
	if (info->side == XT_IPHASH_DEST)
		addr.ip = iph->daddr;
	else
		addr.ip = iph->saddr;

	spin_lock_bh(&iphash_lock);
	t = iphash_table_lookup(iphash_net, info->name);
	e = iphash_entry_lookup(t, &addr);
	if (e != NULL) ret = !ret;
	spin_unlock_bh(&iphash_lock);

	return ret;
}

static int iphash_mt_check(const struct xt_mtchk_param *par)
{
	struct iphash_net *iphash_net = iphash_pernet(par->net);
	const struct xt_iphash_mtinfo *info = par->matchinfo;
	struct iphash_table *t;
#ifdef CONFIG_PROC_FS
	struct proc_dir_entry *pde;
#endif
	unsigned i;
	int ret = -EINVAL;

	if (unlikely(!hash_rnd_inited)) {
		get_random_bytes(&hash_rnd, sizeof(hash_rnd));
		hash_rnd_inited = true;
	}

	if (info->name[0] == '\0' || strnlen(info->name, XT_IPHASH_NAME_LEN) == XT_IPHASH_NAME_LEN)
		return -EINVAL;

	mutex_lock(&iphash_mutex);
	t = iphash_table_lookup(iphash_net, info->name);
	if (t != NULL) {
		t->refcnt++;
		ret = 0;
		goto out;
	}

	t = kzalloc(sizeof(*t) + sizeof(t->hash[0]) * IP_HASH_SIZE, GFP_KERNEL);
	if (t == NULL) {
		ret = -ENOMEM;
		goto out;
	}
	t->refcnt = 2;
	strcpy(t->name, info->name);
	for (i = 0; i < IP_HASH_SIZE; i++)
		INIT_LIST_HEAD(&t->hash[i]);
#ifdef CONFIG_PROC_FS
	pde = proc_create_data(t->name, 0644, iphash_net->xt_iphash, &iphash_mt_fops, t);
	if (pde == NULL) {
		kfree(t);
		ret = -ENOMEM;
		goto out;
	}
#endif
	spin_lock_bh(&iphash_lock);
	list_add_tail(&t->list, &iphash_net->tables);
	spin_unlock_bh(&iphash_lock);
	ret = 0;
out:
	mutex_unlock(&iphash_mutex);
	return ret;
}

static void iphash_mt_destroy(const struct xt_mtdtor_param *par)
{
	struct iphash_net *iphash_net = iphash_pernet(par->net);
	const struct xt_iphash_mtinfo *info = par->matchinfo;
	struct iphash_table *t;

	mutex_lock(&iphash_mutex);
	t = iphash_table_lookup(iphash_net, info->name);
	if (--t->refcnt == 0) {
		spin_lock_bh(&iphash_lock);
		list_del(&t->list);
		spin_unlock_bh(&iphash_lock);
#ifdef CONFIG_PROC_FS
		remove_proc_entry(t->name, iphash_net->xt_iphash);
#endif
		iphash_table_flush(t);
		kfree(t);
	}
	mutex_unlock(&iphash_mutex);
}

#ifdef CONFIG_PROC_FS
struct iphash_iter_state {
	const struct iphash_table *table;
	unsigned int		bucket;
};

static void *iphash_seq_start(struct seq_file *seq, loff_t *pos)
	__acquires(iphash_lock)
{
	struct iphash_iter_state *st = seq->private;
	const struct iphash_table *t = st->table;
	struct iphash_entry *e;
	loff_t p = *pos;

	spin_lock_bh(&iphash_lock);

	for (st->bucket = 0; st->bucket < IP_HASH_SIZE; st->bucket++)
		list_for_each_entry(e, &t->hash[st->bucket], list)
			if (p-- == 0)
				return e;
	return NULL;
}

static void *iphash_seq_next(struct seq_file *seq, void *v, loff_t *pos)
{
	struct iphash_iter_state *st = seq->private;
	const struct iphash_table *t = st->table;
	const struct iphash_entry *e = v;
	const struct list_head *head = e->list.next;

	while (head == &t->hash[st->bucket]) {
		if (++st->bucket >= IP_HASH_SIZE)
			return NULL;
		head = t->hash[st->bucket].next;
	}
	(*pos)++;
	return list_entry(head, struct iphash_entry, list);
}

static void iphash_seq_stop(struct seq_file *s, void *v)
	__releases(iphash_lock)
{
	spin_unlock_bh(&iphash_lock);
}

static int iphash_seq_show(struct seq_file *seq, void *v)
{
	const struct iphash_entry *e = v;

	seq_printf(seq, "ip=%pI4\n",&e->addr.ip);

	return 0;
}

static const struct seq_operations iphash_seq_ops = {
	.start		= iphash_seq_start,
	.next		= iphash_seq_next,
	.stop		= iphash_seq_stop,
	.show		= iphash_seq_show,
};

static int iphash_seq_open(struct inode *inode, struct file *file)
{
	struct proc_dir_entry *pde = PDE(inode);
	struct iphash_iter_state *st;

	st = __seq_open_private(file, &iphash_seq_ops, sizeof(*st));
	if (st == NULL)
		return -ENOMEM;

	st->table    = pde->data;
	return 0;
}

static ssize_t
iphash_mt_proc_write(struct file *file, const char __user *input,
		     size_t size, loff_t *loff)
{
	const struct proc_dir_entry *pde = PDE(file->f_path.dentry->d_inode);
	struct iphash_table *t = pde->data;
	struct iphash_entry *e;
	char buf[sizeof("+b335:1d35:1e55:dead:c0de:1715:5afe:c0de")];
	const char *c = buf;
	union nf_inet_addr addr = {};
	bool add, succ;

	if (size == 0)
		return 0;
	if (size > sizeof(buf))
		size = sizeof(buf);
	if (copy_from_user(buf, input, size) != 0)
		return -EFAULT;

	/* Strict protocol! */
	if (*loff != 0)
		return -ESPIPE;

	switch (*c) {
	case '/': /* flush table */
		spin_lock_bh(&iphash_lock);
		iphash_table_flush(t);
		spin_unlock_bh(&iphash_lock);
		return size;
	case '-': /* remove address */
		add = false;
		break;
	case '+': /* add address */
		add = true;
		break;
	default:
		pr_info("Need \"+ip\", \"-ip\" or \"/\"\n");
		return -EINVAL;
	}

	++c;
	--size;
	succ   = in4_pton(c, size, (void *)&addr, '\n', NULL);

	if (!succ) {
		pr_info("illegal address written to procfs\n");
		return -EINVAL;
	}

	spin_lock_bh(&iphash_lock);
	e = iphash_entry_lookup(t, &addr);
	if (e == NULL) {
		if (add)
			iphash_entry_add(t, &addr);
	} else {
		if (!add)
			iphash_entry_remove(t, e);
	}
	spin_unlock_bh(&iphash_lock);
	/* Note we removed one above */
	*loff += size + 1;
	return size + 1;
}

static const struct file_operations iphash_mt_fops = {
	.open    = iphash_seq_open,
	.read    = seq_read,
	.write   = iphash_mt_proc_write,
	.release = seq_release_private,
	.owner   = THIS_MODULE,
};

static int __net_init iphash_proc_net_init(struct net *net)
{
	struct iphash_net *iphash_net = iphash_pernet(net);

	iphash_net->xt_iphash = proc_mkdir("xt_iphash", net->proc_net);
	if (!iphash_net->xt_iphash)
		return -ENOMEM;
	return 0;
}

static void __net_exit iphash_proc_net_exit(struct net *net)
{
	proc_net_remove(net, "xt_iphash");
}
#else
static inline int iphash_proc_net_init(struct net *net)
{
	return 0;
}

static inline void iphash_proc_net_exit(struct net *net)
{
}
#endif /* CONFIG_PROC_FS */

static int __net_init iphash_net_init(struct net *net)
{
	struct iphash_net *iphash_net = iphash_pernet(net);

	INIT_LIST_HEAD(&iphash_net->tables);
	return iphash_proc_net_init(net);
}

static void __net_exit iphash_net_exit(struct net *net)
{
	struct iphash_net *iphash_net = iphash_pernet(net);

	BUG_ON(!list_empty(&iphash_net->tables));
	iphash_proc_net_exit(net);
}

static struct pernet_operations iphash_net_ops = {
	.init	= iphash_net_init,
	.exit	= iphash_net_exit,
	.id	= &iphash_net_id,
	.size	= sizeof(struct iphash_net),
};

static struct xt_match iphash_mt_reg[] __read_mostly = {
	{
		.name       = "iphash",
		.revision   = 0,
		.family     = NFPROTO_IPV4,
		.match      = iphash_mt,
		.matchsize  = sizeof(struct xt_iphash_mtinfo),
		.checkentry = iphash_mt_check,
		.destroy    = iphash_mt_destroy,
		.me         = THIS_MODULE,
	},
};

static int __init iphash_mt_init(void)
{
	int err;

	err = register_pernet_subsys(&iphash_net_ops);
	if (err)
		return err;
	err = xt_register_matches(iphash_mt_reg, ARRAY_SIZE(iphash_mt_reg));
	if (err)
		unregister_pernet_subsys(&iphash_net_ops);
	return err;
}

static void __exit iphash_mt_exit(void)
{
	xt_unregister_matches(iphash_mt_reg, ARRAY_SIZE(iphash_mt_reg));
	unregister_pernet_subsys(&iphash_net_ops);
}

module_init(iphash_mt_init);
module_exit(iphash_mt_exit);

