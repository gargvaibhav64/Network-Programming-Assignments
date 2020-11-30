#include "node.h"

#define SA struct sockaddr

int main(int argc, char *argv[])
{
    int p1= 47430;
    int p2= p1 + 1;
    printf("\np1= %d!\np2= %d\n", p1, p2);
    int n = atoi(argv[1]);
    N = n;
    int readfd, readconfd, wrtfd, wrtconfd;

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

    printf("Before fork!\n");

    // Child process initialization

    for (int i = 1; i < n; i++)
    {
        pid_t parent = getpid();
        pid_t pid = fork();
        if (pid > 0)
        {
            sleep(1);
            printf("Tets\n");
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
    printf("Child completed\n");
    // Child process ends

    int len;
    readconfd = accept(wrtfd, (SA *)&waddr[1], &len);

    // connect the client socket to server socket
    if (connect(readfd, (SA *)&addr[0], sizeof(addr[0])) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }

    struct buffer *buf = (struct buffer *)malloc(sizeof(struct buffer));
    bzero(buf, sizeof(struct buffer));

    buf->len = n;
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

    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
    {
        perror("Send error");
    }

    sz = n - n / 2;
    buf->start = 0;
    buf->end = sz / 2 - 1;
    buf->destNode = 0;
    buf->merged = 0;

    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
    {
        perror("Send error");
    }

    printf("Sent!\n");

    int cnt = 0;
    //  bzero(buf, 100);
    struct buffer *buf1 = (struct buffer *)malloc(sizeof(struct buffer));
    for (;;)
    {
        int l;
        if (recv(readfd, buf, sizeof(struct buffer), 0) < 0)
        {
            perror("Read error");
        }

        if (buf->destNode != 0)
        {
            // printf("Root recieved for dest= %d Src= %d start= %d end= %d\n", buf->destNode, buf->srcNode, buf->start, buf->end);
            if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
            {
                perror("Send error");
            }
        }
        else
        {
            
            if (buf->merged)
            {
                printf("Root recieved merged for dest= 0 Src= %d start= %d end= %d\n", buf->srcNode, buf->start, buf->end);
                // for(int i=0; i<n; i++){
                //     printf("%d", buf->arr[i]);
                // }
                cnt++;
                if (cnt != 2)
                {
                    memcpy(buf1, buf, sizeof(struct buffer));
                    continue;
                }

                cnt = 0;

                if (buf->len == n - n / 2 || buf->len == n / 2)
                {
                    merge(buf->arr, 0, n/2 -1, n-1);
                    // printf("Sorted array :\n");
                    // for(int i=0; i<n; i++)
                    //     printf("%d ", buf->arr[i]);
                    return 0; // Final list obtained
                }
                else
                {
                    merge(buf->arr, min(buf->start, buf1->start), min(buf->end, buf1->end), max(buf->end, buf1->end));
                    sz = buf->len + buf1->len;
                    buf->start = 0;
                    buf->end = sz - 1;
                    buf->destNode = 0;
                    buf->merged = 1;

                    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
                    {
                        perror("Send error");
                    }
                }
            }
            else
            {
                printf("Root recieved unmerged for dest= 0 Src= %d start= %d end= %d\n", buf->srcNode, buf->start, buf->end);
                // for(int i=0; i<n; i++){
                //     printf("%d", buf->arr[i]);
                // }
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
                    }
                }
                else
                {
                    buf->start = buf->len / 2;
                    buf->end = buf->len - 1;
                    buf->destNode = buf->start;
                    buf->merged = 0;

                    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
                    {
                        perror("Send error");
                    }

                    sz = n - n / 2;
                    buf->start = 0;
                    buf->end = buf->len / 2 - 1;
                    buf->destNode = 0;
                    buf->merged = 0;

                    if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
                    {
                        perror("Send error");
                    }
                }
            }
        }
    }

    // prnt after fork
    close(readfd);
    close(wrtfd);
}