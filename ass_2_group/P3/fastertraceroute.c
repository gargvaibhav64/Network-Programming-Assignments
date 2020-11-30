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

#define BUFSIZE 1500

struct rec {
	unsigned short seq;		/* sequence number of this packet */
	unsigned short ttl;		/* ttl packet left with */
	struct timeval tval;    /* time packet left */
};

char recvbuf[BUFSIZE];
char sendbuf[BUFSIZE];

int datalen;
char *host;
unsigned short sport, dport;

int nsent;

pid_t pid;

const char *icmpcode_v4(int);
int recv_v4(int, struct timeval *);

void sig_alrm(int);
void traceloop(void);
void tv_sub(struct timeval *, struct timeval *);

struct proto {
    const char *(*icmpcode)(int);
    int (*recv)(int, struct timeval *);
    struct sockaddr *sasend;
    struct sockaddr *sarecv;
    struct sockaddr *salast;
    struct sockaddr *sabind;
    socklen_t salen;
    int icmpproto;
    int ttllevel;
    int ttloptname;
} *pr;

int main(int argc, char *argv[]){
    struct proto proto_v4 = {icmpcode_v4, recv_v4, NULL, NULL,
                            NULL, NULL, 0, IPPROTO_ICMP, IPPROTO_IP, IP_TTL};


    int datalen = sizeof(struct rec);
    int max_ttl = 30;
    int nprobes = 3;

    dport = 32768 + 666;

    pid = getpid();

    signal(SIGALRM, sig_alrm);

    struct addrinfo *ai;
    char *h;

    int ret;

    // Not required due to iPv4 only

    struct addrinfo hints, *res;

    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = 0;
    hints.ai_socktype = 0;

    if((ret = getaddrinfo(host, NULL, &hints, &res)) != 0){
        ai = NULL;
        perror("getaddrinfo() : ");
        exit(0);
    }        
        
    ai = res;

    pr = &proto_v4;
    pr->sasend = ai->ai_addr; /*Contains destination address */
    pr->sarecv = calloc(1, ai->ai_addrlen);
    pr->salast = calloc(1, ai->ai_addrlen);
    pr->sabind = calloc(1, ai->ai_addrlen);
    pr->salen = ai->ai_addrlen;

    traceloop();

    return 0;
}

void traceloop(void) {
    int seq, code, done;

    double rtt;
    struct rec *rec;

    struct timeval tvrecv;

    int recvfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto);

    setuid(getuid());

    int sendfd = socket(pr->sasend->sa_family, SOCK_DGRAM, 0);

    pr->sabind->sa_family = pr->sasend->sa_family;

    sport = (getpid() & 0xffff) | 0x8000;

    
}


