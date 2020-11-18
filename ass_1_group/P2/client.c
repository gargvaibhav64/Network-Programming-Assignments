#define MSGQ_PATH "/mnt/e/Academics/3-1/IS F462/ass_1_group/P2"

#include <stdio.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 50 // Should be less than 150

#define CREATE_FILE 1
#define CP 2
#define ADD_CHUNK 3
#define STATUS 4
#define LIST_OF_D 5
#define MV 6
#define AVAILABLE 7
#define D_READY 8
#define RM 9
#define EXEC_COMMAND 10
#define D_FOR_EXEC 11

struct my_msgbuf
{
    long mtype;
    char mtext[200];
};

int main(){
    struct my_msgbuf buf;
    key_t key;
    int msqid = 0;

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

    printf("Client Server Online\n");

    char command_input[400];

    printf("Enter command (ADD_FILE / CP / RM / MV / EXEC / HELP)\n");
    fgets(command_input, 400, stdin);

    while (strcmp(command_input, "quit\n") != 0) {
        printf("%s", command_input);
        char *tmp;
        tmp = command_input;
        char *command;
        int i = 0;
        const char s[2] = " ";

        command = strsep(&tmp, " \t\n");
        // printf("%s\n", command);

        if(strcmp("ADD_FILE", command) == 0){
            char *file_name;
            file_name = strsep(&tmp, " \t\n");

            printf("%s", file_name);

            FILE *fptr;
            fptr = fopen(file_name, "r");

            if(fptr == NULL){
                perror("File Doesn't exist on Disk / Couldn't open File");
            } else {
                buf.mtype = CREATE_FILE;
                memset(buf.mtext, '\0', sizeof(char)*200);
                strcpy(buf.mtext, file_name);

                if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                    perror("Could not send create file message");
            
                if(msgrcv(msqid, &(buf.mtype), sizeof(buf), STATUS, 0) >= 0){
                    if(strcmp(buf.mtext, "SUCCESS") == 0){
                        printf("File %s added to metadata\n", file_name);

                        unsigned char buffer[CHUNK_SIZE + 1];
                        size_t bytesRead = 0;

                        int num_of_chunks_in_file = 0;

                        // For all chunks of file
                        // 1. Send ADD_CHUNK msg to M
                        // 2. Decode 4 integers from message received
                        // 3. Send CHUNK_CREATE message to the integers
                        // 4. If all succeed, print ADD CHUNK CHUNK_ID message

                        while ((bytesRead = fread(buffer, 1, sizeof(buffer), fptr)) > 0){
                            num_of_chunks_in_file++;
                            buffer[bytesRead + 1] = '\0';
                            memset(buf.mtext, '\0', sizeof(char)*200);
                            strcpy(buf.mtext, file_name);
                            buf.mtype = ADD_CHUNK;

                            // printf("Bytes read in chunk %d\n", bytesRead);
                            // printf("%s", buffer);

                            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                                perror("Could not send add_chunk message");

                            if(msgrcv(msqid, &(buf.mtype), sizeof(buf), LIST_OF_D, 0) >= 0){
                                tmp = buf.mtext;
                                char *temp;
                                int d1, d2, d3;
                                int chunk = 0;

                                temp = strsep(&tmp, " \t\n");
                                d1 = atoi(temp);
                                temp = strsep(&tmp, " \t\n");
                                d2 = atoi(temp);
                                temp = strsep(&tmp, " \t\n");
                                d3 = atoi(temp);                             
                                temp = strsep(&tmp, " \t\n");
                                chunk = atoi(temp);

                                sprintf(buf.mtext, "%d ", chunk);
                                strcat(buf.mtext, buffer);

                                buf.mtype = d1;
                                if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                                    perror("Could not send chunk to D server");

                                buf.mtype = d2;
                                if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                                    perror("Could not send chunk to D server");

                                buf.mtype = d3;
                                if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                                    perror("Could not send chunk to D server");

                                printf("Added chunk no. %d of file %s, ABSOLUTE CHUNK ID: %d\n", num_of_chunks_in_file, file_name, chunk);

                            }
                        } // end-while for data reading               
                        
                    } else {
                        perror("Error in File Metadata in M_Server");
                    }
                    
                } // endif for signal received check
                fclose(fptr);

            } // endif for fptr == NULL check

        } // end if for command type check

        else if(strcmp("MV", command) == 0){
            char *file_name_old;
            file_name_old = strsep(&tmp, " \t\n");
            char *file_name_new;
            file_name_new = strsep(&tmp, " \t\n");

            strcpy(buf.mtext, file_name_old);
            strcat(buf.mtext, " ");
            strcat(buf.mtext, file_name_new);

            buf.mtype = MV;

            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                perror("Could not send move file message");

            if(msgrcv(msqid, &(buf.mtype), sizeof(buf), STATUS, 0) >= 0){
                if(strcmp(buf.mtext, "SUCCESS") == 0){
                    printf("File Successfully Moved\n");
                }
                else {
                    printf("Error in Moving File\n");
                }
            }         
        }

        else if(strcmp("CP", command) == 0){
            char *file_name_old;
            file_name_old = strsep(&tmp, " \t\n");
            char *file_name_new;
            file_name_new = strsep(&tmp, " \t\n");

            strcpy(buf.mtext, file_name_old);
            strcat(buf.mtext, " ");
            strcat(buf.mtext, file_name_new);

            buf.mtype = CP;

            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                perror("Could not send copy file message");

            if(msgrcv(msqid, &(buf.mtype), sizeof(buf), STATUS, 0) >= 0){
                if(strcmp(buf.mtext, "SUCCESS") == 0){
                    printf("File Successfully Copied\n");
                }
                else {
                    printf("Error in copying File\n");
                }
            }

        } 
        
        else if(strcmp("RM", command) == 0){

            char *file_name_old;
            file_name_old = strsep(&tmp, " \t\n");
            strcpy(buf.mtext, file_name_old);
            buf.mtype = RM;

            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                perror("Could not send remove file message");    

            if (msgrcv(msqid, &(buf.mtype), sizeof(buf), STATUS, 0) >= 0){
                if(strcmp(buf.mtext, "SUCCESS") == 0){
                    printf("File Successfully Deleted\n");
                }
                else {
                    printf("Error in Deleting File\n");
                }
            }
        } 
        
        else if(strcmp("EXEC", command) == 0){
            char *process, *filename; 
            process = strsep(&tmp, " \t\n"); // Discard keyword "ON"
            filename = strsep(&tmp, " \t\n");
            char com[200];

            process = strsep(&tmp, "\n");
            strcpy(com, process);
            
            buf.mtype = EXEC_COMMAND;
            memset(buf.mtext, '\0', sizeof(char)*200);
            strcpy(buf.mtext, filename);

            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                perror("Could not send find file message");


            char * stat;
            char * num_chunk;
        
            if(msgrcv(msqid, &(buf.mtype), sizeof(buf), STATUS, 0) >= 0){
                tmp = buf.mtext;
                stat = strsep(&tmp, " \t\n");
                num_chunk = strsep(&tmp, " \t\n");

                int loop = atoi(num_chunk);
                if(strcmp(stat, "SUCCESS") == 0){
                    while(loop--){

                        if(msgrcv(msqid, &(buf.mtype), sizeof(buf), D_FOR_EXEC, 0) >= 0){
                            tmp = buf.mtext;
                            char *temp;
                            int d1, d2, d3;
                            int chunk = 0;

                            temp = strsep(&tmp, " \t\n");
                            d1 = atoi(temp);
                            temp = strsep(&tmp, " \t\n");
                            d2 = atoi(temp);
                            temp = strsep(&tmp, " \t\n");
                            d3 = atoi(temp);                             
                            temp = strsep(&tmp, " \t\n");
                            chunk = atoi(temp);

                            sprintf(buf.mtext, "%d ", chunk);
                            strcat(buf.mtext, com);

                            buf.mtype = d1;
                            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                                perror("Could not send chunk to D server");

                            buf.mtype = d2;
                            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                                perror("Could not send chunk to D server");

                            buf.mtype = d3;
                            if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
                                perror("Could not send chunk to D server");

                            if(msgrcv(msqid, &(buf.mtype), sizeof(buf), chunk + 15, 0) >= 0){
                                printf("Output of command received from 1st D server"
                                    "for absolute chunk ID: %d is", chunk);

                                printf("%s\n", buf.mtext);
                            }

                            if(msgrcv(msqid, &(buf.mtype), sizeof(buf), chunk + 15, 0) >= 0){
                                printf("Output of command received from 2nd D server"
                                    "for absolute chunk ID: %d is", chunk);

                                printf("%s\n", buf.mtext);
                            }

                            if(msgrcv(msqid, &(buf.mtype), sizeof(buf), chunk + 15, 0) >= 0){
                                printf("Output of command received from 3rd D server"
                                    "for absolute chunk ID: %d is", chunk);

                                printf("%s\n", buf.mtext);
                            }
                                                  
                        }
                    }
                } 
                else {
                    perror("File not found in metadata server");
                }
            } 
            else {
                perror("Could not receive message from M-server");
            }

        }  
        
        else if(strcmp("HELP", command) == 0){
            printf("Formats of the commands are: \n\n"
                    "1. ADD_FILE /path/to/file/filename\n"
                    "2. CP /path/to/src/filename /path/to/dest/filename\n"
                    "3. MV /path/to/src/filename /path/to/dest/filename\n"
                    "4. RM /path/to/file/filename\n"
                    "5. EXEC ON <pid of D server> <command>\n\n");
        } 
        
        else {
            printf("Could not recognise command... Please try again\n");
        }

        printf("Enter command (ADD_FILE / CP / MV / RM / EXEC)\n");

        fgets(command_input, 400, stdin);
    }

}
