#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

enum opcode {
     RRQ=1,
     DATA,
     ACK,
     ERROR
};

/* tftp message structure */
typedef union {

    struct {
        /* RRQ */
        char filename_and_mode[514];
    } read;     

    struct {
        /* DATA */
        short block_number;
        char data[512];
    } data;

    struct {
        /* ACK */             
        short block_number;
    } ack;

    struct {
        /* ERROR */     
        short error_code;
        char error_string[512];
    } error;

} message;

typedef struct {
    short opcode;
    message msg;
} tftp_message;

tftp_message t;

/* base directory */
char *base_directory;

int main(int argc, char *argv[]) {

    if(argc < 2){
        printf("Usage: $ ./tftp 8069");
        exit(1);
    }

    int sockfd;

    struct sockaddr_in serv;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("server: socket() error");
        exit(1);
    }

    int port = atoi(argv[1]);
    port = htons(port);

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = port;

    if (bind(sockfd, (struct sockaddr *) &serv, sizeof(serv)) == -1) {
        perror("server: bind()");
        close(sockfd);
        exit(1);
    }

    printf("tftp server: listening on %d\n", ntohs(serv.sin_port));

    while(1){




    }


}