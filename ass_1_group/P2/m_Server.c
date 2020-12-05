#define MSGQ_PATH "/media/vaibhav/New Volume/Academics/3-1/IS F462/Network Programming/ass_1_group/P2"

#include <stdio.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#define NUM_FILES 10

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

struct chunk_server {
  int chunk_id;
  int server1;
  int server2;
  int server3;
};

struct file_chunks {
  char filename[200];
  struct chunk_server * chunks;
  int num_chunks;
};

int num_files = 0;
int max_files = 10;

int chunk = 1;

struct file_chunks * files;

struct my_msgbuf buf;
key_t key;
int msqid = 0;

void createFile(char * name){  

  if(num_files == 0){
    files = (struct file_chunks *)malloc(max_files * sizeof(struct file_chunks));
  }
  else if(num_files == max_files - 1){
    files = (struct file_chunks *)realloc(files, max_files * 2 * sizeof(struct file_chunks));
    max_files = max_files * 2;
  }
  
  int i = 0;
  while(i < num_files && strcmp(files[i].filename, "DELETED") != 0){
    i++;
  }

  if(i < num_files){
    strcpy(files[i].filename, name);
    files[i].num_chunks = 0;
  } 
  else {
    strcpy(files[num_files].filename, name);
    files[num_files].num_chunks = 0;
    num_files += 1;
  }

}

int chunksinFile(char *name){
  int i = 0;
  while(i < num_files && strcmp(files[i].filename, name) != 0){
    i++;
  }

  if(i < num_files){
    return files[i].num_chunks;
  }

  return -1;
}

char * fetchChunk(char * name, int chunk_no){
  int i = 0;
  while(i < num_files && strcmp(files[i].filename, name) != 0){
    i++;
  }

  char *send = (char *)malloc(sizeof(char)*100);

  if(i < num_files){
    if(chunk_no < files[i].num_chunks){
      char temp[12];

      sprintf(temp, "%d ", files[i].chunks[chunk_no].server1);
      strcpy(send, temp);

      sprintf(temp, "%d ", files[i].chunks[chunk_no].server2);
      strcat(send, temp);

      sprintf(temp, "%d ", files[i].chunks[chunk_no].server3);
      strcat(send, temp);

      sprintf(temp, "%d ", files[i].chunks[chunk_no].chunk_id);
      strcat(send, temp);
    }
  }

  return send;
}


char * addChunk(char * name){
  int i = 0;
  while(i < num_files && strcmp(files[i].filename, name) != 0){
    i++;
  }

  char *send = (char *)malloc(sizeof(char)*100);

  if(i < num_files){
    files[i].num_chunks++;
    int num_chunks = files[i].num_chunks;
    if(num_chunks == 1){
      files[i].chunks = (struct chunk_server *)malloc(sizeof(struct chunk_server));
    }
    else {
      files[i].chunks = (struct chunk_server *)realloc(
          files[i].chunks, sizeof(struct chunk_server) * (num_chunks));
    }

    files[i].chunks[num_chunks - 1].chunk_id = chunk;

    // Return a list of 3 D server's along with absolute chunk ID
    buf.mtype = AVAILABLE;
    strcpy(buf.mtext, "?");
    if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
        perror("Could not contact D servers for status");

    if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
        perror("Could not contact D servers for status");

    if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
        perror("Could not contact D servers for status");

    if(msgrcv(msqid, &(buf.mtype), sizeof(buf), D_READY, 0) >= 0){
      strcat(send, buf.mtext);
      files[i].chunks[num_chunks - 1].server1 = atoi(buf.mtext);

      if(msgrcv(msqid, &(buf.mtype), sizeof(buf), D_READY, 0) >= 0){
        strcat(send, buf.mtext);
        files[i].chunks[num_chunks - 1].server2 = atoi(buf.mtext);

        if(msgrcv(msqid, &(buf.mtype), sizeof(buf), D_READY, 0) >= 0){
          strcat(send, buf.mtext);
          files[i].chunks[num_chunks - 1].server3 = atoi(buf.mtext);
        }
      }
    }

    char temp[12];
    sprintf(temp, "%d", chunk);
    strcat(send, temp);
    chunk++;
  }

  return send;
}

int deleteFile(char *p1){
  int i = 0;
  while(i < num_files && strcmp(files[i].filename, p1) != 0){
    i++;
  }

  if(i < num_files){
    int j = 0;
    while(j < files[i].num_chunks){
      struct chunk_server t1 = files[i].chunks[j];

      memset(buf.mtext, '\0', sizeof(char)*200);
      strcpy(buf.mtext, "DELETE_CHUNK ");
      char temp[12];
      sprintf(temp, "%d ", t1.chunk_id);
      strcat(buf.mtext, temp);
      buf.mtype = t1.server1;

      if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
          perror("Could not send add_chunk message");

      buf.mtype = t1.server2;

      if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
          perror("Could not send add_chunk message");

      buf.mtype = t1.server3;

      if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
          perror("Could not send add_chunk message");

      j++;

    }

    struct chunk_server * t;
    t = files[i].chunks;
    files[i].chunks = NULL;
    free(t);
    strcpy(files[i].filename, "DELETED"); 
    files[i].num_chunks = -1;

    return 0;
  }

  return -1;
}

int copyFile(char * p1, char *p2){
  int i = 0;
  while(i < num_files && strcmp(files[i].filename, p1) != 0){
    i++;
  }

  int k = 0;
  while(k < num_files && strcmp(files[k].filename, p2) != 0){
    k++;
  }

  if(i < num_files && k >= num_files){
    createFile(p2);

    int file2 = 0;

    while(strcmp(files[file2].filename, p2) != 0){
      file2++;
    }

    int j = 0;
    while(j < files[i].num_chunks){
      files[file2].num_chunks++;
      int num_chunks = files[file2].num_chunks;
      if(num_chunks == 1){
        files[file2].chunks = (struct chunk_server *)malloc(sizeof(struct chunk_server));
      }
      else {
        files[file2].chunks = (struct chunk_server *)realloc(
            files[file2].chunks, sizeof(struct chunk_server) * (num_chunks));
      }

      struct chunk_server t1 = files[i].chunks[j];
      files[file2].chunks[j].chunk_id = chunk;

      memset(buf.mtext, '\0', sizeof(char)*200);
      strcpy(buf.mtext, "COPY_CHUNK ");
      char temp[12];
      sprintf(temp, "%d ", t1.chunk_id);
      strcat(buf.mtext, temp);

      sprintf(temp, "%d", chunk);
      strcat(buf.mtext, temp);

      buf.mtype = t1.server1;
      files[file2].chunks[num_chunks - 1].server1 = t1.server1;

      if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
          perror("Could not send add_chunk message");

      buf.mtype = t1.server2;
      files[file2].chunks[num_chunks - 1].server2 = t1.server2;

      if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
          perror("Could not send add_chunk message");

      buf.mtype = t1.server3;
      files[file2].chunks[num_chunks - 1].server3 = t1.server3;

      if (msgsnd (msqid, &(buf.mtype), sizeof (buf), 0) == -1)
          perror("Could not send add_chunk message");

      j++;
      chunk++;

    } // while-end (chunks)

    return 0;
    
  }

  return -1;
}

int moveFile(char * p1, char *p2){
  int i = 0;
  while(i < num_files && strcmp(files[i].filename, p1) != 0){
    i++;
  }

  int j = 0;
  while(j < num_files && strcmp(files[j].filename, p2) != 0){
    j++;
  }

  if(i < num_files && j >= num_files){
    strcpy(files[i].filename, p2);
    return 0;
  }

  return -1;
}


int main(){
  num_files = 0;

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

  printf("Master Server Online\n");

  while(1){
    if(msgrcv(msqid, &(buf.mtype), sizeof(buf), CREATE_FILE, IPC_NOWAIT) >= 0){
      printf("Create File Message Received\n");
      createFile(buf.mtext);
      // memset(buf.mtext, '\0', sizeof(char)*199);
      buf.mtype = STATUS;
      strcpy(buf.mtext, "SUCCESS");

      if(msgsnd(msqid, &buf, sizeof(buf), 0) == -1){
        perror("Couldn't send SUCCESS message");
      }
    }

    if(msgrcv(msqid, &(buf.mtype), sizeof(buf), ADD_CHUNK, IPC_NOWAIT) >= 0){    
      char *text;
      text = addChunk(buf.mtext);
      memset(buf.mtext, '\0', sizeof(char)*200);
      buf.mtype = LIST_OF_D;
      
      strcpy(buf.mtext, text);
      // Send list of three D servers and chunk absolute ID

      if(msgsnd(msqid, &(buf.mtype), sizeof(buf), 0) == -1){
        perror("Couldn't send LIST of three D servers");
      }
    }

    if(msgrcv(msqid, &(buf.mtype), sizeof(buf), MV, IPC_NOWAIT) >= 0){
      char *src;
      char *dest;

      int i = 0;
      char *tmp = buf.mtext;
      src = strsep(&tmp, " \t\n");
      dest = strsep(&tmp, " \t\n");

      int res = moveFile(src, dest);

      memset(buf.mtext, '\0', sizeof(char)*200);
      buf.mtype = STATUS;

      if(res == 0){
        strcpy(buf.mtext, "SUCCESS");
      } else {
        strcpy(buf.mtext, "FAILURE");
      }

      if(msgsnd(msqid, &(buf.mtype), sizeof(buf), 0) == -1){
        perror("Couldn't send SUCCESS message");
      }
    }

    if(msgrcv(msqid, &(buf.mtype), sizeof(buf), CP, IPC_NOWAIT) >= 0){
      char *src;
      char *dest;

      int i = 0;
      char *tmp = buf.mtext;
      src = strsep(&tmp, " \t\n");
      dest = strsep(&tmp, " \t\n");

      if(strcmp(src, dest) == 0){
        strcpy(buf.mtext, "FAILURE");
      } else {
        if(copyFile(src, dest) == 0){
          strcpy(buf.mtext, "SUCCESS");
        } else {
          strcpy(buf.mtext, "FAILURE");
        }
      }

      buf.mtype = STATUS;
      if(msgsnd(msqid, &(buf.mtype), sizeof(buf), 0) == -1){
        perror("Couldn't send SUCCESS message");
      }

    }

    if(msgrcv(msqid, &(buf.mtype), sizeof(buf), RM, IPC_NOWAIT) >= 0){
      if(deleteFile(buf.mtext) == 0){
        strcpy(buf.mtext, "SUCCESS");
      } else {
        strcpy(buf.mtext, "FAILURE");
      }

      buf.mtype = STATUS;

      if(msgsnd(msqid, &(buf.mtype), sizeof(buf), 0) == -1){
        perror("Couldn't send SUCCESS message");
      }
    }

    if(msgrcv(msqid, &(buf.mtype), sizeof(buf), EXEC_COMMAND, IPC_NOWAIT) >= 0){

      char fname[200];
      strcpy(fname, buf.mtext);
      int chunksin = chunksinFile(buf.mtext);

      if(chunksin > 0){
        strcpy(buf.mtext, "SUCCESS");
      } else {
        strcpy(buf.mtext, "FAILURE");
      }

      char temp[12];
      sprintf(temp, " %d ", chunksin);
      strcat(buf.mtext, temp);

      buf.mtype = STATUS;

      if(msgsnd(msqid, &(buf.mtype), sizeof(buf), 0) == -1){
        perror("Couldn't send SUCCESS message");
      } 
      else {
        for(int i = 0 ; i < chunksin; i++){
          char * res = fetchChunk(fname, i);

          strcpy(buf.mtext, res);

          buf.mtype = D_FOR_EXEC;

          if(msgsnd(msqid, &(buf.mtype), sizeof(buf), 0) == -1){
            perror("Couldn't send D_FOR_EXEC message");
          }
        }  
      }

    }
  
  }
}
