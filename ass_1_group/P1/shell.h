#include <stdlib.h>
#include <stdio.h>               
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>  

#define NUM_OF_ARGS 30
#define LENGTH_OF_ARG 100

struct command{
    int argc;
    char *argv[NUM_OF_ARGS];
    bool ip_redirect;
    bool op_redirect;
    bool op_append;
    char ip_file[LENGTH_OF_ARG];
    char op_file[LENGTH_OF_ARG];
    struct command * next;
};

typedef struct{
    struct command * begin;
    struct command * end;
    int cmd_cnt;
}pipeline;

struct cmd_status{
    int id;
    int status;
    char cmd_name[NUM_OF_ARGS * LENGTH_OF_ARG];
    struct cmd_status * next;
};
struct cmd_status * head;
struct cmd_status * tail;

char * input;
int num_cmds;
int qsize;

void init_cmd(struct command **cmd);
void parse_cmd(struct command **cmd, char *input);
void insert_cmd(pipeline *pipeline, struct command *cmd);

void create_pipeline2(char *input, pipeline *pipeline, int flag);
void exec_commands2(pipeline *pipeline);

void create_pipeline3(char *input, pipeline *pipeline, int flag);
void exec_commands3(pipeline *pipeline);

void error_msg(char *msg);
bool is_fd(char *str);