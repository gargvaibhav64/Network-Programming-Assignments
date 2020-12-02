#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <pthread.h>

#define BUFSIZE 1500
#define MAX_TTL 30
#define NUM_PROBES 3

struct rec {
	unsigned short seq;		/* sequence number of this packet */
	unsigned short ttl;		/* ttl packet left with */
	struct timeval tval;    /* time packet left */
};

char recvbuf[BUFSIZE];
char sendbuf[BUFSIZE];
int num_bytes_read;

int datalen;
char *host;
unsigned short sport, dport;

int nprobes;
int sendfd, recvfd;	/* send on UDP sock, read on raw ICMP sock */
int max_ttl;

int nsent;

pid_t pid;

const char *icmpcode_v4(int);
int recv_v4(int, struct timeval *);

void sig_alrm(int);
void traceloop(void);
void tv_sub(struct timeval *, struct timeval *);
char * sock_ntop_host(const struct sockaddr *sa, socklen_t salen);
int sock_cmp_addr(const struct sockaddr *sa1, const struct sockaddr *sa2, socklen_t salen);

double rtt;
struct rec *rec;

struct timeval tvrecv;

struct proto {
    const char *(*icmpcode)(int);
    // int (*recv)(int, struct timeval *);
    struct sockaddr *sasend;
    struct sockaddr *sarecv;
    struct sockaddr *salast;
    struct sockaddr *sabind;
    socklen_t salen;
    int icmpproto;
    int ttllevel;
    int ttloptname;
} *pr;

typedef struct thread_data {
   int ttl;
   char * str;
} thread_data;