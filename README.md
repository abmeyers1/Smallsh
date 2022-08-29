# SmallShell program by Alex Meyers

Features for this shell program include: 
- Provide a prompt for running commands
- Handle blank lines and comments, which are lines beginning with the # character
- Provide expansion for the variable $$ into the current processID
- Execute 3 commands exit, cd, and status via code built into the shell
- Execute other commands by creating new processes using a function from the exec family of functions
- Support input and output redirection with "<" and ">"
- Support running commands in foreground and background processes
- Implement custom handlers for 2 signals: Ctrl+C will be ignored, and Ctrl+Z will enable/disable background processes

Instructions for compiling:

- Copy smallsh.c to Unix machine
- Compile with:  gcc -std=gnu99 smallsh.c -o smallsh
- Run with: ./smallsh
