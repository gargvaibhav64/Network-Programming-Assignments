#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#define MAX_CLIENTS 100

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

typedef struct {
     int sockfd;
     int block_num;
     in_port_t port;
     struct sockaddr_in client_sock;
     int clilen;
     int filefd;
     int block_num;
     int prev_data_size;
} clients;

tftp_message t;

/* base directory */
char *base_directory;

clients client[MAX_CLIENTS]; 

int cl_num;

int tftp_recv_req(int s, tftp_message *m, struct sockaddr_in *sock, socklen_t *slen)
{
    int c;

    if ((c = recvfrom(s, m, sizeof(*m), 0, (struct sockaddr *) sock, slen)) < 0
          && errno != EAGAIN) {
          perror("server: recvfrom()");
    }
    return c;
}

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

    fd_set rset;
    FD_ZERO(&rset);

    cl_num = 0;

    if (bind(sockfd, (struct sockaddr *) &serv, sizeof(serv)) == -1) {
        perror("server: bind()");
        close(sockfd);
        exit(1);
    }

    printf("tftp server: listening on %d\n", ntohs(serv.sin_port));

    FD_SET(sockfd, &rset);
    int maxfdp1 = sockfd;

    while(1){
        struct sockaddr_in client_sock;
        socklen_t slen = sizeof(client_sock);

        int msglen;
        tftp_message msg;

        select(maxfdp1, &rset, NULL, NULL, 0);

        if (FD_ISSET(sockfd, &rset)){

            if ((msglen = tftp_recv_req(sockfd, &msg, &client_sock, &slen)) < 0) {
                continue;
            }

            if (msglen < 4) { 
                printf("%s.%u: request with invalid size received\n",
                        inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port));
                // tftp_send_error(s, 0, "invalid request size", &client_sock, slen);
                continue;
            }

            int opcode = ntohs(msg.opcode);

            if (opcode == RRQ) {
                /* spawn a child process to handle the request */
                // if (fork() == 0) {
                tftp_handle_request(&msg, msglen, &client_sock, slen);
                        // exit(0);
                // }

                client[cl_num].client_sock = client_sock;
                client[cl_num].clilen = slen;

                int clsockfd;

                if ((clsockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
                    perror("server: socket()");
                    exit(1);
                }

                client[cl_num].sockfd = clsockfd;

                char *filename, *end;
                filename = msg.msg.read.filename_and_mode;

                end = &filename[msglen - 2 - 1]; // 2 bytes for opcode ;

                if (*end != '\0') {
                    // printf("%s.%u: invalid filename or mode\n",
                    //         inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
                    // tftp_send_error(s, 0, "invalid filename or mode", client_sock, slen);

                    // filename exceeding max length ; Last character not NULL
                    continue;
                }

                char * mode_s = strchr(filename, '\0') + 1; // NULL byte after filename, then Mode starts

                if (mode_s > end) {
                    // printf("%s.%u: transfer mode not specified\n",
                    //         inet_ntoa(client_sock->sin_addr), ntohs(client_sock->sin_port));
                    // tftp_send_error(s, 0, "transfer mode not specified", client_sock, slen);

                    // mode name exceeding max length ;
                    continue;
                }

                opcode = ntohs(msg.opcode);
                int fd = fopen(filename, "r"); // Handling only Read requests

                if (fd == NULL) {
                    perror("server: fopen()"); // Error in opening file
                    // tftp_send_error(s, errno, strerror(errno), client_sock, slen);
                    
                    continue;
                }

                if(strcasecmp(mode_s, "octet") != 0){
                    // Only octet mode supported
                    continue;
                }

                

            }

            else {
                printf("%s.%u: invalid request received: opcode \n", 
                        inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port),
                        opcode);
                // tftp_send_error(s, 0, "invalid opcode", &client_sock, slen);
            }
        }

    }

}