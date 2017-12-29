
#include "rc.h"

#define BUF_SIZE 512

/*
static int tinc_gfwlist(void)
{
	char buf[BUF_SIZE];
	FILE *f_list;
	char *p, *q;
	pid_t pid;
	int ret;
	char *tinc_gfwlist_argv[] = {"/usr/bin/wget", "-T", "500", "-O", "/etc/tinc/gfw_list.sh", nvram_safe_get("tinc_gfwlist_url"), NULL};

	ret = _eval(tinc_gfwlist_argv, NULL, 0, &pid);
	if(ret != 0) {
		fprintf(stderr, "[vpn] tinc download gfwlist fail\n");
		return ret;
	}

	chmod("/etc/tinc/gfw_list.sh", 0700);
	system("/etc/tinc/gfw_list.sh");

	return 0;
}
*/

int tinc_start_main(int argc_tinc, char *argv_tinc[])
{
	char buffer[BUF_SIZE];
	FILE *f_tinc;
/*
	pid_t pid;
	int ret;
	char *tinc_config_argv[] = {"/usr/bin/wget", "-T", "120", "-O", "/etc/tinc/tinc.tar.gz", nvram_safe_get("tinc_url"), NULL};

	ret = _eval(tinc_config_argv, NULL, 0, &pid);

	if(ret != 0) {
		fprintf(stderr, "[vpn] tinc download congfig fail\n");
		return ret;
	}
*/
	if (!( f_tinc = fopen("/etc/tinc/tinc.sh", "w"))) {
		perror( "/etc/tinc/tinc.sh" );
		return -1;
	}

	fprintf(f_tinc,
		"#!/bin/sh\n"
		"ip rule del to 8.8.8.8 pref 5 table 200\n"

		"if [ A$1 == A\"stop\" ];then\n"
			"exit\n"
		"fi\n"

		"ip rule add to 8.8.8.8 pref 5 table 200\n"

		"wget -T 120 -O /etc/tinc/tinc.tar.gz \"%s?mac=%s&id=%s&model=RT-AC1200GP\"\n"
		"if [ $? -ne 0 ];then\n"
			"exit\n"
		"fi\n"

		"if [ ! -n /etc/gfw_list.sh ];then\n"
			"wget -T 500 -O /etc/gfw_list.sh \"%s\"\n"
		"fi\n"
		"if [ $? -ne 0 ];then\n"
			"exit\n"
		"fi\n"

		"cd /etc/tinc\n"
		"tar -zxvf tinc.tar.gz\n"
		"chmod -R 0700 /etc/tinc\n"
		"tinc -n gfw start\n"

		"chmod +x /etc/gfw_list.sh\n"
		"/bin/sh /etc/gfw_list.sh\n"
		, nvram_safe_get("tinc_url")
		, nvram_safe_get("et0macaddr")
		, nvram_safe_get("tinc_id")
		, nvram_safe_get("tinc_gfwlist_url")
	);

	fclose(f_tinc);
	chmod("/etc/tinc/tinc.sh", 0700);
	system("/etc/tinc/tinc.sh start");

//	tinc_gfwlist();

	return 0;
}

void start_tinc(void)
{
	if(nvram_get_int("tinc_enable") != 1) return;

	modprobe("tun");
	mkdir("/etc/tinc", 0700);

	f_write_string("/proc/1/net/xt_srd/DEFAULT", "+google.com", 0, 0);
	f_write_string("/proc/1/net/xt_srd/DEFAULT", "+facebook.com", 0, 0);
	f_write_string("/proc/1/net/xt_srd/DEFAULT", "+youtube.com", 0, 0);
	f_write_string("/proc/1/net/xt_srd/DEFAULT", "+twitter.com", 0, 0);

	xstart("tinc_start");

	return;
}

void stop_tinc(void)
{
	killall_tk("tinc_start");
	killall_tk("tincd");
	eval("/etc/tinc/tinc.sh", "stop");
	system( "/bin/rm -rf /etc/tinc\n" );

	return;
}

