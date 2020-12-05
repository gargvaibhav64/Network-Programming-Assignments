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
	while ((n%2) != 1)
	{
		cnt++;
		n= n/2;
	}
	return (int)pow(2.0, cnt);
}

void swap(int* xp, int* yp) 
{ 
    int temp = *xp; 
    *xp = *yp; 
    *yp = temp; 
} 
  
void selectionSort(int arr[], int n) 
{ 
    int i, j, min_idx; 
  
    for (i = 0; i < n - 1; i++) { 
  
        min_idx = i; 
        for (j = i + 1; j < n; j++) 
            if (arr[j] < arr[min_idx]) 
                min_idx = j; 
  
        swap(&arr[min_idx], &arr[i]); 
    } 
} 

void partSort(int arr[], int N, int a, int b) 
{ 

    int l = min(a, b); 
    int r = max(a, b); 
 
    int temp[r - l + 1]; 
    int j = 0; 
    for (int i = l; i <= r; i++) { 
        temp[j] = arr[i]; 
        j++; 
    } 
 
    selectionSort(temp, r - l + 1); 

    j = 0; 
    for (int i = l; i <= r; i++) { 
            arr[i] = temp[j]; 
            j++; 
    }   
}

int mergeUtil(int buf[], int ret[], int l1, int e1, int l2, int e2, int *retlen){
	int tarr[MAX];
	
	// printf("Merge util got l1= %d e1= %d l2= %d e2= %d: retlen= %d\n", l1, e1, l2, e2, *retlen);
	// for(int i=0; i<e1-l1+1; i++)
	// 	printf("[%d] ", buf[l1+i]);
	// printf("\n");
	// for(int i=0; i<e2-l2+1; i++)
	// 	printf("(%d) ", ret[i]);
	// printf("\n");
	for(int i=0; i< e1-l1 +1; i++){
		ret[e2+i+1]= buf[l1+i];
	}
	*retlen= *retlen + e1-l1+1;

	// for(int i=0; i<*retlen; i++)
	// 	printf("{%d} ", ret[i]);
	// printf("\n");
	merge2(ret, 0, e2, *retlen-1);
	// for(int i=0; i<*retlen; i++)
	// 	printf("%d ", ret[i]);
	// printf("\nend\n");
	
}

int merge2(int arr[], int l, int m, int r)
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
	int readfd, readconfd, wrtfd, wrtconfd;
	struct sockaddr_in node;
	struct sockaddr_in wnode;

	// Reading
	readfd = socket(AF_INET, SOCK_STREAM, 0);
	if (readfd == -1)
	{
		perror("socket creation failed...");
		exit(0);
	}
	bzero(&node, sizeof(node));
	node.sin_family = AF_INET;
	node.sin_addr.s_addr = inet_addr("127.0.0.1");
	node.sin_port = htons(port1);

	if (connect(readfd, (SA *)&node, sizeof(node)) != 0)
	{
		perror("failed connect");
		exit(0);
	}

	// Writing

	wrtfd = socket(AF_INET, SOCK_STREAM, 0);
	if (wrtfd == -1)
	{
		perror("socket creation failed...");
		exit(0);
	}
	bzero(&wnode, sizeof(wnode));

	wnode.sin_family = AF_INET;
	wnode.sin_addr.s_addr = htonl(INADDR_ANY);
	wnode.sin_port = htons(port2);

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

	kill(parent, SIGALRM);

	int len = sizeof(node);
	if((readconfd = accept(wrtfd, (SA *)&node, &len))<0){
		perror("accept error in child : ");
		exit(0);
	}

	struct buffer *buf = (struct buffer *)malloc(sizeof(struct buffer));

	int cnt = 0;
	int mergeSize= 0;
	struct buffer *buf1 = (struct buffer *)malloc(sizeof(struct buffer));
	int ret[MAX];
	int retcnt=0;

	int cflag=0;
	int backSize=0;

	for (;;)
	{
		if (recv(readfd, buf, sizeof(struct buffer), 0) < 0)
		{
			perror("Read error in child");
			exit(0);
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
				exit(0);
			}
		}
		else
		{
			cflag++;
			if(cflag==1){
				backSize= buf->len;
			}

			printf("Node-%d received data for Src= %d len= %d RPort = %d WPort = %d\n", 
				nodeNum, buf->srcNode,  buf->len, port1, port2);
				
			if (buf->merged)
			{

                // printf("Node-%d (backsize= %d) received merged for dest= %d Src= %d start= %d end= %d len= %d\n", nodeNum, backSize, buf->destNode, buf->srcNode, buf->start, buf->end, buf->len);
				
				mergeSize+= buf->len;
				
                // for(int i=0; i<N; i++){
                //     printf("%d", buf->arr[i]);
                // }

				cnt++;
				if (cnt < 2)
				{
					memcpy(buf1, buf, sizeof(struct buffer));
                    retcnt= buf->end- buf->start+ 1;
                    for(int i= 0; i<retcnt; i++)
                        ret[i]= buf->arr[buf->start+i];
					continue;
				}

				// cnt = 0;

				int n = getCount(nodeNum);

				if (mergeSize== backSize)
				{
					// merge(buf->arr, min(buf->start, buf1->start), min(buf->end, buf1->end), max(buf->end, buf1->end));
					// printf("Calling mergeutil nonRoot with\n");
					mergeUtil(buf->arr, ret, buf->start, buf->end, 0, retcnt-1, &retcnt);

                    for(int i=0; i<listlen; i++)
                        printf("%d ", ret[i]);

					printf("\n");

                    // printf("mergeutil ended nonRoot\n");
					for(int i= 0; i<backSize; i++){
						buf->arr[i + buf->start]= ret[i];
					}
                    // for(int i=0; i<N; i++){
                    //     printf("%d", buf->arr[i]);
                    // }
                    // printf("\n");

                    // memcpy(buf->arr, ret, sizeof(buf->arr));


					buf->start = min(buf->start, buf1->start);
					buf->end = max(buf->end, buf1->end);
					buf->destNode = nodeNum - n;
					buf->merged = 1;
					buf->srcNode= nodeNum;
					buf->len= buf->len+ buf1->len;

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
						exit(0);
					}
				}
				else
				{
					// if(cnt==2)
					// 	merge(buf->arr, min(buf->start, buf1->start), min(buf->end, buf1->end), max(buf->end, buf1->end), ret, &retcnt);
					// else{
					// 	mergeUtil(buf->arr, ret, buf->start, buf->end, 0, retcnt-1, &retcnt);
					// }
                    mergeUtil(buf->arr, ret, buf->start, buf->end, 0, retcnt-1, &retcnt);
                    // for(int i=0; i<listlen; i++)
                    //     printf("%d ", ret[i]);
                    // printf("\n");
                    // for(int i=0; i<retcnt; i++){
                    //     printf("%d", ret[i]);
                    // }
                    // printf("\n");

                    // memcpy(buf->arr, ret, sizeof(buf->arr));

					
					int sz = buf->len + buf1->len;
					buf->start = min(buf->start, buf1->start);
					buf->end = max(buf->end, buf1->end);
					buf->destNode = nodeNum;
					buf->merged = 1;
					buf->srcNode= nodeNum;
					buf->len= buf->len+ buf1->len;

					memcpy(buf1, buf, sizeof(struct buffer));

					// if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					// {
					// 	perror("Send error");
					// }
				}
			}
			else
			{
				// printf("Node-%d (backsize= %d) received unmerged for dest= %d, Src= %d start= %d end= %d len= %d listlen(%d)/N(%d)= %d\n", nodeNum, backSize, buf->destNode, buf->srcNode, buf->start, buf->end, buf->len, listlen, N, listlen/N);
                // for(int i=0; i<listlen; i++){
                //     printf("%d", buf->arr[i]);
                // }

				if (buf->len == max(1,listlen/N))
				{
					buf->start = buf->start;
					buf->end = buf->end;
					
					if ((nodeNum) % 2 == 0)
						buf->destNode = buf->srcNode;
					else
						buf->destNode = buf->srcNode;

					buf->merged = 1;
					buf->srcNode= nodeNum;

					partSort(buf->arr, listlen, buf->start, buf->end);

					// for(int i=0; i<listlen; i++){
					// 	printf("%d", buf->arr[i]);
					// }

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
						exit(0);
					}
				}
				else
				{
					int start = buf->start;
					buf->start = start + buf->len / 2;
					buf->end = buf->start + buf->len/2 - 1;
					int len= buf->len;
					buf->len= buf->len - buf->len/2;
					buf->destNode = nodeNum + buf->x;
					buf->merged = 0;
					buf->srcNode= nodeNum;
					buf->x /= 2;

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
						exit(0);
					}

					buf->len = len / 2;
					buf->start = start;
					buf->end = buf->start + len / 2 - 1;
					buf->destNode = buf->srcNode;
					buf->merged = 0;

					if (send(readconfd, buf, sizeof(struct buffer), 0) < 0)
					{
						perror("Send error");
						exit(0);
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
