
#define		DPRINTF(format, ...)//	printf(format, ##__VA_ARGS__)

struct ping_config {
//for config
	char host[64];
	char ip[16];
	int size;
	int num;
	int interval;

//for socket
	int sock_ping;
	u_int16_t my_seq;
	u_int16_t my_id;

	char *send_pkt;			//send socket buf
	char *recv_pkt;			//recv socket buf

//for result
	int SendBag;
	int ReceiveBag;
	int MinDelay;
	int MaxDelay;
	int AvgDelay;

//for compute
	unsigned long long min;
	unsigned long long avg;
	unsigned long long max;
	unsigned long long total;
	unsigned long long RecvTime;
	unsigned long long SendTime;

};

extern int do_ping(char *host, int count);
extern volatile int gotuser;

