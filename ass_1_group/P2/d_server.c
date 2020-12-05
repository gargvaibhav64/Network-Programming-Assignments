#define MSGQ_PATH "/media/vaibhav/New Volume/Academics/3-1/IS F462/Network Programming/ass_1_group/P2"

#include <stdio.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHUNK_SIZE 50

#define CREATE_FILE 1
#define CP 2
#define ADD_CHUNK 3
#define STATUS 4
#define LIST_OF_D 5
#define MV 6
#define AVAILABLE 7
#define D_READY 8
#define RM 9

struct my_msgbuf
{
    long mtype;
    char mtext[200];
};

int main(){
    struct my_msgbuf buf;
    key_t key;
    int msqid;
    const char s[2] = " ";

    if ((key = ftok (MSGQ_PATH, 'B')) == -1)
    {
        perror ("Couldn't create key for specified filepath. Exiting!");
        exit (1);
    }

    if ((msqid = msgget (key, IPC_CREAT | 0644)) == -1)
    {
        perror ("Couldn't fetch/create message queue. Exiting!");
        exit (1);
    }

    printf("D Server PID : %d Online\n", getpid());

    while(1){
        if(msgrcv(msqid, &(buf.mtype), sizeof(buf), AVAILABLE, IPC_NOWAIT) >= 0){
            memset(buf.mtext, '\0', sizeof(char)*200);
            buf.mtype = D_READY;

            sprintf(buf.mtext, "%d ", getpid());

            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                perror("Could not reply to M server for status");

            if(msgrcv(msqid, &(buf.mtype), sizeof(buf), getpid(), 0) >= 0){

                char * chunkID;
                char *tmp;
                tmp = buf.mtext;
                chunkID = strsep(&tmp, " \t\n");

                // char *data;
                
                FILE *fptr;
                fptr = fopen(chunkID, "w");
                if(fptr != NULL){
                    // printf("%s", tmp);
                    fprintf(fptr, "%s", tmp);
                    fflush(fptr);

                } else {
                    printf("Couldn't create file\n");
                }

                fclose(fptr);
            }
        } // end create chunk if
        if(msgrcv(msqid, &(buf.mtype), sizeof(buf), getpid(), IPC_NOWAIT) >= 0){
            char *command;
            char *tmp;
            tmp = buf.mtext;

            char xyz[200];
            strcpy(xyz, buf.mtext);
            command = strsep(&tmp, " \t\n");

            if(strcmp(command, "COPY_CHUNK") == 0){
                char *old;
                char *newf;

                old = strsep(&tmp, " \t\n");
                newf = strsep(&tmp, " \t\n");

                FILE *fp1, *fp2;

                if ((fp1 = fopen(old,"r")) == NULL)    
                {    
                    printf("\nFile cannot be opened");
                }

                if ((fp2 = fopen(newf,"w")) == NULL)    
                {    
                    printf("\nFile cannot be opened");
                }
                
                int pos = 0;
                char ch;
                fseek(fp1, 0L, SEEK_END); // file pointer at end of file
                pos = ftell(fp1);

                fseek(fp1, 0L, SEEK_SET); // file pointer set at start
                while (pos--)
                {
                    ch = fgetc(fp1);  // copying file character by character
                    fputc(ch, fp2);
                }

                fclose(fp1);
                fclose(fp2);

            } else if (strcmp(command, "DELETE_CHUNK") == 0){
                char *old;

                old = strsep(&tmp, " \t\n");

                if (remove(old) == 0) 
                    printf("Deleted successfully"); 
                else
                    printf("Unable to delete the file");

                tmp = NULL;
                
            } else { // Command to be executed on D_Server
                int pip[2];
                if(pipe(pip)==-1){
                    perror("Pipe error");
                }
                
                char *c1;
                char *c0;
                char *output;
                char * filename = command;

                int chunk = atoi(filename);

                int status1;

                char comm[200];

                char * xyz2 = comm;

                strcpy(comm, tmp);

                c0 = strsep(&xyz2, " \n");

                strcat(comm, " ");
                strcat(comm, filename);

                // Work from Here
                // c1 : command to be executed. For example: wc
                // filename : chunk id, local directory filename 
                //             on which command will be executed
                pid_t pid;
                if((pid = fork()) == 0){
                    if (dup2(pip[1], STDOUT_FILENO) == -1){
                        perror("dup2 error");
                    }
                    // write(ipip[1], filename, strlen(output));
                    // if (dup2(ipip[0], STDIN_FILENO) == -1){
                    //     perror("dup2 error");
                    // }
                    execlp (c0, comm, (char *) 0);
                    // execlp("wc", "wc", "t1.txt", char/.....);
                }

                else {
                    waitpid(pid, &status1, 0);
                    char * op_buff= (char *)malloc(sizeof(char)* 100);
                    int sz= read(pip[0], op_buff, 100);
                    output = (char *)malloc((sz + 1) * sizeof(char));
                    memcpy(output, &op_buff[0], sz);
                    output[sz] = '\0';
                    free(op_buff);
                    close(pip[1]);
                    close(pip[0]);
                    printf("Executed command %s\n", c1);
                    fflush(0);
                }

                // Do not touch after this. Store output in char * output declared above

                strcpy(buf.mtext, output);
                buf.mtype = chunk + 15;

                if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                    perror("Could not send result to client");
                
            }
        }
    }
}