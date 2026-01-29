#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

// Color codes for terminal
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_RED "\033[1;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_RESET "\033[0m"

// Function prototypes
void parse_command(char *input, char **args);
int is_builtin(char **args);
void execute_builtin(char **args);
void execute_external(char **args);
void print_prompt();
int has_pipe(char *input);
void execute_piped_commands(char *input);
void execute_single_pipeline(char **commands, int num_commands);

// Signal handler for Ctrl+C
void sigint_handler(int sig) {
  printf("\n");
  print_prompt();
  fflush(stdout);
}

// Print colorful prompt with current directory
void print_prompt() {
  char cwd[MAX_INPUT];
  char *username = getenv("USER");
  char hostname[256];

  // Get current directory
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    strcpy(cwd, "unknown");
  }

  // Get hostname
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    strcpy(hostname, "localhost");
  }

  // Shorten home directory to ~
  char *home = getenv("HOME");
  if (home != NULL && strncmp(cwd, home, strlen(home)) == 0) {
    printf(COLOR_GREEN "%s@%s" COLOR_RESET ":" COLOR_BLUE "~%s" COLOR_RESET
                       "$ ",
           username ? username : "user", hostname, cwd + strlen(home));
  } else {
    printf(COLOR_GREEN "%s@%s" COLOR_RESET ":" COLOR_BLUE "%s" COLOR_RESET "$ ",
           username ? username : "user", hostname, cwd);
  }
}

// Check if command contains pipe
int has_pipe(char *input) { return (strchr(input, '|') != NULL); }

// Parse input string into array of arguments
void parse_command(char *input, char **args) {
  int i = 0;
  char *token;

  // Tokenize by spaces and tabs
  token = strtok(input, " \t");

  while (token != NULL && i < MAX_ARGS - 1) {
    args[i] = token;
    i++;
    token = strtok(NULL, " \t");
  }

  args[i] = NULL; // NULL-terminate the array
}

// Execute commands connected by pipes
void execute_piped_commands(char *input) {
  char *commands[MAX_ARGS];
  int num_commands = 0;
  char input_copy[MAX_INPUT];

  // Make a copy since strtok modifies the string
  strcpy(input_copy, input);

  // Split by pipe symbol
  char *token = strtok(input_copy, "|");
  while (token != NULL && num_commands < MAX_ARGS - 1) {
    // Trim leading spaces
    while (*token == ' ' || *token == '\t')
      token++;
    commands[num_commands] = token;
    num_commands++;
    token = strtok(NULL, "|");
  }

  if (num_commands == 0) {
    return;
  }

  // Execute the pipeline
  execute_single_pipeline(commands, num_commands);
}

// Execute a pipeline of commands
void execute_single_pipeline(char **commands, int num_commands) {
  int pipes[num_commands - 1][2]; // Array of pipe pairs
  pid_t pids[num_commands];       // Array of process IDs

  // Create all pipes
  for (int i = 0; i < num_commands - 1; i++) {
    if (pipe(pipes[i]) < 0) {
      perror("pipe");
      return;
    }
  }

  // Create a process for each command
  for (int i = 0; i < num_commands; i++) {
    pids[i] = fork();

    if (pids[i] < 0) {
      perror("fork");
      return;
    }

    if (pids[i] == 0) {
      // CHILD PROCESS

      // Parse this command's arguments
      char *args[MAX_ARGS];
      char cmd_copy[MAX_INPUT];
      strcpy(cmd_copy, commands[i]);
      parse_command(cmd_copy, args);

      if (args[0] == NULL) {
        exit(1);
      }

      // Redirect input from previous pipe (if not first command)
      if (i > 0) {
        dup2(pipes[i - 1][0], STDIN_FILENO);
      }

      // Redirect output to next pipe (if not last command)
      if (i < num_commands - 1) {
        dup2(pipes[i][1], STDOUT_FILENO);
      }

      // Close all pipe file descriptors in child
      for (int j = 0; j < num_commands - 1; j++) {
        close(pipes[j][0]);
        close(pipes[j][1]);
      }

      // Execute the command
      if (execvp(args[0], args) < 0) {
        fprintf(stderr,
                COLOR_RED "myshell: command not found: %s" COLOR_RESET "\n",
                args[0]);
        exit(127);
      }
    }
  }

  // PARENT PROCESS

  // Close all pipes in parent
  for (int i = 0; i < num_commands - 1; i++) {
    close(pipes[i][0]);
    close(pipes[i][1]);
  }

  // Wait for all children to finish
  for (int i = 0; i < num_commands; i++) {
    int status;
    waitpid(pids[i], &status, 0);
  }
}

// Check if command is a built-in
int is_builtin(char **args) {
  if (strcmp(args[0], "cd") == 0)
    return 1;
  if (strcmp(args[0], "pwd") == 0)
    return 1;
  if (strcmp(args[0], "exit") == 0)
    return 1;
  if (strcmp(args[0], "echo") == 0)
    return 1;
  if (strcmp(args[0], "help") == 0)
    return 1;
  if (strcmp(args[0], "clear") == 0)
    return 1;

  return 0;
}

// Execute built-in commands
void execute_builtin(char **args) {
  // cd command
  if (strcmp(args[0], "cd") == 0) {
    if (args[1] == NULL) {
      // No argument - go to home directory
      char *home = getenv("HOME");
      if (home != NULL) {
        if (chdir(home) != 0) {
          perror("cd");
        }
      } else {
        fprintf(stderr, COLOR_RED "cd: HOME not set" COLOR_RESET "\n");
      }
    } else if (strcmp(args[1], "-") == 0) {
      // cd - goes to previous directory
      char *oldpwd = getenv("OLDPWD");
      if (oldpwd != NULL) {
        printf("%s\n", oldpwd);
        if (chdir(oldpwd) != 0) {
          perror("cd");
        }
      } else {
        fprintf(stderr, COLOR_RED "cd: OLDPWD not set" COLOR_RESET "\n");
      }
    } else {
      // Save current directory as OLDPWD
      char cwd[MAX_INPUT];
      if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setenv("OLDPWD", cwd, 1);
      }

      // Change to specified directory
      if (chdir(args[1]) != 0) {
        perror("cd");
      }
    }
    return;
  }

  // pwd command
  if (strcmp(args[0], "pwd") == 0) {
    char cwd[MAX_INPUT];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      printf("%s\n", cwd);
    } else {
      perror("pwd");
    }
    return;
  }

  // echo command
  if (strcmp(args[0], "echo") == 0) {
    for (int i = 1; args[i] != NULL; i++) {
      printf("%s", args[i]);
      if (args[i + 1] != NULL) {
        printf(" ");
      }
    }
    printf("\n");
    return;
  }

  // clear command
  if (strcmp(args[0], "clear") == 0) {
    printf("\033[2J\033[H"); // Clear screen and move cursor to top
    return;
  }

  // help command
  if (strcmp(args[0], "help") == 0) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("                   " COLOR_GREEN "MyShell Help" COLOR_RESET "\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    printf(COLOR_BLUE "Built-in Commands:" COLOR_RESET "\n");
    printf("  cd [dir]       Change directory\n");
    printf("                 - cd          : Go to home directory\n");
    printf("                 - cd -        : Go to previous directory\n");
    printf("                 - cd /path    : Go to specified path\n");
    printf("\n");
    printf("  pwd            Print current working directory\n");
    printf("  echo [text]    Print text to screen\n");
    printf("  clear          Clear the screen\n");
    printf("  help           Show this help message\n");
    printf("  exit           Exit the shell\n");
    printf("\n");
    printf(COLOR_BLUE "Special Features:" COLOR_RESET "\n");
    printf("  !!             Repeat the last command\n");
    printf("  Ctrl+C         Cancel current input (doesn't exit shell)\n");
    printf("  Ctrl+D         Exit the shell\n");
    printf("\n");
    printf(COLOR_BLUE "Piping:" COLOR_RESET "\n");
    printf("  cmd1 | cmd2    Connect output of cmd1 to input of cmd2\n");
    printf("  Examples:\n");
    printf("    ls | grep txt       - List files containing 'txt'\n");
    printf("    cat file | wc -l    - Count lines in file\n");
    printf("    ps aux | grep user  - Find processes by user\n");
    printf("\n");
    printf(COLOR_BLUE "External Commands:" COLOR_RESET "\n");
    printf("  You can run any system command like:\n");
    printf("  ls, cat, grep, date, whoami, etc.\n");
    printf("\n");
    printf("═══════════════════════════════════════════════════════════\n");
    printf("\n");
    return;
  }

  // exit command
  if (strcmp(args[0], "exit") == 0) {
    if (args[1] != NULL) {
      // Exit with specific code if provided
      int code = atoi(args[1]);
      exit(code);
    }
    exit(0);
  }
}

// Execute external commands using fork and exec
void execute_external(char **args) {
  pid_t pid = fork();

  if (pid < 0) {
    // Fork failed
    perror("fork");
    return;
  }

  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) < 0) {
      fprintf(stderr,
              COLOR_RED "myshell: command not found: %s" COLOR_RESET "\n",
              args[0]);
      exit(127); // Standard exit code for command not found
    }
  } else {
    // Parent process
    int status;
    waitpid(pid, &status, 0); // Wait for child to complete

    // Check exit status
    if (WIFEXITED(status)) {
      int exit_code = WEXITSTATUS(status);
      if (exit_code != 0) {
        printf(COLOR_YELLOW "[Process exited with code %d]" COLOR_RESET "\n",
               exit_code);
      }
    } else if (WIFSIGNALED(status)) {
      int signal_num = WTERMSIG(status);
      printf(COLOR_RED "[Process terminated by signal %d]" COLOR_RESET "\n",
             signal_num);
    }
  }
}

int main() {
  char input[MAX_INPUT];
  char *args[MAX_ARGS];
  char last_command[MAX_INPUT] = "";
  int command_count = 0;

  // Install signal handler for Ctrl+C
  signal(SIGINT, sigint_handler);

  // Welcome message
  printf("\n");
  printf("╔════════════════════════════════════════════╗\n");
  printf("║    Welcome to MyShell Enhanced + Pipes!   ║\n");
  printf("║                                            ║\n");
  printf("║  Type 'help' for available commands       ║\n");
  printf("║  Piping is now supported!                 ║\n");
  printf("║  Example: ls | grep txt                   ║\n");
  printf("╚════════════════════════════════════════════╝\n");
  printf("\n");

  // Main shell loop
  while (1) {
    // Print fancy prompt
    print_prompt();
    fflush(stdout);

    // Read user input
    if (fgets(input, MAX_INPUT, stdin) == NULL) {
      printf("\n");
      break; // EOF (Ctrl+D)
    }

    // Remove trailing newline
    input[strcspn(input, "\n")] = '\0';

    // Handle !! (repeat last command)
    if (strcmp(input, "!!") == 0) {
      if (strlen(last_command) == 0) {
        printf(COLOR_YELLOW "myshell: no previous command" COLOR_RESET "\n");
        continue;
      }
      strcpy(input, last_command);
      printf(COLOR_BLUE "Repeating: %s" COLOR_RESET "\n", input);
    } else if (strlen(input) > 0) {
      // Save non-empty command as last command
      strcpy(last_command, input);
    }

    // Skip empty commands
    if (strlen(input) == 0) {
      continue;
    }

    // Increment command counter
    command_count++;

    // Check if command has pipes
    if (has_pipe(input)) {
      // Handle piped commands
      execute_piped_commands(input);
    } else {
      // Handle regular commands
      parse_command(input, args);

      // Skip if no command entered
      if (args[0] == NULL) {
        continue;
      }

      // Check if it's a built-in command
      if (is_builtin(args)) {
        execute_builtin(args);
      } else {
        // Execute external command
        execute_external(args);
      }
    }
  }

  printf("\n");
  printf(COLOR_GREEN "Thanks for using MyShell!" COLOR_RESET "\n");
  printf("You executed %d command(s) in this session.\n", command_count);
  printf("Goodbye! 👋\n\n");

  return 0;
}