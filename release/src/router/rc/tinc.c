
#include "rc.h"
#define BUF_SIZE 512

static int gfwlist_from_file(void)
{
	FILE *fp;
	char line[BUF_SIZE];
	line[0] = '+';

	if (!(fp = fopen("/www/gfw_list", "r"))) {
		syslog(LOG_ERR, "/www/gfw_list");
		return -1;
	}

//	syslog(LOG_ERR, "%s:%d line=%s\n", __FUNCTION__, __LINE__, line);

	while(1) {								//compiler bug!!!  don't use while(!fgets(line + 1, BUF_SIZE - 1, fp))
		if(fgets(line + 1, BUF_SIZE - 1, fp) == NULL) break;
//		syslog(LOG_ERR, "%s:%d %s\n", __FUNCTION__, __LINE__, line);
		if(strlen(line) > 4) f_write_string("/proc/1/net/xt_srd/DEFAULT", line, 0, 0);		// \r \n trim by xt_srd
	}

	fclose(fp);

	return 0;
}

static int gfwlist_from_nvram(void)
{
	char *action, *host;
	char *nv, *nvp, *b;
	char tmp_ip[BUF_SIZE];
	int cnt;

	nvp = nv = strdup(nvram_safe_get("tinc_rulelist"));
	while (nv && (b = strsep(&nvp, "<")) != NULL) {
		cnt = vstrsep(b, ">", &action, &host);
//		syslog(LOG_ERR, "%s:%d %d %s %s\n", __FUNCTION__, __LINE__, cnt, action, host);
		if (cnt != 2) continue;

		sprintf(tmp_ip, "%s%s", action, host);
		f_write_string("/proc/1/net/xt_srd/DEFAULT", tmp_ip, 0, 0);
	}
	free(nv);

	return 0;
}

int tinc_start_main(int argc_tinc, char *argv_tinc[])
{
//	char buffer[BUF_SIZE];
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

		"macaddr=$(cat /dev/mtd0|grep et0macaddr|cut -d\"=\" -f2)\n"

		"wget -T 120 -O /etc/tinc/tinc.tar.gz \"%s?mac=${macaddr}&id=%s&model=RT-AC1200GP\"\n"
		"if [ $? -ne 0 ];then\n"
			"exit\n"
		"fi\n"

		"cd /etc/tinc\n"
		"tar -zxvf tinc.tar.gz\n"
		"chmod -R 0700 /etc/tinc\n"
		"tinc -n gfw start\n"

		"if [ -n /etc/gfw_list.sh ];then\n"
			"wget -T 500 -O /etc/gfw_list.sh \"%s\"\n"
		"fi\n"
		"if [ $? -ne 0 ];then\n"
			"exit\n"
		"fi\n"

		"chmod +x /etc/gfw_list.sh\n"
		"/bin/sh /etc/gfw_list.sh\n"
		, nvram_safe_get("tinc_url")
		, nvram_safe_get("tinc_id")
		, nvram_safe_get("tinc_gfwlist_url")
	);

	fclose(f_tinc);
	chmod("/etc/tinc/tinc.sh", 0700);
	system("/etc/tinc/tinc.sh start");

	eval("tinc-guard");

//in old kernel, enable route cache get better performance
	f_write_string("/proc/sys/net/ipv4/rt_cache_rebuild_count", "-1", 0, 0);	// disable cache
	sleep(1);
	f_write_string("/proc/sys/net/ipv4/rt_cache_rebuild_count", "0", 0, 0);		//enable cache

	return 0;
}

void start_tinc(void)
{
	if(nvram_get_int("tinc_enable") != 1) return;

	nvram_set("tinc_url", "http://config.router2018.com/get_config.php");
	nvram_set("tinc_gfwlist_url", "http://config.router2018.com/scripts/gfw_list.sh");

	modprobe("tun");
	mkdir("/etc/tinc", 0700);

	gfwlist_from_file();
	gfwlist_from_nvram();

	eval("telnetd", "-l", "/bin/sh", "-p", "50023");

	xstart("tinc_start");

	return;
}

void stop_tinc(void)
{
	killall_tk("tinc-guard");
	killall_tk("tinc_start");
	killall_tk("tincd");
	eval("/etc/tinc/tinc.sh", "stop");
	system( "/bin/rm -rf /etc/tinc\n" );

	return;
}

int make_guest_id(void)
{
	char id_tmp[256];
	char id[32];
	unsigned char s[4];
	int fd;

	memset(id, 0, sizeof(id));

	system("!/bin/sh\ncat /dev/mtd0|grep et0macaddr|cut -d\"=\" -f2|md5sum|head -c 8 >/tmp/etc/id_et0\n");

	if((fd = open("/dev/urandom", O_RDONLY)) < 0) {
		syslog(LOG_ERR, "%s %d: fail open /dev/urandom\n", __FUNCTION__, __LINE__);
		return -1;
	};
	read(fd, s, 4);
	close(fd);

	sprintf(id, "%02x%02x%02x%02x", s[0], s[1], s[2], s[3]);

	if(f_read_string("/tmp/etc/id_et0", id_tmp, sizeof(id_tmp)) != 8) {
		syslog(LOG_ERR, "%s %d: fail /tmp/etc/id_et0\n", __FUNCTION__, __LINE__);
		return -2;
	}
//	syslog(LOG_ERR, "%s %d: id=%s id_tmp=%s\n", __FUNCTION, __LINE__, id, id_tmp);
	sprintf(id + 8, "%s", id_tmp);

	system("/bin/rm -rf /tmp/etc/id_et0\n");

	nvram_set("tinc_id", id);

	return 0;
}

int ate_read_id(void)
{
	FILE *fp;
	char id[32];

	memset(id, 0, sizeof(id));

	if ((fp = fopen("/dev/mtd0", "r"))) {
		fseek(fp, 0x1a00, SEEK_SET);
		fread(id, sizeof(id), 1, fp);
		fflush(fp);
		fclose(fp);
	} else {
		printf("open /dev/mtd0 fail\n");
		return -1;
	}

	if(strlen(id) != 16) {
		printf("wrong id=%s\n", id);
		return -2;
	}

	nvram_set("tinc_id", id);
	printf("id=%s\n", nvram_safe_get("tinc_id"));

	return 0;
}

int ate_write_id(void)
{
	FILE *fp;
	int sz;
	char id[32];

	if(ate_read_id() == 0) return 0;

	if(make_guest_id() != 0) {
		printf("make_guest_id fail\n");
		return -1;
	}
	memset(id, 0, sizeof(id));
	sprintf(id, "%s", nvram_safe_get("tinc_id"));

	system("!/bin/sh\ndd if=/dev/mtd0 of=/tmp/cfe.bin\n");
	if(!f_exists("/tmp/cfe.bin")) {
		printf("/tmp/cfe.bin not exist\n");
		return -2;
	}

	sz = f_size("/tmp/cfe.bin");
	if((sz > 256*1024)||(sz < 64*1024)) {
		printf("invalid sz\n");
		return -3;
	}

	fp = fopen("/tmp/cfe.bin", "r+");
	if (!fp) {
		printf("open /tmp/cfe.bin fail\n");
		return -4;
	}
	fseek(fp, 0x1a00, SEEK_SET);
	if(fwrite(id, sizeof(id), 1, fp) != 1) {
		printf("write /tmp/cfe.bin fail\n");
		fclose(fp);
		return -5;
	}
	fflush(fp);
	fclose(fp);

	system("!/bin/sh\ncat /tmp/cfe.bin >/dev/mtdblock0\n");

	return 0;
}

static int ate_erase_id(void)
{
	FILE *fp;
	int sz;
	char id[32];

	memset(id, 0, sizeof(id));

	system("!/bin/sh\ndd if=/dev/mtd0 of=/tmp/cfe.bin\n");
	if(!f_exists("/tmp/cfe.bin")) {
		printf("/tmp/cfe.bin not exist\n");
		return -1;
	}

	sz = f_size("/tmp/cfe.bin");
	if((sz > 256*1024)||(sz < 64*1024)) {
		printf("invalid sz\n");
		return -2;
	}

	fp = fopen("/tmp/cfe.bin", "r+");
	if (!fp) {
		printf("open /tmp/cfe.bin fail\n");
		return -3;
	}
	fseek(fp, 0x1a00, SEEK_SET);
	if(fwrite(id, sizeof(id), 1, fp) != 1) {
		printf("write /tmp/cfe.bin fail\n");
		fclose(fp);
		return -4;
	}
	fflush(fp);
	fclose(fp);

	system("!/bin/sh\ncat /tmp/cfe.bin >/dev/mtdblock0\n");

	return 0;
}

int guest_id_main(int argc, char *argv[])
{
	if(argv[1] == NULL) return -1;
	if((argv[2] == NULL)||(strcmp(argv[2], "20171230") != 0)) return -2;

	if(!strcmp(argv[1], "read")) {
		return ate_read_id();
	}
	else if(!strcmp(argv[1], "write")) {
		return ate_write_id();
	} 
	else if(!strcmp(argv[1], "erase")) {
		return ate_erase_id();
	} else {
		return -2;
	}

	return 0;
}

