#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char *argv[]) {

    if(argc < 2){
        printf("Usage: $ ./tftp 8069");
        exit(1);
    }

    int port = atoi(argv[1]);

    

}