#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <xtables.h>
#include <linux/netfilter/xt_srd.h>

enum {
	O_NAME = 0,
};

#define s struct xt_srd_mtinfo
static const struct xt_option_entry srd_opts[] = {
	{.name = "name", .id = O_NAME, .type = XTTYPE_STRING,
	 .flags = XTOPT_PUT, XTOPT_POINTER(s, name)},
	XTOPT_TABLEEND,
};
#undef s

static void srd_help(void)
{
	printf(
"srd(smart routing of dns resolve) match options:\n"
"    --name name                 Name of the iphash(xt_iphash.ko) list and dns host list to be use. DEFAULT used if none given.\n"
"xt_srd by: zhoutao0712 <846207054@qq.com>\n");
}

static void srd_init(struct xt_entry_match *match)
{
	struct xt_srd_mtinfo *info = (void *)(match)->data;

//XT_SRD_NAME_LEN is currently defined as 128
	strncpy(info->name,"DEFAULT", XT_SRD_NAME_LEN);
	info->name[XT_SRD_NAME_LEN-1] = '\0';
}

static void srd_parse(struct xt_option_call *cb)
{
	struct xt_srd_mtinfo *info = cb->data;

	xtables_option_parse(cb);
}

static void srd_check(struct xt_fcheck_call *cb)
{
}

static void srd_print(const void *ip, const struct xt_entry_match *match, int numeric)
{
	const struct xt_srd_mtinfo *info = (const void *)match->data;

	printf(" srd:");
	if(info->name) printf(" name: %s", info->name);
}

static void srd_save(const void *ip, const struct xt_entry_match *match)
{
	const struct xt_srd_mtinfo *info = (const void *)match->data;

	if(info->name) printf(" --name %s",info->name);
}

static struct xtables_match srd_mt_reg = {
	.name          = "srd",
	.version       = XTABLES_VERSION,
	.family        = NFPROTO_UNSPEC,
	.size          = XT_ALIGN(sizeof(struct xt_srd_mtinfo)),
	.userspacesize = XT_ALIGN(sizeof(struct xt_srd_mtinfo)),
	.help          = srd_help,
	.init          = srd_init,
	.x6_parse      = srd_parse,
	.x6_fcheck     = srd_check,
	.print         = srd_print,
	.save          = srd_save,
	.x6_options    = srd_opts,
};

void _init(void)
{
	xtables_register_match(&srd_mt_reg);
}

