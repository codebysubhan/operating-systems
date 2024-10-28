#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "PUCITshell:- "

int execute(char* arglist[]);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);
int handle_redirection_and_pipes(char* cmdline);

int main() {
    char *cmdline;
    char *prompt = PROMPT;
    while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
        if (handle_redirection_and_pipes(cmdline) != 0) {
            free(cmdline);
            continue;
        }
        free(cmdline);
    }
    printf("\n");
    return 0;
}

int handle_redirection_and_pipes(char* cmdline) {
    int pipefd[2];
    pid_t pid;
    int in_fd = 0;
    char *command = strtok(cmdline, "|");

    while (command != NULL) {
        char *infile = NULL, *outfile = NULL;
        char **arglist = tokenize(command);

        // Check for redirection
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
        } else if (pid == 0) { // Child process
            if (in_fd != 0) { // Redirect stdin
                dup2(in_fd, 0);
                close(in_fd);
            }

            if (infile) { // Input redirection
                int fd0 = open(infile, O_RDONLY);
                if (fd0 < 0) {
                    perror("Error opening input file");
                    exit(1);
                }
                dup2(fd0, 0);
                close(fd0);
            }

            if (outfile) { // Output redirection
                int fd1 = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd1 < 0) {
                    perror("Error opening output file");
                    exit(1);
                }
                dup2(fd1, 1);
                close(fd1);
            }

            if (command != NULL && strtok(NULL, "|") != NULL) { // Pipe redirection
                dup2(pipefd[1], 1);
                close(pipefd[1]);
            }

            execute(arglist);
            exit(1);
        } else { // Parent process
            waitpid(pid, NULL, 0);
            close(pipefd[1]);
            in_fd = pipefd[0];
            command = strtok(NULL, "|");
        }
    }

    return 0;
}

int execute(char* arglist[]) {
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
