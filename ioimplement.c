#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_PATH_LENGTH 4096
#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGS 64

/**
 * Structure to hold redirection information
 */
typedef struct {
    char *input_file;   // File for input redirection (<)
    char *output_file;  // File for output redirection (>)
} Redirection;

/**
 * Check if a file exists and if it is executable
 * Returns 1 if executable, 0 otherwise
 */
int is_executable(const char *filepath) {
    struct stat st;

    if (stat(filepath, &st) == 0) {
        if (S_ISREG(st.st_mode) && (st.st_mode & S_IXUSR)) {
            return 1;
        }
    }
    return 0;
}

/**
 * Search for command in PATH directories
 */
char* search_path(const char *command) {
    char *path_env;
    char *path_copy;
    char *directory;
    char *full_path;

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
            return full_path;
        }

        directory = strtok(NULL, ":");
    }

    free(path_copy);
    free(full_path);
    return NULL;
}


char* expand_variables(const char *str) {
    static char expanded[MAX_COMMAND_LENGTH * 2];
    char var_name[256];
    int i = 0, j = 0;

    while (str[i] != '\0' && j < MAX_COMMAND_LENGTH * 2 - 1) {
        if (str[i] == '$') {
            i++;
            int k = 0;

            while ((str[i] >= 'A' && str[i] <= 'Z') ||
                   (str[i] >= 'a' && str[i] <= 'z') ||
                   (str[i] >= '0' && str[i] <= '9') ||
                   str[i] == '_') {
                var_name[k++] = str[i++];
            }
            var_name[k] = '\0';

            char *value = getenv(var_name);
            if (value != NULL) {
                int len = strlen(value);
                if (j + len < MAX_COMMAND_LENGTH * 2) {
                    strcpy(&expanded[j], value);
                    j += len;
                }
            }
        } else {
            expanded[j++] = str[i++];
        }
    }

    expanded[j] = '\0';
    return strdup(expanded);
}

/**
 * Parse command line into arguments and handle I/O redirection
 */
int parse_arguments(char *input, char **args, Redirection *redir) {
    int argc = 0;
    char *token;
    char *expanded_input;


    redir->input_file = NULL;
    redir->output_file = NULL;


    expanded_input = expand_variables(input);
    if (expanded_input == NULL) {
        return 0;
    }


    token = strtok(expanded_input, " \t\n");

    while (token != NULL && argc < MAX_ARGS - 1) {
        if (strcmp(token, "<") == 0) {
            // Input redirection
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected filename after '<'\n");
                free(expanded_input);
                return -1;
            }
            redir->input_file = strdup(token);
        } else if (strcmp(token, ">") == 0) {
            // Output redirection
            token = strtok(NULL, " \t\n");
            if (token == NULL) {
                fprintf(stderr, "Syntax error: expected filename after '>'\n");
                free(expanded_input);
                return -1;
            }
            redir->output_file = strdup(token);
        } else {
            // Regular argument
            args[argc++] = strdup(token);
        }

        token = strtok(NULL, " \t\n");
    }

    args[argc] = NULL;
    free(expanded_input);
    return argc;
}

/**
 * Setup I/O redirection in child process
 * Returns 0 on success, -1 on failure
 */
int setup_redirection(Redirection *redir) {

    if (redir->input_file != NULL) {
        int fd_in = open(redir->input_file, O_RDONLY);
        if (fd_in < 0) {
            perror(redir->input_file);
            return -1;
        }


        if (dup2(fd_in, STDIN_FILENO) < 0) {
            perror("dup2");
            close(fd_in);
            return -1;
        }
        close(fd_in);
    }


    if (redir->output_file != NULL) {
        int fd_out = open(redir->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_out < 0) {
            perror(redir->output_file);
            return -1;
        }


        if (dup2(fd_out, STDOUT_FILENO) < 0) {
            perror("dup2");
            close(fd_out);
            return -1;
        }
        close(fd_out);
    }

    return 0;
}


void free_arguments(char **args, int argc) {
    for (int i = 0; i < argc; i++) {
        if (args[i] != NULL) {
            free(args[i]);
            args[i] = NULL;
        }
    }
}


void free_redirection(Redirection *redir) {
    if (redir->input_file != NULL) {
        free(redir->input_file);
        redir->input_file = NULL;
    }
    if (redir->output_file != NULL) {
        free(redir->output_file);
        redir->output_file = NULL;
    }
}

/**
 * Execute external command with arguments and I/O redirection
 * Returns 0 on success, -1 on failure
 */
int execute_command(char **args, Redirection *redir) {
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
        // Child process


        if (setup_redirection(redir) < 0) {
            exit(EXIT_FAILURE);
        }


        execv(executable_path, args);


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


        if (executable_path != args[0]) {
            free(executable_path);
        }

        // Check if child exited normally
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
    }

    return 0;
}

int main() {
    char input[MAX_COMMAND_LENGTH];
    char *args[MAX_ARGS];
    Redirection redir;

    while (1) {

        fflush(stdout);

        // Read command from user
        if (fgets(input, MAX_COMMAND_LENGTH, stdin) == NULL) {
            printf("\n");
            break;
        }


        input[strcspn(input, "\n")] = '\0';


        if (strlen(input) == 0) {
            continue;
        }


        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Parse arguments and handle redirection
        int argc = parse_arguments(input, args, &redir);

        if (argc > 0) {
            // Execute the command with redirection
            execute_command(args, &redir);


            free_arguments(args, argc);
            free_redirection(&redir);
        } else if (argc < 0) {

            free_redirection(&redir);
        }
    }

    return 0;
}