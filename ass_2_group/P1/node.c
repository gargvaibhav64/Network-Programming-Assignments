#include "node.h"

#define SA struct sockaddr

int max(int a, int b)
{
	return a > b ? a : b;
}

int min(int a, int b)
{
	return a < b ? a : b;
}

int getCount(int n)
{
	double cnt = 0;
	while (n % 2 != 0)
	{
		cnt++;
		n = n / 2;
	}
	return (int)pow(2.0, cnt);
}

int merge(int arr[], int l, int m, int r)
{
	int i, j, k;
	int n1 = m - l + 1;
	int n2 = r - m;

	int L[n1], R[n2];

	for (i = 0; i < n1; i++)
		L[i] = arr[l + i];
	for (j = 0; j < n2; j++)
		R[j] = arr[m + 1 + j];

	i = 0;
	j = 0;
	k = l;
	while (i < n1 && j < n2)
	{
		if (L[i] <= R[j])
		{
			arr[k] = L[i];
			i++;
		}
		else
		{
			arr[k] = R[j];
			j++;
		}
		k++;
	}

	while (i < n1)
	{
		arr[k] = L[i];
		i++;
		k++;
	}

	while (j < n2)
	{
		arr[k] = R[j];
		j++;
		k++;
	}
}

int newNode(int port1, int port2, pid_t parent, int nodeNum)
{
	printf("****************\n");
	printf("Nodenum= %d\n", nodeNum);
	printf("\nPort1= %d Port2= %d\n", port1, port2);

	int readfd, readconfd, wrtfd, wrtconfd;
	struct sockaddr_in node;
	struct sockaddr_in wnode;

	// Reading
	readfd = socket(AF_INET, SOCK_STREAM, 0);
	if (readfd == -1)
	{
		printf("socket creation failed...\n");
		exit(0);
	}
	bzero(&node, sizeof(node));
	node.sin_family = AF_INET;
	node.sin_addr.s_addr = inet_addr("127.0.0.1");
	node.sin_port = htons(port1);

	// connect the client socket to server socket
	printf("Connecting port= %d\n", port1);
	if (connect(readfd, (SA *)&node, sizeof(node)) != 0)
	{
		perror("failed connect");
		printf("connection with the server failed...\n");
		exit(0);
	}

	// Writing

	wrtfd = socket(AF_INET, SOCK_STREAM, 0);
	if (wrtfd == -1)
	{
		printf("socket creation failed...\n");
		exit(0);
	}
	bzero(&wnode, sizeof(wnode));

	wnode.sin_family = AF_INET;
	wnode.sin_addr.s_addr = htonl(INADDR_ANY);
	wnode.sin_port = htons(port2);

	printf("Before binding wrt\n");

	if ((bind(wrtfd, (SA *)&wnode, sizeof(wnode))) != 0)
	{
		perror("Socket bind error");
		exit(0);
	}

	if ((listen(wrtfd, 5)) != 0)
	{
		perror("Listen fd error");
		exit(0);
	}

	// kill(parent, SIGCHLD);

	printf("Accepting port= %d\n", port2);

	int len;
	if((readconfd = accept(wrtfd, (SA *)&node, &len))<0){
		perror("accept error in child : ");
	}
	printf("Done\n");
	// close the socket

	struct buffer *buf = (struct buffer *)malloc(sizeof(struct buffer));

	int cnt = 0;
	struct buffer *buf1 = (struct buffer *)malloc(sizeof(struct buffer));

	for (;;)
	{
		if (recv(readfd, buf, sizeof(struct buffer), 0) < 0)
		{
			perror("Read error in child");
		}
		// printf("Node-%d read. Dest= %d Src= %d Length= %d start= %d end= %d\n", nodeNum, buf->destNode, buf->srcNode, buf->len, buf->start, buf->end);
		// for(int i=0; i<buf->len; i++)
		//     printf("%d ", buf->arr[i]);
		// printf("\n");
		if (buf->destNode != nodeNum)
		{
			// printf("Node-%d recieved for dest= %d\n", nodeNum, buf->destNode);
			if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
			{
				perror("Send error test");
			}
		}
		else
		{
			if (buf->merged)
			{

				printf("Node-%d recieved merged for dest= %d, Src= %d start= %d end= %d\n", nodeNum, buf->destNode, buf->srcNode, buf->start, buf->end);
                // for(int i=0; i<N; i++){
                //     printf("%d", buf->arr[i]);
                // }

				cnt++;
				if (cnt != 2)
				{
					memcpy(buf1, buf, sizeof(struct buffer));
					continue;
				}

				cnt = 0;

				int n = getCount(nodeNum);

				if (buf->len == n - n / 2 || buf->len == n / 2)
				{
					merge(buf->arr, min(buf->start, buf1->start), min(buf->end, buf1->end), max(buf->end, buf1->end));

					buf->start = min(buf->start, buf1->start);
					buf->end = max(buf->end, buf1->end);
					buf->destNode = nodeNum - n;
					buf->merged = 1;

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
					}
				}
				else
				{
					merge(buf->arr, min(buf->start, buf1->start), min(buf->end, buf1->end), max(buf->end, buf1->end));
					int sz = buf->len + buf1->len;
					buf->start = min(buf->start, buf1->start);
					buf->end = max(buf->end, buf1->end);
					buf->destNode = nodeNum+1;
					buf->merged = 1;

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
					}
				}
			}
			else
			{

				printf("Node-%d recieved unmerged for dest= %d, Src= %d start= %d end= %d\n", nodeNum, buf->destNode, buf->srcNode, buf->start, buf->end);
                // for(int i=0; i<N; i++){
                //     printf("%d", buf->arr[i]);
                // }

				if (buf->len == 1)
				{
					buf->start = buf->start;
					buf->end = buf->end;
					if ((nodeNum) % 2 == 0)
						buf->destNode = buf->start;
					else
						buf->destNode = buf->start - 1;

					buf->merged = 1;

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
					}
				}
				else
				{
					int start = buf->start;
					buf->start = start + buf->len / 2;
					buf->end = buf->start + buf->len - 1;
					buf->destNode = buf->start;
					buf->merged = 0;

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
					}

					buf->len = buf->len - buf->len / 2;
					buf->start = start;
					buf->end = buf->start + buf->len / 2 - 1;
					buf->destNode = start;
					buf->merged = 0;

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
					}
				}
			}
		}

		// if (nodeNum != 7 && (l = send(readconfd, buf, sizeof(struct buffer), 0)))
		// 	printf("Node-%d wrote: %d bytes\n", nodeNum, l);
	}

	close(readfd);
	close(wrtfd);

	return 0;
}
