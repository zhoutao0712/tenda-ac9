#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <xtables.h>
#include <linux/netfilter/xt_iphash.h>

enum {
	O_SET = 0,
	O_RCHECK,
	O_NAME,
	O_RSOURCE,
	O_RDEST,
	F_SET    = 1 << O_SET,
	F_RCHECK = 1 << O_RCHECK,
	F_ANY_OP = F_SET | F_RCHECK,
};

#define s struct xt_iphash_mtinfo
static const struct xt_option_entry iphash_opts[] = {
	{.name = "rcheck", .id = O_RCHECK, .type = XTTYPE_NONE,
	 .excl = F_ANY_OP, .flags = XTOPT_INVERT},
	{.name = "name", .id = O_NAME, .type = XTTYPE_STRING,
	 .flags = XTOPT_PUT, XTOPT_POINTER(s, name)},
	{.name = "rsource", .id = O_RSOURCE, .type = XTTYPE_NONE},
	{.name = "rdest", .id = O_RDEST, .type = XTTYPE_NONE},
	XTOPT_TABLEEND,
};
#undef s

static void iphash_help(void)
{
	printf(
"iphash match options:\n"
"[!] --rcheck                    Match if source address in list.\n"
"    --name name                 Name of the iphash list to be used.  DEFAULT used if none given.\n"
"    --rsource                   Match/Save the source address of each packet in the iphash list table (default).\n"
"    --rdest                     Match/Save the destination address of each packet in the iphash list table.\n"
"xt_iphash by: zhoutao0712 <846207054@qq.com>\n");
}

static void iphash_init(struct xt_entry_match *match)
{
	struct xt_iphash_mtinfo *info = (void *)(match)->data;

//XT_IPHASH_NAME_LEN is currently defined as 128
	strncpy(info->name,"DEFAULT", XT_IPHASH_NAME_LEN);
	info->name[XT_IPHASH_NAME_LEN-1] = '\0';
	info->side = XT_IPHASH_SOURCE;
}

static void iphash_parse(struct xt_option_call *cb)
{
	struct xt_iphash_mtinfo *info = cb->data;

	xtables_option_parse(cb);
	switch (cb->entry->id) {
	case O_RCHECK:
		if (cb->invert)
			info->invert = true;
		break;
	case O_RSOURCE:
		info->side = XT_IPHASH_SOURCE;
		break;
	case O_RDEST:
		info->side = XT_IPHASH_DEST;
		break;
	}
}

static void iphash_check(struct xt_fcheck_call *cb)
{
	if (!(cb->xflags & F_ANY_OP))
		xtables_error(PARAMETER_PROBLEM,
			"iphash: you must specify --rcheck' ");
}

static void iphash_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_iphash_mtinfo *info = (const void *)match->data;

	if (info->invert)
		printf(" !");

	printf(" iphash:");
	printf(" CHECK");
	if(info->name) printf(" name: %s", info->name);
	if (info->side == XT_IPHASH_SOURCE)
		printf(" side: source");
	if (info->side == XT_IPHASH_DEST)
		printf(" side: dest");
}

static void iphash_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_iphash_mtinfo *info = (const void *)match->data;

	if (info->invert)
		printf(" !");
	printf(" --rcheck");
	if(info->name) printf(" --name %s",info->name);
	if (info->side == XT_IPHASH_SOURCE)
		printf(" --rsource");
	if (info->side == XT_IPHASH_DEST)
		printf(" --rdest");
}

static struct xtables_match iphash_mt_reg = {
	.name          = "iphash",
	.version       = XTABLES_VERSION,
	.family        = NFPROTO_UNSPEC,
	.size          = XT_ALIGN(sizeof(struct xt_iphash_mtinfo)),
	.userspacesize = XT_ALIGN(sizeof(struct xt_iphash_mtinfo)),
	.help          = iphash_help,
	.init          = iphash_init,
	.x6_parse      = iphash_parse,
	.x6_fcheck     = iphash_check,
	.print         = iphash_print,
	.save          = iphash_save,
	.x6_options    = iphash_opts,
};

void _init(void)
{
	xtables_register_match(&iphash_mt_reg);
}

