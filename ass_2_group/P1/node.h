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

int newNode(int , int, pid_t, int);
int N;
int max(int, int);
int min(int, int);
int merge(int arr[], int l, int m, int r);