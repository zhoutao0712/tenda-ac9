#ifndef _LINUX_NETFILTER_XT_SRD_H
#define _LINUX_NETFILTER_XT_SRD_H 1

#include <linux/types.h>

#define XT_SRD_NAME_LEN	128

struct xt_srd_mtinfo {
	char name[XT_SRD_NAME_LEN];
};

#endif /* _LINUX_NETFILTER_XT_SRD_H */
