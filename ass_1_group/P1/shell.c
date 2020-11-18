#include "shell.h"

void error_msg(char *msg)
{
    tail->status= -1;
    perror(msg);
    exit(EXIT_FAILURE);
}

int countFreq(char *pat, char *txt)
{
    int M = strlen(pat);
    int N = strlen(txt);
    int res = 0;
    for (int i = 0; i <= N - M; i++)
    {
        int j;
        for (j = 0; j < M; j++)
            if (txt[i + j] != pat[j])
                break;

        if (j == M)
        {
            res++;
            j = 0;
        }
    }
    return res;
}

bool is_fd(char *str)
{
    int l = strlen(str);

    for (int i = 0; i < l; i++)
    {
        if (!isdigit(str[i]))
            return false;
    }
    return true;
}

void close_pipes(int pipe_fd[][2], int cnt)
{
    for (int i = 0; i < cnt; i++)
    {
        close(pipe_fd[i][0]);
        close(pipe_fd[i][1]);
    }
}

void init_cmd(struct command **cmd)
{
    *cmd = (struct command *)malloc(sizeof(struct command));

    if (*cmd == NULL)
    {
        error_msg("Malloc error");
    }

    (*cmd)->argc = 0;
    strcpy((*cmd)->ip_file, "");
    strcpy((*cmd)->op_file, "");
    (*cmd)->ip_redirect = 0;
    (*cmd)->op_redirect = 0;
    (*cmd)->op_append = 0;
    (*cmd)->next = NULL;

    for (int i = 0; i < NUM_OF_ARGS; i++)
        (*cmd)->argv[i] = NULL;
}

void insert_cmd(pipeline *pipeline, struct command *cmd)
{
    if (pipeline->begin == NULL)
    {
        pipeline->begin = cmd;
        pipeline->end = cmd;
        return;
    }
    pipeline->end->next = cmd;
    pipeline->end = cmd;
}

void parse_cmd(struct command **cmd, char *input)
{
    int l = strlen(input);
    char *str = (char *)malloc(sizeof(char) * (l + 1));

    if (str == NULL)
    {
        error_msg("Malloc error for str");
    }

    char prev_str[LENGTH_OF_ARG];
    int arg_size = 0;
    int arg_cnt = 0;

    strcpy(prev_str, "");

    for (int i = 0; i < l; i++)
    {
        if (!isspace(input[i]))
        {
            str[arg_size++] = input[i];
        }
        else if ((i + 1 < l && !isspace(input[i + 1]) && arg_size >= 1) || (i + 1 >= l && arg_size >= 1))
        {
            str[arg_size] = '\0';
            arg_size = 0;

            if (!strcmp(prev_str, "<"))
            {
                strcpy((*cmd)->ip_file, str);
                strcpy(prev_str, str);
                continue;
            }

            if (!strcmp(prev_str, ">") || !strcmp(prev_str, ">>"))
            {
                strcpy((*cmd)->op_file, str);
                strcpy(prev_str, str);
                continue;
            }

            strcpy(prev_str, str);

            if (!strcmp(str, "<"))
            {
                (*cmd)->ip_redirect = true;
                continue;
            }

            if (!strcmp(str, ">"))
            {
                (*cmd)->op_redirect = true;
                (*cmd)->op_append = false;
                continue;
            }

            if (!strcmp(str, ">>"))
            {
                (*cmd)->op_redirect = true;
                (*cmd)->op_append = true;
                continue;
            }

            (*cmd)->argv[arg_cnt] = (char *)malloc(sizeof(char) * (arg_size + 10));
            if ((*cmd)->argv[arg_cnt] == NULL)
            {
                error_msg("Malloc error for argv[arg_cnt]");
            }

            strcpy((*cmd)->argv[arg_cnt], str);

            arg_cnt++;
        }
    }
    (*cmd)->argv[arg_cnt] = NULL;
    (*cmd)->argc = arg_cnt;
    free(str);
}

void create_pipeline(char *input, pipeline *pipeline, int flag)
{
    char *token;
    int size = 0;

    if (flag == 0)
    {
        while ((token = strsep(&input, "|")) != NULL)
        {
            struct command *cmd;

            init_cmd(&cmd);
            parse_cmd(&cmd, token);
            size++;
            if (cmd->ip_redirect && is_fd(cmd->ip_file))
            {
                int ip_fd = atoi(cmd->ip_file);
                // Work reqd
            }

            insert_cmd(pipeline, cmd);

            if (cmd->op_redirect && is_fd(cmd->op_file))
            {
                int op_fd = atoi(cmd->op_file);
                // Work reqd
            }
        }
    }

    num_cmds = size;
    pipeline->cmd_cnt = num_cmds;
    return;
}

void exec_commands(pipeline *pipeline)
{
    struct command *curr = pipeline->begin;
    int cnt = pipeline->cmd_cnt;

    int pipe_fd[cnt - 1][2];

    for (int i = 0; i < cnt - 1; i++)
    {
        if (pipe(pipe_fd[i]) == -1)
        {
            error_msg("Pipe creation error");
        }
    }

    struct command *cmd;
    int i;
    for (i = 0, cmd = pipeline->begin; i < cnt, cmd != NULL; i++, cmd = cmd->next)
    {
        // if (i < cnt - 1)
        // {
        //     printf("\n**--------** pipe[%d] : read_fd = %d, write_fd = %d **--------**\n", i, pipe_fd[i][0], pipe_fd[i][1]);
        // }

        pid_t pid = fork();

        if (pid == -1)
        {
            error_msg("Fork error");
        }

        if (pid == 0)
        {
            printf("\n|========== Process[%d] pid: %d ==========|\n", i, getpid());

            if (i != 0)
            {
                if (dup2(pipe_fd[i - 1][0], STDIN_FILENO) == -1)
                {
                    error_msg("dup2 error while read");
                }
            }

            if (i != cnt - 1)
            {
                if (dup2(pipe_fd[i][1], STDOUT_FILENO) == -1)
                {
                    error_msg("dup2 error while write");
                }
            }

            if (cmd->ip_redirect && !is_fd(cmd->ip_file))
            {
                int inp_fd;
                inp_fd = open(cmd->ip_file, O_RDONLY);

                if (inp_fd == -1)
                {
                    error_msg("Open input file error");
                }

                if (dup2(inp_fd, STDIN_FILENO) == -1)
                {
                    error_msg("dup2 error for stdin");
                }

                if (close(inp_fd) == -1)
                {
                    error_msg("Close fd error");
                }
            }

            if (cmd->op_redirect && cmd->op_append && !is_fd(cmd->op_file))
            {
                int out_fd = open(cmd->op_file, O_APPEND | O_WRONLY | O_CREAT, 0777);
                if (out_fd == -1)
                {
                    error_msg("Open output file error");
                }

                if (dup2(out_fd, STDOUT_FILENO) == -1)
                {
                    error_msg("dup2 error for stdout");
                }

                if (close(out_fd) == -1)
                {
                    error_msg("Close fd error");
                }
            }
            else if (cmd->op_redirect && !is_fd(cmd->op_file))
            {
                int out_fd = open(cmd->op_file, O_TRUNC | O_WRONLY | O_CREAT, 0777);
                if (out_fd == -1)
                {
                    error_msg("Open output file error");
                }
                if (dup2(out_fd, STDOUT_FILENO) == -1)
                {
                    error_msg("dup2 error for stdout");
                }
                if (close(out_fd) == -1)
                {
                    error_msg("Close fd error");
                }
            }
            close_pipes(pipe_fd, cnt - 1);

            if (execvp(cmd->argv[0], cmd->argv) == -1)
            {
                error_msg("execvp error");
            }
        }
        else
        {
            if (i - 1 > 0)
            {
                close(pipe_fd[i - 1][0]);
            }
            close(pipe_fd[i][1]);
            if (wait(NULL) == -1)
            {
                error_msg("wait error");
            }
        }
    }
    close_pipes(pipe_fd, cnt - 1);
}

void remove_commands(pipeline *pipeline)
{
    struct command *tmp;
    if (!pipeline)
        return;
    for (tmp = pipeline->begin; tmp != NULL; tmp = tmp->next)
    {
        free(tmp);
    }
    pipeline->begin = NULL;
    pipeline->end = NULL;
    pipeline->cmd_cnt = 0;
}

void sigquitHandler(int signo, siginfo_t *info, void *secret)
{
    if (signo == SIGQUIT)
    {
        printf("\nDO YOU REALLY WANT TO QUIT? (Y/N)- ");
        char ch;
        ch= getc(stdin);
        if (ch == 'Y' || ch == 'y')
        {
            kill(info->si_pid, SIGKILL);
        }
        printf("P1_Shell> ");
        fflush(0);
    }
}

void sigintHandler(int signo, siginfo_t *info, void *secret)
{
    if (signo == SIGINT)
    {
        struct cmd_status *ptr;
        printf("\nLast 10 commands:- (Highest index => Latest)\n");
        if (qsize == 0)
        {
            printf("No commands executed yet\n");
        }
        else{
            int i=1;
            for (ptr = head; ptr != NULL; ptr = ptr->next)
            {
                char *st = ptr->status == 1 ? "Completed" : "Running";
                printf("%d. #CMD-- %s   #STATUS-- %s\n",i++, ptr->cmd_name, st);
            }
        }
    }
    printf("P1_Shell> ");
    fflush(0);
}

void load_command(char *input, int id)
{
    if (qsize == 0)
    {
        strcpy(head->cmd_name, input);
        head->next = NULL;
        head->id = id;
        tail = head;
        qsize++;
    }
    else if (qsize < 10)
    {
        qsize++;
        struct cmd_status *node = (struct cmd_status *)malloc(sizeof(struct cmd_status));
        strcpy(node->cmd_name, input);
        node->next = NULL;
        node->status = 0;
        node->id = id;
        tail->next = node;
        tail = node;
    }
    else
    {
        struct cmd_status *node = (struct cmd_status *)malloc(sizeof(struct cmd_status));
        strcpy(node->cmd_name, input);
        node->next = NULL;
        node->status = 0;
        node->id = id;
        tail->next = node;
        tail = node;
        struct cmd_status *temp = head;
        free(temp);
        head = head->next;
    }
}

void update_command()
{
    if(tail->status==0)
        tail->status = 1;
}

int main()
{
    int qid = 0;
    head = (struct cmd_status *)malloc(sizeof(struct cmd_status));
    tail = (struct cmd_status *)malloc(sizeof(struct cmd_status));

    strcpy(head->cmd_name, "");
    head->next = NULL;
    head->status = 0;
    head->id = qid;

    strcpy(tail->cmd_name, "");
    tail->next = NULL;
    tail->status = 0;
    tail->id = qid;

    qsize = 0;

    input = NULL;
    size_t size = 0;
    pipeline pipeline;
    pipeline.begin = NULL;
    pipeline.end = NULL;
    pipeline.cmd_cnt = 0;

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigdelset(&mask, SIGQUIT);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    struct sigaction sa;
    sa.sa_sigaction = &sigquitHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGQUIT, &sa, NULL);

    struct sigaction sb;
    sb.sa_sigaction = &sigintHandler;
    sigemptyset(&sb.sa_mask);
    sb.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sb, NULL);

    while (1)
    {
        printf("P1_Shell> ");
        getline(&input, &size, stdin);
        if (input && strcmp(input, "\n"))
        {
            load_command(input, qid);

            if (countFreq("||", input) == 1)
            {
                create_pipeline2(input, &pipeline, 1);
                exec_commands2(&pipeline);
            }
            else if (countFreq("|||", input) == 1)
            {
                create_pipeline3(input, &pipeline, 2);
                exec_commands3(&pipeline);
            }
            else
            {
                create_pipeline(input, &pipeline, 0);
                exec_commands(&pipeline);
            }

            update_command();
            qid = (++qid) % 10;
            remove_commands(&pipeline);
        }
    }
    free(head);
    free(tail);
    return 0;
}