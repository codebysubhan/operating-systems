#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 10
#define HISTORY_SIZE 10
#define MAXVARS 20  // Max number of variables
#define PROMPT "PUCITshell:- "

typedef struct {
    pid_t pid;
    char command[MAX_LEN];
} Job;

struct var {
    char *name;
    char *value;
    int global;  // 0 for local, 1 for global
};

Job jobs[HISTORY_SIZE];
int job_count = 0;

char* history[HISTORY_SIZE];
int history_count = 0;

struct var var_table[MAXVARS];
int var_count = 0;

// Function Prototypes
void sigchld_handler(int sig);
void add_to_history(char* cmd);
void add_job(pid_t pid, char* command);
void remove_job(pid_t pid);
void list_jobs();
void set_var(char *name, char *value, int global);
char* get_var(char *name);
void list_vars();
int handle_builtin(char* arglist[]);
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char* arglist[], int background);
int handle_redirection_and_pipes(char* cmdline);
void parse_and_execute(char* cmdline);

int main() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history[i] = NULL;
        jobs[i].pid = 0;
    }

    signal(SIGCHLD, sigchld_handler);

    char *cmdline;
    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        parse_and_execute(cmdline);
        free(cmdline);
    }
    printf("\n");
    return 0;
}

void parse_and_execute(char* cmdline) {
    add_to_history(cmdline);

    char *equals_sign = strchr(cmdline, '=');
    if (equals_sign) {
        *equals_sign = '\0';
        char *name = cmdline;
        char *value = equals_sign + 1;
        set_var(name, value, 0);  // Local variable by default
    } else {
        if (handle_redirection_and_pipes(cmdline) != 0) {
            printf("Error executing command\n");
        }
    }
}

void sigchld_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    int c, pos = 0;
    char* cmdline = (char*)malloc(sizeof(char) * MAX_LEN);
    while ((c = getc(fp)) != EOF) {
        if (c == '\n') break;
        cmdline[pos++] = c;
    }
    if (c == EOF && pos == 0) return NULL;
    cmdline[pos] = '\0';
    return cmdline;
}

char** tokenize(char* cmdline) {
    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char*)malloc(sizeof(char) * 30);
        bzero(arglist[j], 30);
    }
    int argnum = 0;
    char* cp = cmdline;
    char* start;
    int len;
    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t') cp++;
        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) len++;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }
    arglist[argnum] = NULL;
    return arglist;
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

void add_job(pid_t pid, char* command) {
    if (job_count < HISTORY_SIZE) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command, command, MAX_LEN);
        job_count++;
    }
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

void list_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s\n", i + 1, jobs[i].pid, jobs[i].command);
    }
}

void set_var(char *name, char *value, int global) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_table[i].name, name) == 0) {
            free(var_table[i].value);
            var_table[i].value = strdup(value);
            var_table[i].global = global;
            if (global) setenv(name, value, 1);
            return;
        }
    }
    if (var_count < MAXVARS) {
        var_table[var_count].name = strdup(name);
        var_table[var_count].value = strdup(value);
        var_table[var_count].global = global;
        if (global) setenv(name, value, 1);
        var_count++;
    } else {
        printf("Variable limit reached.\n");
    }
}

char* get_var(char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_table[i].name, name) == 0) {
            return var_table[i].value;
        }
    }
    return getenv(name);
}

void list_vars() {
    printf("Local and environment variables:\n");
    for (int i = 0; i < var_count; i++) {
        printf("%s=%s (local)\n", var_table[i].name, var_table[i].value);
    }
}

int handle_builtin(char* arglist[]) {
    if (strcmp(arglist[0], "set") == 0) {
        list_vars();
        return 0;
    } else if (strcmp(arglist[0], "export") == 0) {
        if (arglist[1] != NULL) {
            set_var(arglist[1], get_var(arglist[1]), 1);
        } else {
            printf("Usage: export <variable>\n");
        }
        return 0;
    } else if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] != NULL) {
            if (chdir(arglist[1]) != 0) perror("cd failed");
        } else printf("cd: missing argument\n");
        return 0;
    } else if (strcmp(arglist[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(arglist[0], "jobs") == 0) {
        list_jobs();
        return 0;
    }
    return 1;
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
                add_job(pid, command);
                printf("[%d] %d\n", job_count, pid);
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
