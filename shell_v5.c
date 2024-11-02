#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define HISTORY_SIZE 10
#define PROMPT "PUCITshell:- "

// Structure to store background jobs
typedef struct {
    pid_t pid;
    char command[MAX_LEN];
} Job;

Job jobs[HISTORY_SIZE];
int job_count = 0;

// Function declarations
char* read_cmd(char*, FILE*);
char** tokenize(char* cmdline);
int execute(char* arglist[], int background);
void sigchld_handler(int sig);
void add_job(pid_t pid, char* command);
void remove_job(pid_t pid);
void list_jobs();
int handle_builtin(char* arglist[]);
void add_to_history(char* cmd);
void kill_job(int job_number);

// Command history
char* history[HISTORY_SIZE];
int history_count = 0;

int main() {
    // Initialize history and jobs
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history[i] = NULL;
        jobs[i].pid = 0;
    }

    // Handle SIGCHLD to prevent zombies
    signal(SIGCHLD, sigchld_handler);

    char *cmdline;
    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        char **arglist = tokenize(cmdline);
        
        // Check for empty command
        if (arglist[0] == NULL) {
            free(cmdline);
            continue;
        }

        // Add command to history
        add_to_history(cmdline);

        // Check if it's a built-in command
        if (handle_builtin(arglist) == 0) {
            free(cmdline);
            continue;
        }

        // Check if command should run in the background
        int background = 0;
        int last_arg = 0;
        while (arglist[last_arg] != NULL) last_arg++;
        if (last_arg > 0 && strcmp(arglist[last_arg - 1], "&") == 0) {
            background = 1;
            free(arglist[last_arg - 1]);
            arglist[last_arg - 1] = NULL;
        }

        pid_t pid = fork();
        if (pid == 0) { // Child process
            execvp(arglist[0], arglist);
            perror("Command not found...");
            exit(1);
        } else if (pid > 0) { // Parent process
            if (background) {
                add_job(pid, cmdline);
                printf("[%d] %d\n", job_count, pid);
            } else {
                waitpid(pid, NULL, 0);
            }
        } else {
            perror("Fork failed");
        }

        free(cmdline);
    }
    printf("\n");
    return 0;
}

// Read a command from standard input
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

// Tokenize a command line into arguments
char** tokenize(char* cmdline) {
    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[j], ARGLEN);
    }
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

// Add command to history
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

// Handle built-in commands
int handle_builtin(char* arglist[]) {
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] != NULL) {
            if (chdir(arglist[1]) != 0) {
                perror("cd failed");
            }
        } else {
            fprintf(stderr, "cd: missing argument\n");
        }
        return 0;
    } else if (strcmp(arglist[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(arglist[0], "jobs") == 0) {
        list_jobs();
        return 0;
    } else if (strcmp(arglist[0], "kill") == 0) {
        if (arglist[1] != NULL) {
            int job_number = atoi(arglist[1]);
            kill_job(job_number);
        } else {
            fprintf(stderr, "kill: missing argument\n");
        }
        return 0;
    } else if (strcmp(arglist[0], "help") == 0) {
        printf("Built-in commands:\n");
        printf("cd <directory>    - Change the current directory\n");
        printf("exit              - Exit the shell\n");
        printf("jobs              - List background jobs\n");
        printf("kill <job_number> - Kill a background job\n");
        printf("help              - Show this help message\n");
        return 0;
    }
    return 1;
}

// SIGCHLD handler to prevent zombie processes
void sigchld_handler(int sig) {
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        remove_job(pid);
    }
}

// Add a background job
void add_job(pid_t pid, char* command) {
    if (job_count < HISTORY_SIZE) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command, command, MAX_LEN - 1);
        job_count++;
    }
}

// Remove a background job by PID
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

// List all background jobs
void list_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s\n", i + 1, jobs[i].pid, jobs[i].command);
    }
}

// Kill a background job by job number
void kill_job(int job_number) {
    if (job_number < 1 || job_number > job_count) {
        fprintf(stderr, "No such job\n");
        return;
    }
    pid_t pid = jobs[job_number - 1].pid;
    if (kill(pid, SIGKILL) == 0) {
        printf("Killed job [%d] %d\n", job_number, pid);
        remove_job(pid);
    } else {
        perror("Failed to kill job");
    }
}
