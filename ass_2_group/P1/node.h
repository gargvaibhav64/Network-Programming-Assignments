#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define MAX 100

struct buffer {
    int destNode;
    int srcNode;
    int len;
    int start;
    int end;
    int arr[MAX];
    int merged;
};

int N;
int listlen;

int newNode(int , int, pid_t, int);
int max(int, int);
int min(int, int);
int merge(int arr[], int l, int m, int r, int ret[], int *retlen);
int merge2(int arr[], int l, int m, int r);
int mergeUtil(int buf[], int ret[], int l1, int e1, int l2, int e2, int *retlen);
void partSort(int arr[], int N, int a, int b);