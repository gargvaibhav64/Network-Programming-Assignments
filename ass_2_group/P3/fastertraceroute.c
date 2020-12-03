#include "trace.h"

int datalen = sizeof(struct rec); /* defaults */
int max_ttl = 30;
int nprobes = 3;
u_short dport = 32768 + 666;
int done;

pthread_mutex_t sendbuf_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wait_on_icmp[MAX_TTL];
pthread_mutex_t wait_mutex[MAX_TTL] = {PTHREAD_MUTEX_INITIALIZER};

struct proto proto_v4 = {icmpcode_v4, NULL, NULL,
                            NULL, NULL, 0, IPPROTO_ICMP, IPPROTO_IP, IP_TTL};

int gotalarm;

void sig_alrm(int signo)
{
    gotalarm = 1; /* set flag to note that alarm occurred */
    return;       /* and interrupt the recvfrom() */
}

int main(int argc, char *argv[])
{
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

    host = "www.google.com";

    if ((ret = getaddrinfo(host, NULL, &hints, &res)) != 0)
    {
        ai = NULL;
        perror("getaddrinfo() : ");
        exit(0);
    }

    ai = res;

    pr = &proto_v4;
    pr->sasend = ai->ai_addr; /*Contains destination address */
    pr->sarecv = (struct sockaddr *)calloc(1, ai->ai_addrlen);
    pr->salast = (struct sockaddr *)calloc(1, ai->ai_addrlen);
    pr->sabind = (struct sockaddr *)calloc(1, ai->ai_addrlen);
    pr->salen = ai->ai_addrlen;

    traceloop();

    return 0;
}

int process_icmp(char * recvbuf, int n, int seq){

    int				hlen1, hlen2, icmplen, ret;
	struct ip		*ip, *hip;
	struct icmp		*icmp;
	struct udphdr	*udp;

    ip = (struct ip *) recvbuf;	/* start of IP header */
    hlen1 = ip->ip_hl << 2;		/* length of IP header */

    icmp = (struct icmp *) (recvbuf + hlen1); /* start of ICMP header */
    if ( (icmplen = n - hlen1) < 8)
        return -4;				/* not enough to look at ICMP header */

    if (icmp->icmp_type == ICMP_TIMXCEED &&
        icmp->icmp_code == ICMP_TIMXCEED_INTRANS) {
        if (icmplen < 8 + sizeof(struct ip))
            return -4;			/* not enough data to look at inner IP */

        hip = (struct ip *) (recvbuf + hlen1 + 8);
        hlen2 = hip->ip_hl << 2;
        if (icmplen < 8 + hlen2 + 4)
            return -4;			/* not enough data to look at UDP ports */

        udp = (struct udphdr *) (recvbuf + hlen1 + 8 + hlen2);
        if (hip->ip_p == IPPROTO_UDP &&
            udp->uh_sport == htons(sport) &&
            udp->uh_dport == htons(dport + seq)) {
            ret = -2;		/* we hit an intermediate router */
            return -4;
        }

    } else if (icmp->icmp_type == ICMP_UNREACH) {
        if (icmplen < 8 + sizeof(struct ip))
            return -4;			/* not enough data to look at inner IP */

        hip = (struct ip *) (recvbuf + hlen1 + 8);
        hlen2 = hip->ip_hl << 2;
        if (icmplen < 8 + hlen2 + 4)
            return -4;			/* not enough data to look at UDP ports */

        udp = (struct udphdr *) (recvbuf + hlen1 + 8 + hlen2);
        if (hip->ip_p == IPPROTO_UDP &&
            udp->uh_sport == htons(sport) &&
            udp->uh_dport == htons(dport + seq)) {
            if (icmp->icmp_code == ICMP_UNREACH_PORT)
                ret = -1;	/* have reached destination */
            else
                ret = icmp->icmp_code;	/* 0, 1, 2, ... */
            return -4;
        }
    }
    int seq1 = ntohs(udp->uh_dport)-dport;

    // Set ttl to appropriate thread
    int ttl = (seq1 - 1)/NUM_PROBES + 1;

    printf("*** TTL: %2d Seq: %2d \n", ttl, seq1);
    return ret;
}

void * icmp1(void * arg){
    int n;
    socklen_t len;

    int				hlen1, hlen2, icmplen, ret;
	struct ip		*ip, *hip;
	struct icmp		*icmp;
	struct udphdr	*udp;

    int nprobe[MAX_TTL] = {0};

    fd_set rset;
    FD_ZERO(&rset);
    

    struct timeval sel_tval;

    int nready = 0;

    while(1){
        sel_tval.tv_sec = 5;
        FD_SET(recvfd, &rset);
        int nready = select(recvfd+1 , &rset, NULL, NULL, &sel_tval);

        if(FD_ISSET(recvfd, &rset)){
            n = recvfrom(recvfd, recvbuf, sizeof(recvbuf), 0, pr->sarecv, &len);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                else
                    perror("recvfrom error : ");
            }

            ip = (struct ip *) recvbuf;	/* start of IP header */
            hlen1 = ip->ip_hl << 2;		/* length of IP header */
        
            icmp = (struct icmp *) (recvbuf + hlen1); /* start of ICMP header */
            if ( (icmplen = n - hlen1) < 8)
                continue;				/* not enough to look at ICMP header */

            if (icmplen < 8 + sizeof(struct ip))
                continue;			/* not enough data to look at inner IP */

            hip = (struct ip *) (recvbuf + hlen1 + 8);
            hlen2 = hip->ip_hl << 2;
            if (icmplen < 8 + hlen2 + 4)
                continue;			/* not enough data to look at UDP ports */

            udp = (struct udphdr *) (recvbuf + hlen1 + 8 + hlen2);

            int seq1 = ntohs(udp->uh_dport)-dport;

            // Set ttl to appropriate thread
            int ttl = (seq1 - 1)/NUM_PROBES + 1;

            printf("--- TTL: %2d Seq: %2d \n", ttl, seq1);

            if(ttl > 30 || ttl < 1){
                continue;
            } 
                
            nprobe[ttl-1]++;

            num_bytes_read = n;

            gettimeofday(&tvrecv, NULL);

            struct timespec t1;
            t1.tv_sec = tvrecv.tv_sec + 2;
            t1.tv_nsec = 0;

            pthread_cond_signal(&wait_on_icmp[ttl-1]);
            // pthread_mutex_lock(&wait_mutex[ttl-1]);
            // int x = pthread_cond_timedwait(&wait_on_icmp[ttl-1], &wait_mutex[ttl-1], &t1);
            if(nprobe[ttl-1] <= 3){
                int x = pthread_cond_wait(&wait_on_icmp[ttl-1], &wait_mutex[ttl-1]);
            }
        } 
        else { // select timed out. No replies since last 5 seconds.
            recvbuf[0] = '\0';
            for(int i = 0 ; i < max_ttl ; i++){
                pthread_cond_signal(&wait_on_icmp[i]);
            }
        }

    }
}

void * ttlUtil(void * arg)
{
    struct sockaddr *sarecv_copy = (struct sockaddr *)calloc(1, pr->salen);;
    struct sockaddr *salast_copy = (struct sockaddr *)calloc(1, pr->salen);;
    thread_data * thr = (thread_data *)arg;
    int ttl = thr->ttl;

    char * buf = (char *) malloc(sizeof(char) * 1500);
    sprintf(buf, "%2d ", ttl);
    strcpy(thr->str, buf);

    char recv_copy[BUFSIZE];
    char send_copy[BUFSIZE];

    int seq1;

    for (int probe = 0; probe < nprobes; probe++)
    {
        rec = (struct rec *)send_copy;
        ++seq1;
        rec->seq = (ttl - 1)* nprobes + seq1;
        rec->ttl = ttl;
        gettimeofday(&rec->tval, NULL);

        // printf("%d seq sent in ttl : \n", rec->seq);
        struct sockaddr_in *raw_serv;
        
        // Mutex open
        pthread_mutex_lock(&sendbuf_mutex);

            printf("+++ TTL: %2d Seq: %2d\n", ttl, (ttl - 1)* nprobes + seq1);
            setsockopt(sendfd, pr->ttllevel, pr->ttloptname, &ttl, sizeof(int));
            bzero(pr->salast, pr->salen);
            strcpy(sendbuf, send_copy);
            raw_serv = (struct sockaddr_in *)pr->sasend;
            raw_serv->sin_port = htons(dport + rec->seq);
            pr->sasend = (struct sockaddr *)raw_serv;
            sendto(sendfd, sendbuf, datalen, 0, pr->sasend, pr->salen);

        pthread_mutex_unlock(&sendbuf_mutex);
        // Mutex close

        int num_bytes;
        struct timeval tvrecv1;

        pthread_cond_wait(&wait_on_icmp[ttl-1], &wait_mutex[ttl-1]);
            memcpy(recv_copy, recvbuf, BUFSIZE);
            num_bytes = num_bytes_read;
            tvrecv1.tv_sec = tvrecv.tv_sec;
            tvrecv1.tv_usec = tvrecv.tv_usec;
            memcpy(sarecv_copy, pr->sarecv, pr->salen);
            // printf("Signalled %d\n", ttl);
        pthread_cond_signal(&wait_on_icmp[ttl-1]);
        pthread_mutex_unlock(&wait_mutex[ttl-1]);

        int code;

        if(recv_copy[0] == '\0'){
            // Timed out.
            sprintf(buf, " *"); /* timeout, no reply */
            strcpy(thr->str, buf);
            break;         
        }

        if ((code = process_icmp(recv_copy, num_bytes, (ttl - 1)* nprobes + seq1)) == -3) // Change receive to wake from thread
        {
            printf(buf, " *"); /* timeout, no reply */
            sprintf(buf, " *"); /* timeout, no reply */
            strcpy(thr->str, buf);
        }
        else
        {
            char str[NI_MAXHOST];

            if (sock_cmp_addr(sarecv_copy, salast_copy, pr->salen) != 0)
            {
                if (getnameinfo(sarecv_copy, pr->salen, str, sizeof(str), NULL, 0, 0) == 0){
                    printf(" %s (%s)", str, sock_ntop_host(sarecv_copy, pr->salen));
                    sprintf(buf, " %s (%s)", str, sock_ntop_host(sarecv_copy, pr->salen));
                    strcpy(thr->str, buf);
                }
                else{
                    sprintf(buf, " %s", sock_ntop_host(sarecv_copy, pr->salen));
                    strcpy(thr->str, buf);
                }
                memcpy(salast_copy, sarecv_copy, pr->salen);
            }

            tv_sub(&tvrecv1, &rec->tval);
            rtt = tvrecv1.tv_sec * 1000.0 + tvrecv1.tv_usec / 1000.0;
            printf(" %.3f ms", rtt);
            sprintf(buf, " %.3f ms", rtt);
            strcpy(thr->str, buf);

            if (code == -1) /* port unreachable; at destination */
            {
                if(ttl < done)
                    done = ttl;
            }
            else if (code >= 0) {
                printf(" (ICMP %s)", (*pr->icmpcode)(code));
                sprintf(buf, " (ICMP %s)", (*pr->icmpcode)(code));
                strcpy(thr->str, buf);
            }
        }
        // printf("Signalled %d\n", ttl);
        pthread_cond_signal(&wait_on_icmp[ttl-1]);
        printf("\n");
    }
    sprintf(buf, "\n");
    strcpy(thr->str, buf);
    printf("%s", thr->str);
}

void traceloop(void)
{
    if ((recvfd = socket(pr->sasend->sa_family, SOCK_RAW, pr->icmpproto)) == -1)
    {
        perror("server: socket()");
        exit(1);
    }

    setuid(getuid());

    if ((sendfd = socket(pr->sasend->sa_family, SOCK_DGRAM, 0)) == -1)
    {
        perror("server: socket()");
        exit(1);
    }

    pr->sabind->sa_family = pr->sasend->sa_family;

    sport = (getpid() & 0xffff) | 0x8000;

    struct sockaddr_in *serv = (struct sockaddr_in *)pr->sabind;
    serv->sin_port = htons(sport);

    if (bind(sendfd, (struct sockaddr *)serv, sizeof(*serv)) == -1)
    {
        perror("server: bind()");
        close(sendfd);
        exit(1);
    }

    pr->sabind = (struct sockaddr *)serv;

    sig_alrm(SIGALRM);
    done = INT8_MAX;

    pthread_t pt[max_ttl], ic_thread;
    thread_data thr[max_ttl];

    pthread_create(&ic_thread, NULL, icmp1, NULL);

    for (int ttl = 1; ttl <= max_ttl; ttl++)
    {
        thr[ttl - 1].ttl = ttl;
        thr[ttl - 1].str = (char *) malloc(sizeof(char) * 1500);
        pthread_create(&pt[ttl-1], NULL, ttlUtil, (void *)&thr[ttl-1]);
    }

    for (int ttl = 1; ttl <=max_ttl; ttl++){
        pthread_join(pt[ttl-1], NULL);
        pthread_cond_signal(&wait_on_icmp[ttl-1]);
    }

    for (int ttl = 1; ttl <=done; ttl++){
        printf("%s\n", thr[ttl-1].str);
    }    
}

const char *icmpcode_v4(int code)
{
    static char errbuf[100];
    switch (code)
    {
    case 0:
        return ("network unreachable");
    case 1:
        return ("host unreachable");
    case 2:
        return ("protocol unreachable");
    case 3:
        return ("port unreachable");
    case 4:
        return ("fragmentation required but DF bit set");
    case 5:
        return ("source route failed");
    case 6:
        return ("destination network unknown");
    case 7:
        return ("destination host unknown");
    case 8:
        return ("source host isolated (obsolete)");
    case 9:
        return ("destination network administratively prohibited");
    case 10:
        return ("destination host administratively prohibited");
    case 11:
        return ("network unreachable for TOS");
    case 12:
        return ("host unreachable for TOS");
    case 13:
        return ("communication administratively prohibited by filtering");
    case 14:
        return ("host recedence violation");
    case 15:
        return ("precedence cutoff in effect");
    default:
        sprintf(errbuf, "[unknown code %d]", code);
        return errbuf;
    }
}
