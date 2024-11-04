#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_LEN 512
#define MAXARGS 10
#define MAXVARS 20  // Max number of variables

// Struct for storing variables
struct var {
    char *name;
    char *value;
    int global;  // 0 for local, 1 for global (environment)
};

struct var var_table[MAXVARS];
int var_count = 0;

void set_var(char *name, char *value, int global);
char* get_var(char *name);
void list_vars();
void execute(char* arglist[]);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);
void parse_and_execute(char* cmdline);

int main() {
    char *cmdline;
    char* prompt = "PUCITshell:- ";

    // Shell loop
    while((cmdline = read_cmd(prompt, stdin)) != NULL) {
        parse_and_execute(cmdline);
        free(cmdline);
    }

    printf("\n");
    return 0;
}

// Function to parse commands and execute them
void parse_and_execute(char* cmdline) {
    if (cmdline[0] == '\0') return;

    // Check for variable assignment
    char *equals_sign = strchr(cmdline, '=');
    if (equals_sign) {
        *equals_sign = '\0';
        char *name = cmdline;
        char *value = equals_sign + 1;
        set_var(name, value, 0);  // Local variable by default
    }
    // Check for variable access or other commands
    else {
        char** arglist = tokenize(cmdline);

        if (strcmp(arglist[0], "set") == 0) {
            list_vars();
        } else if (strcmp(arglist[0], "export") == 0) {
            if (arglist[1] != NULL) {
                set_var(arglist[1], get_var(arglist[1]), 1);  // Set as global
            } else {
                printf("Usage: export <variable>\n");
            }
        } else if (strcmp(arglist[0], "cd") == 0) {
            if (arglist[1] != NULL) {
                if (chdir(arglist[1]) != 0) {
                    perror("cd failed");
                }
            } else {
                printf("Usage: cd <directory>\n");
            }
        } else {
            execute(arglist);
        }

        // Free tokenized arguments
        for (int j = 0; j < MAXARGS + 1; j++) free(arglist[j]);
        free(arglist);
    }
}

// Function to set a variable in the table
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

// Function to get a variable's value from the table or environment
char* get_var(char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(var_table[i].name, name) == 0) {
            return var_table[i].value;
        }
    }
    return getenv(name);  // Check environment if not found in local
}

// Function to list all variables
void list_vars() {
    printf("Local and environment variables:\n");
    for (int i = 0; i < var_count; i++) {
        printf("%s=%s (local)\n", var_table[i].name, var_table[i].value);
    }
    extern char **environ;
    for (char **env = environ; *env; env++) {
        printf("%s (global)\n", *env);
    }
}

// Function to execute external commands
void execute(char* arglist[]) {
    int status;
    int cpid = fork();
    switch(cpid) {
        case -1:
            perror("fork failed");
            exit(1);
        case 0:
            execvp(arglist[0], arglist);
            perror("Command not found...");
            exit(1);
        default:
            waitpid(cpid, &status, 0);
            printf("Child exited with status %d\n", status >> 8);
    }
}

// Function to tokenize command line input
char** tokenize(char* cmdline) {
    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char*)malloc(sizeof(char) * MAX_LEN);
    }
    int argnum = 0;
    char *token = strtok(cmdline, " ");
    while (token != NULL && argnum < MAXARGS) {
        strcpy(arglist[argnum++], token);
        token = strtok(NULL, " ");
    }
    arglist[argnum] = NULL;
    return arglist;
}

// Function to read a command from the prompt
char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    char *cmdline = (char*)malloc(sizeof(char) * MAX_LEN);
    if (fgets(cmdline, MAX_LEN, fp) == NULL) return NULL;
    cmdline[strcspn(cmdline, "\n")] = '\0';  // Remove newline
    return cmdline;
}
