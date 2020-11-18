#include "shell.h"

void create_pipeline2(char *input, pipeline *pipeline, int flag)
{
    char *token;
    int size = 0;

    struct command *cmd1;
    struct command *cmd2;
    struct command *cmd3;

    init_cmd(&cmd1);
    init_cmd(&cmd2);
    init_cmd(&cmd3);

    token = strsep(&input, "|");
    parse_cmd(&cmd1, token);

    if (cmd1->ip_redirect && is_fd(cmd1->ip_file))
    {
        int ip_fd = atoi(cmd1->ip_file);
        // Work reqd
    }

    insert_cmd(pipeline, cmd1);

    if (cmd1->op_redirect && is_fd(cmd1->op_file))
    {
        int op_fd = atoi(cmd1->op_file);
        // Work reqd
    }

    strsep(&input, "|");
    strsep(&input, " ");
    token = strsep(&input, ",");


    char c = ' ';

    size_t len = strlen(token);
    char *str2 = malloc(len + 1 + 1 ); /* one for extra char, one for trailing zero */
    strcpy(str2, token);
    str2[len] = c;
    str2[len + 1] = '\0';

    parse_cmd(&cmd2, str2);
    if (cmd2->ip_redirect && is_fd(cmd2->ip_file))
    {
        int ip_fd = atoi(cmd2->ip_file);
        // Work reqd
    }

    insert_cmd(pipeline, cmd2);

    if (cmd2->op_redirect && is_fd(cmd2->op_file))
    {
        int op_fd = atoi(cmd2->op_file);
        // Work reqd
    }
    
    token = strsep(&input, ",");
    
    parse_cmd(&cmd3, token);

    if (cmd3->ip_redirect && is_fd(cmd3->ip_file))
    {
        int ip_fd = atoi(cmd3->ip_file);
        // Work reqd
    }

    insert_cmd(pipeline, cmd3);

    if (cmd3->op_redirect && is_fd(cmd3->op_file))
    {
        int op_fd = atoi(cmd3->op_file);
        // Work reqd
    }

    num_cmds = 3;
    pipeline->cmd_cnt = num_cmds;
}

void exec_commands2(pipeline *pipeline)
{
    int pfd[2];
    if (pipe(pfd) == -1)
    {
        error_msg("Pipe creation error");
    }
    struct command *cmd = pipeline->begin;
    int i = 0;
    int cnt = 3;
    char op_buff[100 * sizeof(char)];
    char *output;

    // start command 1

    // printf("\n**--------** pipe[%d] : read_fd = %d, write_fd = %d **--------**\n", i, pfd[0], pfd[1]);
    pid_t pid = fork();
    int temp_fd = fileno(stdout);
    if (pid == -1)
    {
        error_msg("Fork error");
    }
    if (pid == 0)
    {
        printf("\n|========== Process[%d] pid: %d ==========|\n", i, getpid());
        if (dup2(pfd[1], STDOUT_FILENO) == -1)
        {
            error_msg("dup2 error while write");
        }

        if (cmd->op_redirect && cmd->op_append && is_fd(cmd->op_file))
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
        close(pfd[0]);
        close(pfd[1]);
        if (execvp(cmd->argv[0], cmd->argv) == -1)
        {
            error_msg("execvp error");
        }
    }

    else
    {
        if (wait(NULL) == -1)
        {
            error_msg("wait error");
        }
        int sz;
        sz = read(pfd[0], op_buff, 100);
        output = (char *)malloc((sz + 1) * sizeof(char));
        memcpy(output, &op_buff[0], sz);
        output[sz] = '\0';
        close(pfd[1]);
        close(pfd[0]);

        // end command 1
        for (i = 1, cmd = cmd->next; i < cnt, cmd != NULL; i++, cmd = cmd->next)
        {
            // if (i < cnt - 1)
            // {
            //     printf("\n**--------** pipe[%d] : read_fd = %d, write_fd = %d **--------**\n", i, pfd[0], pfd[1]);
            // }

            pid = fork();

            if (pid == -1)
            {
                error_msg("Fork error");
            }

            if (pid == 0)
            {
                int fd[2];
                if (pipe(fd) == -1)
                {
                    error_msg("Pipe creation error");
                }

                write(fd[1], output, strlen(output));

                printf("\n|========== Process[%d] pid: %d ==========|\n", i, getpid());

                if (dup2(fd[0], STDIN_FILENO) == -1)
                {
                    error_msg("dup2 error while read");
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

                if (cmd->op_redirect && cmd->op_append && is_fd(cmd->op_file))
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
                close(fd[0]);
                close(fd[1]);
                if (execvp(cmd->argv[0], cmd->argv) == -1)
                {
                    error_msg("execvp error");
                }
            }

            else
            {
                if (wait(NULL) == -1)
                {
                    error_msg("wait error");
                }
            }
        }
    }
}