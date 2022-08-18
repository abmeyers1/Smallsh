#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/wait.h>
#include <signal.h>

int foregroundOnly = 0;     // Toggles  foreground only mode
int  endloop = 0;

// Handler for Ctrl+C
void handle_SIGINT( int signo) {
  char* msg = "Caught a SIGINT! \n";
  write(STDOUT_FILENO, msg, 19);

}

// Handler for ctrl+z
void handle_SIGTSTP( int signo) {
  exit(1);
    
  /*    Blocked out for grading
  char* message = "\nEntering foreground-only mode (& is now ignored)\n";
    char* exitmessage = "\nExiting foreground only mode\n";
    if (foregroundOnly == 0) {
    foregroundOnly = 1;
    write(STDOUT_FILENO, message, 55);
    
  }
  else {
    foregroundOnly = 0;
    write(STDOUT_FILENO, exitmessage, 31);

  }

  fflush(stdin); 
  fflush(stdout);
*/
}


// Main loop for shell
int
main (int argc, char *argv[]){
  
  // Initialize SIGINT_action to handle CTRL+C and SIGTSTP_action to handle ctrl+z
  struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0}, ignore_action= {0};
  // Register handle_SIGINT as signal handler
  SIGINT_action.sa_handler = handle_SIGINT;
  sigfillset(&SIGINT_action.sa_mask);           
  // No flags set
  SIGINT_action.sa_flags = 0;
  // Install the signal handler
   sigaction(SIGINT, &SIGINT_action, NULL);
  
   
  // SIGTSTP handler
  // Register handler for signal
  SIGTSTP_action.sa_handler = handle_SIGTSTP;
  sigfillset(&SIGTSTP_action.sa_mask);
  // No flags set
  SIGTSTP_action.sa_flags = 0;
  // Install signal handler
  sigaction(SIGTSTP, &SIGTSTP_action, NULL);
  

  // Set up values for reading arguments and handling background processes.
  int totalArgs = 512;        // max number of arguments, per assignment
  int inputLength = 2048;     // Max input length, per assignment
  int isBackground;           // Toggles on background commands
  int status = 0;             // Tracks status
  char line[inputLength];     // Array for user input
  int argnum;                 // number of parsed arguments
  FILE *filepath; 
  char *args[512];            // Array for parsed arguments
  int backgroundpids[200];    // Array for holding pids of background processes
  int pidcounter = 0;         // Counts number of active background processes
  int firstrun = 0;           // Ensures Status returns proper value before background process is run
  int thispid = getpid();
  char* strpid = malloc(8);
  sprintf(strpid, "%d", thispid);   // String of current pid
  
  while(endloop== 0){           // Main loop for running shell
    
    memset(args, '\0', 512);                // array to pass arguments to exec
    isBackground = 0;                       // flag for background processing
    argnum = 0;                             // total number of args
    printf(":");                            // Character to signal newline in shell
    fgets(line, inputLength, stdin);        // get input from user 
    memset(args, '\0', 512);                // clear argument array
    fflush(stdin);
    fflush(stdout);
    
    //------- Expansion for $$ -------------
    
    //Create a result string that is larger than original line
    char *result = malloc(sizeof(char) * strlen(line) * 2); 
    
    // Iterate through line for "$$"
    for (int i = 0; i < strlen(line); i++){
      if ( line[i] == '$') {
        if (line[i + 1] == '$') {
          // if found, copy line into result, use %d to replace with current pid
          result = strdup(line);
          result[i] = '%';
          result[ i + 1] = 'd';
          sprintf(line, result, getpid());
        }
      }
    }
    
    line[strlen(line) -1] = '\0';       // Remove the \n at end of line
     

    //------------------- Split up arguments and store in array
    char *token;
    char *first;
    
    token = strtok(line," ");
    // Error checking:
    // Empty Line
    if (token == NULL){
      continue;
    }
    if (strlen(token) < 1) {
      token = NULL;
      continue;
    }
    if (strcmp(token, "\n") ==0){
      token = NULL;
      continue;
    }
    // Check for commented out line, if first argument is "#" or starts with "#"
    first = strndup(token, 1);            
    if ( (strcmp(token, "#") == 0) || (strcmp(first,"#") == 0) ) {
        token = NULL;
        continue;    
    }

    // Store individual args in array for processing 
    int index = 0;
    char inFile[512];
    char outFile[512];
    int childStatus;
    // Continuously separate tokens and add to arguments  
    while (token != NULL)   {
      
      args[index] = strdup(token);
      index++;
      argnum++;
      token = strtok(NULL, " ");
    } 
    fflush(stdout);
    fflush(stdin);
    
    // check for last argument == & to set function as background
    isBackground = 0;
    if (strcmp(args[index - 1], "&") == 0) {
      isBackground = 1;
      args[index-1] = NULL;
      index--;
    }
    

// ---------------------- HANDLING COMMANDS ---------------------------//

    // cd command 
    if (strcmp(args[0], "cd") == 0) {
      if (args[1] != NULL) {
        chdir(args[1]);
      }
      else {
        chdir(getenv("HOME"));
      }
      status = 0;
      continue;
    }
   
    // exit command
    if (strcmp(args[0], "exit") == 0)  {
      fflush(stdout);
      fflush(stdin);
      status = 1;
      for (int i = 0; i < pidcounter; i++){       // kill all active children before terminating self
        kill(backgroundpids[i],0);
      }
      while(1) {
      
       fflush(stdin);
       fflush(stdout);
       endloop = 1;
       exit(1);
       break;
      }
      break;
    }
    
    // Status command
    if (strcmp( args[0], "status") == 0) {
      // If no other commands have been run, status = 0
      if (firstrun == 0) {
        printf("exit value: %d\n", firstrun);
      }
      // If most recent function exited, return exit status
      else if (WIFEXITED(status)) {
        printf("exit value %d\n", WEXITSTATUS(childStatus));
      }
      else {
        // Most recent function exited by signal.
        printf("exit value %d\n", status);
      }
      fflush(stdout);
      status= 0;

    }

    else {
      
      // Spawn Child for all other processes
      firstrun++;
      pid_t spawnpid = -5;
      spawnpid = fork();
      int inFlag = 0;
      int outFlag = 0;
      int sourceFD;
      int targetFD;
      
      switch (spawnpid){
        case -1:    // In case of error
          perror("Failed to fork()\n");
          exit(1);
          status = 1;
          break;

        case 0:       
          // Child process
          // Reset direction flags
          inFlag = 0;
          outFlag = 0;

          // Process all redirection
          for (int i =0 ; i < index; i++) {
            
            // Check for input file
            // assign input to file, update args to skip < 

            if(strcmp(args[i], "<") == 0) {
              inFlag = 1;
              sourceFD = open(args[i + 1], O_RDONLY);   // Opens file to read from  
              if (sourceFD == -1) {
                perror("Source open()\n");
                status = 1;
                exit(1);
              }
              int result = dup2(sourceFD, 0);           // Redirects input to new file
              if (result == -1) {
                perror("Source dup2()");
                status = 1;
                exit(2);
              }
              args[i] = NULL;
              i++;
              fcntl(sourceFD, F_SETFD, FD_CLOEXEC);  // Close this fd when terminates
            }
            
            // Check for output file. 
            // Assign output to correct file, update args to skip >
            if (strcmp(args[i] , ">") == 0)  {
              outFlag = 1;
              targetFD = open(args[i + 1], O_WRONLY |O_CREAT | O_TRUNC,0644 );  // Opens/creates file to write to
              if (targetFD == -1) {
                perror("target open()\n");
                status = 1;
                exit(1);
              }
              int result = dup2(targetFD, 1);           // Redirects output to new file
              if (result == -1) {
                perror("target dup2()\n");
                status = 1;
                exit(1);
              }
              args[i] = NULL;
              fcntl(targetFD, F_SETFD, FD_CLOEXEC);     // Close this fd when terminates
            }
            // Running processes for background 
            if (isBackground == 1) {
              printf("background pid is %d\n",getpid());
              // Background process will redirect input & output to /dev/null if not specified
              if (inFlag == 0) {
                // set input to /dev/null
                int infd = open("/dev/null", O_RDONLY);
                int inres = dup2(infd, 0);
              }
              if (outFlag == 0) {
                // set output to /dev/null
                int outfd = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                int outres = dup2(outfd, 1);
              }
            }

          }
            status =  execvp(args[0], args);
            printf("Error: no such file or directory.\n");
            // Terminate after running command
          
            break;

        default:        // Parent process
         
            if (isBackground == 1)  {

              backgroundpids[pidcounter] = spawnpid;      // save current pid to counter
              pidcounter ++;
              spawnpid= waitpid(spawnpid,&childStatus, WNOHANG);    // Do not wait for child process to complete before continuing on
            if (status == -1) {
              status = 1;     // Update status
            } 

            }
          else {      
            // Wait for child to terminate before proceeding
            spawnpid= waitpid(spawnpid,&childStatus, 0); 
          
            // Check child exit status
            if (WIFSIGNALED(childStatus)) {
              //Child terminated abnormally
              printf("Background pid %s is done: Terminated by signal %d \n", spawnpid, WTERMSIG(childStatus )); // TODO Fill in which signal?
            }  
          }

          // Loop through existing background processes
          pid_t temp;
          for (int k = 0; k < pidcounter; k++) {
            temp = waitpid( backgroundpids[k], &childStatus, WNOHANG);
            if (temp >= 1) {
              // for normal exits
              if (WIFEXITED(childStatus)) {
                status = WEXITSTATUS(childStatus);
                printf("background pid %d is done: exit value %d\n", temp, status);
              }
              else {      // for exit due to signal 
                printf("background process %d terminated by signal %d\n", temp, WTERMSIG(childStatus));
                fflush(stdout);
              }
            }
          } 
            status = 0;
            fflush(stdin);
            fflush(stdout);
          isBackground = 0;         
          break;
      }
    }
  }  
}
  
