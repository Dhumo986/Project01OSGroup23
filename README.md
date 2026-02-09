# Operating Systems Project 1 - Shell Implementation
**Group 23**

## Team Members
1. Aryan Patel
2. Jamie Velasquez
3. Dhruv Upadhyay

## Project Description
A fully functional UNIX shell supporting internal commands, piping, I/O redirection, background processing, environment variable expansion, and more.

## Features Implemented

### Part 1: Prompt ✅
- Displays `USER@MACHINE:PWD>` format
- Shows current working directory with tilde expansion for home

### Part 2: Environment Variable Expansion ✅
- Expands `$VAR` tokens (e.g., `$USER`, `$HOME`, `$PATH`)
- Works with all commands

### Part 3: Tilde Expansion ✅
- Expands `~` to home directory
- Supports `~/path` format

### Part 4: $PATH Search ✅
- Searches executable in $PATH directories
- Supports commands with full paths (e.g., `/bin/ls`)

### Part 5: External Command Execution ✅
- Executes external commands using fork() and execv()
- Proper argument handling

### Part 6: I/O Redirection ✅
- Output redirection: `cmd > file`
- Input redirection: `cmd < file`
- Combined: `cmd < input > output`
- Proper file permissions (0600 for output files)

### Part 7: Piping ✅
- Single pipe: `cmd1 | cmd2`
- Multiple pipes: `cmd1 | cmd2 | cmd3`
- Proper file descriptor management

### Part 8: Background Processing ✅
- Run commands in background: `cmd &`
- Job tracking with job numbers
- Completion notifications
- `jobs` command to list active jobs

### Part 9: Built-in Commands ✅
- `cd [dir]` - Change directory (supports `cd`, `cd ~`, `cd -`, `cd /path`)
- `pwd` - Print working directory
- `echo [text]` - Print text
- `exit` - Exit shell
- `help` - Show help menu
- `clear` - Clear screen
- `jobs` - List background jobs

## File Structure
```
Project01OS/
├── src/
│   └── myshell.c       # Complete shell implementation
├── bin/
│   └── shell           # Compiled executable
├── Makefile            # Build instructions
└── README.md           # This file
```

## How to Compile
```bash
make
```

This compiles `src/myshell.c` and places the executable in `bin/shell`.

## How to Run
```bash
./bin/shell
```

## How to Clean
```bash
make clean
```

## Division of Labor

### Planned Division (Before Implementation)

**Part 1: Prompt**
- Aryan Patel, Jamie Velasquez

**Part 2: Environment Variables**
- Aryan Patel, Dhruv Upadhyay

**Part 3: Tilde Expansion**
- Aryan Patel, Dhruv Upadhyay

**Part 4: $PATH Search**
- Jamie Velasquez, Aryan Patel

**Part 5: External Command Execution**
- Jamie Velasquez, Aryan Patel

**Part 6: I/O Redirection**
- Jamie Velasquez, Dhruv Upadhyay

**Part 7: Piping**
- Dhruv Upadhyay, Jamie Velasquez

**Part 8: Background Processing**
- Dhruv Upadhyay, Jamie Velasquez

**Part 9: Internal Command Execution**
- Dhruv Upadhyay, Aryan Patel

### Actual Implementation (After)

**Dhruv Upadhyay:**
- Integrated and completed all parts (1-9) into single working implementation
- Implemented Parts 1-6 (prompt, environment variables, tilde expansion, PATH search, external commands, I/O redirection)
- Implemented Parts 7-9 (piping, background processing, built-in commands)
- Created build system (Makefile)
- Structured project directories
- Comprehensive testing of all features
- Documentation and README

**Aryan Patel & Jamie Velasquez:**
- Contributed initial code modules for specific parts
- Provided code that was integrated into final implementation

## Testing

All parts (1-9) tested and verified working:
- ✅ Prompt format (USER@MACHINE:PWD>)
- ✅ Environment variables ($USER, $HOME, $PATH)
- ✅ Tilde expansion (~, ~/path)
- ✅ PATH search
- ✅ External commands with fork() and execv()
- ✅ I/O redirection (>, <, combined)
- ✅ Piping (single and multiple pipes)
- ✅ Background processing (& and jobs command)
- ✅ All built-in commands

## Implementation Notes

- Uses only fork() and execv() as required (no system() or execvp())
- Proper signal handling for SIGINT (Ctrl+C) and SIGCHLD (zombie cleanup)
- Memory management with proper cleanup
- Error handling for invalid commands and file operations
- Follows all project restrictions and requirements

## Known Limitations

- Maximum 10 background jobs supported (as per requirements)
- Piping and I/O redirection don't combine in single command (as per requirements)
- No glob expansion or regex support (not required)

## Compilation Requirements

- **Compiler:** gcc
- **Standard:** C99
- **Flags:** -Wall -Wextra
- **Platform:** Tested on macOS and Linux (linprog)

## GitHub Repository
https://github.com/Dhumo986/Project01OSGroup23

## Due Date
February 9, 2026, 11:59 PM

---

**Note:** This project successfully implements all required features (Parts 0-9) with proper documentation, build system, and project structure as specified in the rubric.
