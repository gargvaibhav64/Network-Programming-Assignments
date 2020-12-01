#include "trace.h"

int datalen = sizeof(struct rec); /* defaults */
int max_ttl = 30;
int nprobes = 3;
u_short dport = 32768 + 666;

int gotalarm;

void sig_alrm(int signo)
{
    gotalarm = 1; /* set flag to note that alarm occurred */
    return;       /* and interrupt the recvfrom() */
}

int main(int argc, char *argv[])
{
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
    pr->sarecv = calloc(1, ai->ai_addrlen);
    pr->salast = calloc(1, ai->ai_addrlen);
    pr->sabind = calloc(1, ai->ai_addrlen);
    pr->salen = ai->ai_addrlen;

    traceloop();

    return 0;
}

void * ttlUtil(int ttl)
{
    struct sockaddr_in *raw_serv;
    setsockopt(sendfd, pr->ttllevel, pr->ttloptname, &ttl, sizeof(int));
    bzero(pr->salast, pr->salen);
    printf("%2d ", ttl);
    fflush(stdout);
    for (probe = 0; probe < nprobes; probe++)
    {
        rec = (struct rec *)sendbuf;
        rec->seq = ++seq;
        rec->ttl = ttl;
        gettimeofday(&rec->tval, NULL);

        raw_serv = (struct sockaddr_in *)pr->sasend;
        raw_serv->sin_port = htons(dport + seq);
        pr->sasend = (struct sockaddr *)raw_serv;

        sendto(sendfd, sendbuf, datalen, 0, pr->sasend, pr->salen);

        if ((code = (*pr->recv)(seq, &tvrecv)) == -3)
        {
            printf(" *"); /* timeout, no reply */
        }
        else
        {
            char str[NI_MAXHOST];
            if (sock_cmp_addr(pr->sarecv, pr->salast, pr->salen) != 0)
            {
                if (getnameinfo(pr->sarecv, pr->salen, str, sizeof(str), NULL, 0, 0) == 0)
                    printf(" %s (%s)", str, sock_ntop_host(pr->sarecv, pr->salen));
                else
                    printf(" %s", sock_ntop_host(pr->sarecv, pr->salen));

                memcpy(pr->salast, pr->sarecv, pr->salen);
            }

            tv_sub(&tvrecv, &rec->tval);
            rtt = tvrecv.tv_sec * 1000.0 + tvrecv.tv_usec / 1000.0;
            printf(" %.3f ms", rtt);
            if (code == -1) /* port unreachable; at destination */
                done++;
            else if (code >= 0)
                printf(" (ICMP %s)", (*pr->icmpcode)(code));
        }

        fflush(stdout);
    }
    printf("\n");
}

void traceloop(void)
{
    int seq, code, done;

    double rtt;
    struct rec *rec;

    struct timeval tvrecv;

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
    seq = 0;
    done = 0;

    pthread_t pt[max_ttl];

    for (ttl = 1; ttl <= max_ttl && done == 0; ttl++)
    {
        pthread_create(&pt[ttl-1], NULL, ttlUtil, ttl);
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
