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

ssize_t tftp_send_data(int s, uint16_t block_number, uint8_t *data,
                       ssize_t dlen, struct sockaddr_in *sock, socklen_t slen) {
    tftp_message m;
    ssize_t c;

    m.opcode = htons(DATA);
    m.msg.data.block_number = htons(block_number);
    memcpy(m.msg.data.data, data, dlen);

    if ((c = sendto(s, &m, 4 + dlen, 0,
                    (struct sockaddr *) sock, slen)) < 0) {
        perror("server: sendto()");
    }

    return c;
}

int tftp_recv(int s, tftp_message *m, struct sockaddr_in *sock, socklen_t *slen) {
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

            if ((msglen = tftp_recv(sockfd, &msg, &client_sock, &slen)) < 0) {
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

                // tftp_handle_request(&msg, msglen, &client_sock, slen);

                int clsockfd;

                if ((clsockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
                    perror("server: socket()");
                    exit(1);
                }

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

                FD_SET(clsockfd, &rset);
                maxfdp1 = max(maxfdp1, clsockfd) + 1;

                client[cl_num].block_num = 0;
                client[cl_num].client_sock = client_sock;
                client[cl_num].clilen = slen;
                client[cl_num].sockfd = clsockfd;
                client[cl_num].filefd = fd;
                client[cl_num].prev_data_size = -1;
                client[cl_num].port = 0;

                cl_num++;
            }

            else {
                printf("%s.%u: invalid request received: opcode \n", 
                        inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port),
                        opcode);
                // tftp_send_error(s, 0, "invalid opcode", &client_sock, slen);
            }
        } 
        else {
            for(int i = 0 ; i < cl_num ; i++){
                if((client[i].filefd != -1 && FD_ISSET(client[i].sockfd, &rset)) 
                        || client[i].prev_data_size == -1) {

                    if(client[i].prev_data_size != -1){

                        if ((msglen = tftp_recv(client[i].sockfd, &msg, &client[i].client_sock, &client[i].clilen)) < 0) {
                            continue;
                        }

                        if (msglen < 4) { 
                            printf("%s.%u: request with invalid size received\n",
                                    inet_ntoa(client[i].client_sock.sin_addr), ntohs(client[i].client_sock.sin_port));
                            // tftp_send_error(s, 0, "invalid request size", &client_sock, slen);
                            continue;
                        }

                        if (ntohs(msg.opcode) == ERROR)  {
                            printf("%s.%u: error message received: %u %s\n",
                                    inet_ntoa(client[i].client_sock.sin_addr), ntohs(client[i].client_sock.sin_port),
                                    ntohs(msg.msg.error.error_code), msg.msg.error.error_string);
                            
                            
                            // Close connection
                        }

                        if (ntohs(m.opcode) != ACK)  {
                            printf("%s.%u: invalid message during transfer received\n",
                                inet_ntoa(client[i].client_sock.sin_addr), ntohs(client[i].client_sock.sin_port));
                            // tftp_send_error(s, 0, "invalid message during transfer", client_sock, slen);
                                
                            // Close connection
                        }

                        if (ntohs(msg.msg.ack.block_number) != client[i].block_num) { // the ack number is too high
                            printf("%s.%u: invalid ack number received\n", 
                                inet_ntoa(client[i].client_sock.sin_addr), ntohs(client[i].client_sock.sin_port));
                            // tftp_send_error(s, 0, "invalid ack number", client_sock, slen);

                            // Close connection
                        }

                    }
                    
                    char data[512];

                    int dlen;

                    dlen = fread(data, 1, sizeof(data), client[i].filefd);

                    if(dlen < 512){
                        
                    }
                    
                    int c;

                    c = tftp_send_data(client[i].sockfd, client[i].block_num, &data, dlen, 
                                            &client[i].client_sock, client[i].clilen);

                    if (c < 0) {
                        printf("%s.%u: transfer killed\n",
                            inet_ntoa(client[i].client_sock.sin_addr), ntohs(client[i].client_sock.sin_port));

                        // Close connection
                    }
                }
            }
        }

    }

}