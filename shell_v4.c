#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define HISTORY_SIZE 10
#define PROMPT "PUCITshell:- "

int execute(char* arglist[], int background);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);
int handle_redirection_and_pipes(char* cmdline);
void sigchld_handler(int sig);
void add_to_history(char* cmd);
char* get_command_from_history(int index);

char* history[HISTORY_SIZE];
int history_count = 0;

int main() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history[i] = NULL;
    }

    signal(SIGCHLD, sigchld_handler);
    
    char *cmdline;
    char *prompt = PROMPT;
    while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
        if (cmdline[0] == '!') {
            int index;
            if (strcmp(cmdline, "!-1") == 0) {
                index = history_count - 1;
            } else if (cmdline[1] >= '0' && cmdline[1] <= '9') {
                index = atoi(cmdline + 1) - 1;
            } else {
                printf("Invalid history command.\n");
                free(cmdline);
                continue;
            }

            if (index < 0 || index >= history_count || history[index] == NULL) {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            }

            free(cmdline);
            cmdline = strdup(history[index]);
            printf("%s\n", cmdline);
            } else {
            add_to_history(cmdline);
        }

        if (handle_redirection_and_pipes(cmdline) != 0) {
            free(cmdline);
            continue;
        }
        free(cmdline);
    }
    printf("\n");
    return 0;
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int handle_redirection_and_pipes(char* cmdline) {
    int pipefd[2];
    pid_t pid;
    int in_fd = 0;
    char *command = strtok(cmdline, "|");

    while (command != NULL) {
        char *infile = NULL, *outfile = NULL;
        char **arglist = tokenize(command);
        
        int background = 0;
        int last_arg = 0;
        while (arglist[last_arg] != NULL) last_arg++;
        if (last_arg > 0 && strcmp(arglist[last_arg - 1], "&") == 0) {
            background = 1;
            free(arglist[last_arg - 1]);
            arglist[last_arg - 1] = NULL;
        }
        for (int i = 0; arglist[i] != NULL; i++) {
            if (strcmp(arglist[i], "<") == 0) {
                infile = arglist[i + 1];
                arglist[i] = NULL;
            } else if (strcmp(arglist[i], ">") == 0) {
                outfile = arglist[i + 1];
                arglist[i] = NULL;
            }
        }

        if (pipe(pipefd) == -1) {
            perror("Pipe failed");
            exit(1);
        }

        pid = fork();
        if (pid == -1) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            if (in_fd != 0) {
                dup2(in_fd, 0);
                close(in_fd);
            }

            if (infile) {
                int fd0 = open(infile, O_RDONLY);
                if (fd0 < 0) {
                    perror("Error opening input file");
                    exit(1);
                }
                dup2(fd0, 0);
                close(fd0);
            }

            if (outfile) {
                int fd1 = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd1 < 0) {
                    perror("Error opening output file");
                    exit(1);
                }
                dup2(fd1, 1);
                close(fd1);
            }

            if (command != NULL && strtok(NULL, "|") != NULL) {
                dup2(pipefd[1], 1);
                close(pipefd[1]);
            }

            execute(arglist, background);
            exit(1);
        } else {
            if (!background) {
                waitpid(pid, NULL, 0);
            } else {
                printf("[%d] %d\n", background, pid);
            }
            close(pipefd[1]);
            in_fd = pipefd[0];
            command = strtok(NULL, "|");
        }
    }

    return 0;
}

int execute(char* arglist[], int background) {
    execvp(arglist[0], arglist);
    perror("Command not found...");
    exit(1);
}

char** tokenize(char* cmdline) {
    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[j], ARGLEN);
    }
    if (cmdline[0] == '\0')
        return NULL;
    int argnum = 0;
    char* cp = cmdline;
    char* start;
    int len;
    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t')
            cp++;
        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t'))
            len++;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }
    arglist[argnum] = NULL;
    return arglist;
}

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    int c;
    int pos = 0;
    char* cmdline = (char*)malloc(sizeof(char) * MAX_LEN);
    while ((c = getc(fp)) != EOF) {
        if (c == '\n')
            break;
        cmdline[pos++] = c;
    }
    if (c == EOF && pos == 0)
        return NULL;
    cmdline[pos] = '\0';
    return cmdline;
}

void add_to_history(char* cmd) {
    if (history_count == HISTORY_SIZE) {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++) {
            history[i - 1] = history[i];
        }
        history_count--;
    }
    history[history_count++] = strdup(cmd);
}

char* get_command_from_history(int index) {
    if (index < 0 || index >= history_count)
        return NULL;
    return history[index];
}
