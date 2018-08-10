
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <stdint.h>
#include <syslog.h>
#include <pthread.h>

#include <sys/syscall.h>

#include <math.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>

#include <shutils.h>

#include "ping.h"

static unsigned long long monotonic_us(void)
{
	struct timespec ts;
	syscall(SYS_clock_gettime, CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000ULL + ts.tv_nsec/1000;
}

static int init_icmp_socket(struct ping_config *config, struct sockaddr_in *dest)
{
	int ret;
	struct addrinfo hint, *answer, *curr;
	struct timeval timeout={3, 0};
	struct sockaddr_in *ipv4;
	int size = 16 * 1024;

	config->sock_ping = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(config->sock_ping < 0) return -1;

	setsockopt(config->sock_ping, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	setsockopt(config->sock_ping, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	setsockopt(config->sock_ping, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

	memset(&hint, 0, sizeof(struct addrinfo));  
	hint.ai_family = AF_INET;
	hint.ai_socktype = SOCK_STREAM;

	ret = getaddrinfo(config->host, NULL, &hint, &answer);
	if(ret != 0) {
		close(config->sock_ping);
		return -2;
	}

	dest->sin_family = AF_INET;
	for (curr = answer; curr != NULL; curr = curr->ai_next) {
		ipv4 = (struct sockaddr_in *)(curr->ai_addr);
		dest->sin_addr = ipv4->sin_addr;
		inet_ntop(AF_INET, &dest->sin_addr, config->ip, 16);
DPRINTF("%s\n", config->ip);
		break;
	}

	freeaddrinfo(answer);		//donnot forget

	return ret;
}

static int unpack(struct ping_config *config)
{
	struct ip *Ip = (struct ip *)(config->recv_pkt);
	struct icmp *Icmp;
	int ipHeadLen;
	unsigned long long rtt, send_time;
	char *dst_ip;

	ipHeadLen = Ip->ip_hl << 2;
	Icmp = (struct icmp *)(config->recv_pkt + ipHeadLen);

	if ((Icmp->icmp_type == ICMP_ECHOREPLY) && Icmp->icmp_id == config->my_id)
	{
		memcpy(&send_time, &Icmp->icmp_data, sizeof(unsigned long long));
DPRINTF("send_time=%llu config->SendTime=%llu\n", send_time, config->SendTime);

		rtt = config->RecvTime - config->SendTime;
		if((config->SendTime != send_time)||(rtt < 0)){
			return -1;
		}

		dst_ip = inet_ntoa(Ip->ip_src);
		if((dst_ip == NULL)||(strcmp(dst_ip, config->ip) != 0)) return -2;

		if (rtt < config->min || 0 == config->min) config->min = rtt;
		if (rtt > config->max) config->max = rtt;
		config->total += rtt;

		DPRINTF("%u bytes from %s: SendBag=%d icmp_seq=%u ttl=%u time=%lluus total_time=%lluus\n",
			ntohs(Ip->ip_len) - ipHeadLen - 8,
			inet_ntoa(Ip->ip_src),
			config->SendBag,
			Icmp->icmp_seq,
			Ip->ip_ttl,
			rtt,
			config->total);

		return 0;
	}

	return -3;
}

static u_int16_t Compute_cksum(struct ping_config *config, struct icmp *pIcmp)
{
	u_int16_t *data = (u_int16_t *)pIcmp;
	int len = config->size + 8;
	u_int32_t sum = 0;
	
	while (len > 1) {
		sum += *data++;
		len -= 2;
	}
	if (1 == len) {
		u_int16_t tmp = *data;
		tmp &= 0xff00;
		sum += tmp;
	}

	while (sum >> 16)
		sum = (sum >> 16) + (sum & 0x0000ffff);
	sum = ~sum;

	return sum;
}

static void SetICMP(struct ping_config *config)
{
	struct icmp *pIcmp;

	pIcmp = (struct icmp*)(config->send_pkt);
	pIcmp->icmp_type = ICMP_ECHO;
	pIcmp->icmp_code = 0;
	pIcmp->icmp_cksum = 0;					//校验和
	pIcmp->icmp_seq = config->my_seq;			//序号
	pIcmp->icmp_id = config->my_id;
	memcpy(&pIcmp->icmp_data, &config->SendTime, sizeof(unsigned long long));

	pIcmp->icmp_cksum = Compute_cksum(config, pIcmp);
}

//---------------------------------------------------------------------------------------------------------------------------------------------
static void Statistics(struct ping_config *config)
{
	int PacketLossProbability;

	if(config->ReceiveBag > 0) {
		config->avg = config->total/config->ReceiveBag;
	} else {
		config->avg = 0;
	}

	if(config->SendBag != 0) {
		PacketLossProbability = ((config->SendBag - config->ReceiveBag) * 100) / config->SendBag;
	} else {
		PacketLossProbability = 100;
	}

	config->MaxDelay = (int)(config->max / 1000);
	config->MinDelay = (int)(config->min / 1000);
	config->AvgDelay = (int)(config->avg / 1000);

	DPRINTF("--- %s  ping statistics ---\n", config->host);

	DPRINTF("%d packets transmitted, %d received, %d%% packet loss\n"
		, config->SendBag, config->ReceiveBag, PacketLossProbability);

	DPRINTF("rtt min/avg/max = %d/%d/%d ms\n",config->MinDelay, config->AvgDelay, config->MaxDelay);

	close(config->sock_ping);
}

static int RecvePacket(struct ping_config *config, struct sockaddr_in *dest_addr)
{
	int ret;
	int RecvBytes = 0;
	int addrlen = sizeof(struct sockaddr_in);

	RecvBytes = recvfrom(config->sock_ping, config->recv_pkt, config->size + 28, 0, (struct sockaddr *)dest_addr, &addrlen);

	if (RecvBytes < 0) {
DPRINTF("recvfrom fail\n");
		return -1;
	}

DPRINTF("RecvBytes=%d\n", RecvBytes);
	config->RecvTime = monotonic_us();
	ret = unpack(config);
	if((ret == -2)||(ret == -3)) {
		return -2; 
	}

	if(ret == 0) config->ReceiveBag++;

	return ret;
}

static int SendPacket(struct ping_config *config, struct sockaddr_in *dest_addr)
{
	int SendBytes;
	int addrlen = sizeof(struct sockaddr_in);

	config->SendTime = monotonic_us();

	SetICMP(config);

	SendBytes = sendto(config->sock_ping, config->send_pkt, config->size + 8, 0, (struct sockaddr *)dest_addr, addrlen);
	if (SendBytes < 0) {
DPRINTF("sento fail\n");
		return -1;
	}

DPRINTF("SendBytes=%d\n", SendBytes);
	config->SendBag++;
	config->my_seq++;

	return 0;
}

static int real_do_ping(struct ping_config *config)
{
	int ret;
	struct sockaddr_in dest_addr;

	ret = init_icmp_socket(config, &dest_addr);

	if(ret != 0) {
DPRINTF("init_icmp_socket fail\n");
		return -1;
	}

DPRINTF("PING %s(%s) %d bytes of data.\n", config->host, inet_ntoa(dest_addr.sin_addr), config->size);

	while (config->SendBag < config->num) {
		ret = SendPacket(config, &dest_addr);
		if(ret != 0) {
DPRINTF("send fail\n");
			return -1;
		}

		ret = RecvePacket(config, &dest_addr);
		if (ret == -2) {
DPRINTF("loopback received\n");
			RecvePacket(config, &dest_addr);
		}

		usleep(config->interval * 1000);		// sleep
	}

	Statistics(config);	//输出信息，关闭套接字

	return 0;
}

int do_ping(char *host, int count)
{
	int ret;
	struct ping_config config;

	memset(&config, 0, sizeof(config));

	config.interval = 2000;		// 2000ms
	config.my_id = (u_int16_t)rand();
	config.my_seq = 1;
	config.size = 32;
	config.num = count;
	strcpy(config.host, host);
	config.send_pkt = malloc(config.size + 28);
	if(config.send_pkt == NULL) return -1;
	config.recv_pkt = malloc(config.size + 28);
	if(config.recv_pkt == NULL) {
		free(config.send_pkt);
		return -2;
	}

	ret = real_do_ping(&config);
	free(config.send_pkt);
	free(config.recv_pkt);

	if(ret != 0) return -3;

	if(config.ReceiveBag == 0) return -4;

	return 0;
};


