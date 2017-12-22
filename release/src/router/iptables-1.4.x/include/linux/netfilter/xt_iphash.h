#ifndef _LINUX_NETFILTER_XT_IPHASH_H
#define _LINUX_NETFILTER_XT_IPHASH_H 1

#include <linux/types.h>

#define XT_IPHASH_NAME_LEN	128

enum {
	XT_IPHASH_SOURCE   = 0,
	XT_IPHASH_DEST     = 1,
};

struct xt_iphash_mtinfo {
	char name[XT_IPHASH_NAME_LEN];
	__u8 side;
	__u8 invert;
};

#endif /* _LINUX_NETFILTER_XT_IPHASH_H */
