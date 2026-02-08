#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define MAX_PATH_LENGTH 4096
#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

/**
 * Check if the files exists and is executable
 * Returns 1 if executable, 0 otherwise
 */
int is_executable(const char *filepath) {
    struct stat st;

    // Check if file exists and get its stats
    if (stat(filepath, &st) == 0) {
        // Check if it's a regular file and has execute permissions
        if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
            return 1;
        }
    }
    return 0;
}

/**
 * Search for command in PATH directories
 * Returns dynamically allocated string with full path if found, NULL otherwise
 */
char* search_path(const char *command) {
    char *path_env;
    char *path_copy;
    char *directory;
    char *full_path;

    // Checks if command contains a slash, don't do PATH search
    if (strchr(command, '/') != NULL) {
        return NULL;
    }


    path_env = getenv("PATH");
    if (path_env == NULL) {
        fprintf(stderr, "PATH environment variable not set\n");
        return NULL;
    }


    path_copy = strdup(path_env);
    if (path_copy == NULL) {
        perror("strdup");
        return NULL;
    }


    full_path = (char *)malloc(MAX_PATH_LENGTH);
    if (full_path == NULL) {
        perror("malloc");
        free(path_copy);
        return NULL;
    }


    directory = strtok(path_copy, ":");

    while (directory != NULL) {

        snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", directory, command);


        if (is_executable(full_path)) {
            free(path_copy);
            return full_path;  // Found it!
        }


        directory = strtok(NULL, ":");
    }


    free(path_copy);
    free(full_path);
    return NULL;
}

/**
 * Parse command line into arguments
 * Returns the number of arguments parsed
 */

int parse_arguments(char *input, char **args) {
    int argc = 0;
    char *token;


    token = strtok(input, " \t\n");

    while (token != NULL && argc < MAX_ARGS - 1) {
        args[argc++] = token;
        token = strtok(NULL, " \t\n");
    }


    args[argc] = NULL;

    return argc;
}

/**
 * Executes external command with arguments
 * Returns 0 if success, -1 if its a failure
 */
int execute_command(char **args) {
    char *executable_path = NULL;
    pid_t pid;
    int status;

    if (args[0] == NULL) {
        return -1;
    }


    if (strchr(args[0], '/') != NULL) {

        if (is_executable(args[0])) {
            executable_path = args[0];
        } else {
            fprintf(stderr, "%s: command not found\n", args[0]);
            return -1;
        }
    } else {

        executable_path = search_path(args[0]);

        if (executable_path == NULL) {
            fprintf(stderr, "%s: command not found\n", args[0]);
            return -1;
        }
    }


    pid = fork();

    if (pid < 0) {

        perror("fork");
        if (executable_path != args[0]) {
            free(executable_path);
        }
        return -1;
    } else if (pid == 0) {

        execv(executable_path, args);

        // If execv returns, it failed
        perror("execv");
        exit(EXIT_FAILURE);
    } else {

        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid");
            if (executable_path != args[0]) {
                free(executable_path);
            }
            return -1;
        }

        // Free allocated memory if we used PATH search
        if (executable_path != args[0]) {
            free(executable_path);
        }

        // Check if the child exited normally
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
    }

    return 0;
}

int main() {
    char input[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];

    while (1) {

        fflush(stdout);


        if (fgets(input, MAX_COMMAND_LENGTH, stdin) == NULL) {
            printf("\n");
            break;  // EOF or error
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = '\0';


        if (strlen(input) == 0) {
            continue;
        }


        if (strcmp(input, "exit") == 0) {
            break;
        }


        int argc = parse_arguments(input, args);

        if (argc > 0) {
            // Execute the command
            execute_command(args);
        }
    }

    return 0;
}