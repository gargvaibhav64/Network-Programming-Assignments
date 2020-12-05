#include "node.h"

#define SA struct sockaddr

int gotalarm;

void sig_alrm(int signo)
{
	gotalarm = 1;	/* set flag to note that alarm occurred */
	return;			/* and interrupt the recvfrom() */
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        printf("Usage: $ ./server 4 \n");
        exit(1);
    }

    srand(time(NULL));
    int p1= 30000 + rand()%20000;
    int p2= p1 + 1;
    // printf("\np1= %d!\np2= %d\n", p1, p2);
    int n = atoi(argv[1]);
    N = n;
    int readfd, readconfd, wrtfd, wrtconfd;

    signal(SIGALRM, sig_alrm);

    // Reading

    struct sockaddr_in addr[n + 1];

    if ((readfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation error");
        exit(0);
    }
    bzero(&addr[0], sizeof(addr[0]));

    addr[0].sin_family = AF_INET;
    addr[0].sin_addr.s_addr = inet_addr("127.0.0.1");
    addr[0].sin_port = htons(p1);

    // Writing

    struct sockaddr_in waddr[n + 1];

    if ((wrtfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation error");
        exit(0);
    }
    bzero(&waddr[0], sizeof(waddr[0]));

    waddr[0].sin_family = AF_INET;
    waddr[0].sin_addr.s_addr = htonl(INADDR_ANY);
    waddr[0].sin_port = htons(p2);

    if ((bind(wrtfd, (SA *)&waddr[0], sizeof(waddr[0]))) != 0)
    {
        perror("Socket bind error");
        exit(0);
    }

    if ((listen(wrtfd, 5)) != 0)
    {
        perror("Listen fd error");
        exit(0);
    }

    // printf("Before fork!\n");

    // Child process initialization

    for (int i = 1; i < n; i++)
    {
        pid_t parent = getpid();
        pid_t pid = fork();
        if (pid > 0)
        {
            while(gotalarm == 0){

            }
            gotalarm = 0;
            // printf("Tets\n");
        }
        else
        {
            int port1 = p1 + i; // read port
            int port2 = port1 + 1; // write port
            if (i == n - 1)
                port2 = p1;
            newNode(port1, port2, parent, i);
            exit(1);
        }
    }
    // printf("Child completed\n");
    // Child process ends

    int len;
    

    if ((readconfd = accept(wrtfd, (SA *)&waddr[0], &len)) < 0)
    {
        // printf("connection with the server failed...\n");
        perror("Server : accept() ");
        exit(0);
    }

    // connect the client socket to server socket
    if (connect(readfd, (SA *)&addr[0], sizeof(addr[0])) != 0)
    {
        perror("connection with the server failed...");
        exit(0);
    }

    struct buffer *buf = (struct buffer *)malloc(sizeof(struct buffer));
    bzero(buf, sizeof(struct buffer));

    buf->len = n/2;
    buf->srcNode = 0;

    printf("Enter %d integers: \n", n);

    for (int i = 0; i < n; i++)
        scanf("%d", &buf->arr[i]);

    int arr0 = buf->arr[0];

    int sz = n;

    buf->start = sz / 2;
    buf->end = sz - 1;
    buf->destNode = buf->start;
    buf->merged = 0;
    buf->srcNode= 0;

    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
    {
        perror("Send error");
        exit(0);
    }

    sz = n - n / 2;
    buf->start = 0;
    buf->end = n / 2 - 1;
    buf->destNode = 0;
    buf->merged = 0;

    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
    {
        perror("Send error");
        exit(0);
    }

    // printf("Sent!\n");

    int cnt = 0;
    int mergeSize=0;
    //  bzero(buf, 100);
    struct buffer *buf1 = (struct buffer *)malloc(sizeof(struct buffer));

	int ret[MAX];
	int retcnt=0;

    for (;;)
    {
        int l;
        if (recv(readfd, buf, sizeof(struct buffer), 0) < 0)
        {
            perror("Read error");
            exit(0);
        }

        if (buf->destNode != 0)
        {
            // printf("Root recieved for dest= %d Src= %d start= %d end= %d\n", buf->destNode, buf->srcNode, buf->start, buf->end);
            if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
            {
                perror("Send error");
                exit(0);
            }
        }
        else
        {            
            printf("Node-0 recieved data for Src= %d len= %d RPort = %d WPort = %d\n", 
				buf->srcNode,  buf->len, p1, p2);

            if (buf->merged)
            {
                mergeSize+= buf->len;
                // printf("Root recieved merged for dest= 0 Src= %d start= %d end= %d len= %d\n", buf->srcNode, buf->start, buf->end, buf->len);
                // for(int i=0; i<n; i++){
                //     printf("%d", buf->arr[i]);
                // }
                cnt++;
                if (cnt < 2)
                {
                    memcpy(buf1, buf, sizeof(struct buffer));
                    retcnt= buf->len;
                    // printf("Setting retcnt in root= %d\n", retcnt);
                    for(int i= 0; i<retcnt; i++){
                        ret[i]= buf->arr[buf->start+i];
                        // printf("%d ", ret[i]);
                    }
                    // printf("retend\n");
                    continue;
                }

                // cnt = 0;

                if (mergeSize == n)
                {
					mergeUtil(buf->arr, ret, buf->start, buf->end, 0, retcnt-1, &retcnt);
					for(int i= 0; i<n+1; i++){
						buf->arr[i]= ret[i];
					}
                    printf("Sorted array :\n");
                    for(int i=0; i<n; i++)
                        printf("%d ", ret[i]);

                    
                    return 0; // Final list obtained
                }
                else
                {

                    mergeUtil(buf->arr, ret, buf->start, buf->end, 0, retcnt-1, &retcnt);

                    sz = buf->len + buf1->len;
                    // printf("buf->src= %d buf1->src= %d buflen= %d buf1len= %d\n", buf->srcNode, buf1->srcNode, buf->len, buf1->len);
                    buf->start = 0;
                    buf->end = sz - 1;
                    buf->destNode = 0;
                    buf->merged = 1;
                    buf->srcNode= 0;
                    buf->len= sz;
                
                }
            }
            else
            {
                // printf("Root recieved unmerged for dest= 0 Src= %d start= %d end= %d len= %d\n", buf->srcNode, buf->start, buf->end, buf->len);

                buf->srcNode= 0;
                if (buf->len == 1)
                {
                    sz = 1;
                    buf->start = 0;
                    buf->end = 0;
                    buf->destNode = 0;
                    buf->merged = 1;

                    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
                    {
                        perror("Send error");
                        exit(0);
                    }
                }
                else
                {
                    buf->start = buf->len / 2;
                    buf->end = buf->len - 1;
                    buf->destNode = buf->start;
                    buf->merged = 0;
                    buf->len= buf->len/2;

                    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
                    {
                        perror("Send error");
                        exit(0);
                    }

                    sz = n - n / 2;
                    buf->start = 0;
                    buf->end = buf->len - 1;
                    buf->destNode = 0;
                    buf->merged = 0;

                    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
                    {
                        perror("Send error");
                        exit(0);
                    }
                }
            }
        }
    }

    // prnt after fork
    close(readfd);
    close(wrtfd);
}