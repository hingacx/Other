/* DESCRIPTION: A project that implements a working version of a UNIX Shell. Not quite the same as UNIX or LINUX in terms of usability.
*
* NOTABLE IMPLEMENTATIONS: 
* - Built in Commands: exit, status, cd.
* - Other commands: Forked child processes.
* - "$$" is expanded to be the working pid of the shell whenever it's presented.
* - & at the end of a command line tells the program to run it as a background process.
* - Customized CTRL^C and CTRL^Z signals. Because of this, type in "exit", if you need to exit the program.
* - Supports commands lengths of 2048 characters and up to 512 arguments.
* - Can run up to 1000 child processes before needing to restart the program.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>


// Flag used with CTRL^Z, when 1 is active, background commands are allowed.
int bgIgnore = 1;


/* Built in command, changes the directory. */
void cd(char* line) {

    // Gets the command line length, if len is 3, then the user input was just "cd\0", otherwise there are two arguments.
    int len = strlen(line);

    if (len == 3) {

        // Changes the directory to the home environment.
        if (chdir(getenv("HOME")) < 0) {
            perror("chdir()");
        } 
        
    } else {
        // Removing the first command "cd".
        char *newPath = strtok(line, " \n");

        // Getting the second token of the command line which will be the desired path.
        newPath = strtok(NULL, " \n");
        
        if (chdir(newPath) < 0) {
            perror("chdir()");
        } 

    }
}


/* Built in command, prints the exit status or termination signal of the last
   foreground process ran, used/modified from lectures.*/
void status(int status) {

    // Exited normally.
    if (WIFEXITED(status)) {
        printf("exit value %d\n", WEXITSTATUS(status));

    // Exited via signal.
    } else {
        printf("terminated by signal %d\n", WTERMSIG(status));
    }
}


/* Built in command, exits the program and terminates the processes. */
void exitCMD(int pidArray[]) {

    int i;
    for (i = 0; i < 1000; i++) {
        // If there is a PID value in the index, attempts to kill it if it's not terminated already.
        if (pidArray[i] > -1) {
            kill(pidArray[i], SIGKILL);
        }
    }

    exit(2);
}


/* Checks for expansion of the command line.*/
void expandInput(char input[]) {

    /* Get the PID from smallsh and convert it to string format.*/
    char pid_str[10];
    int pid = getpid();
    sprintf(pid_str, "%d", pid);

    // Getting the length of the PID in string format.
    int pid_len = strlen(pid_str);

    int i;
    int count = 0;

    /* Finding the instances of $ so we can calculate how much theoretical
        space we need to add to a new buffer to expand "$$". */

    // Getting the count of $ instances in the command line.
    for (i = 0; i < strlen(input)-1; i++) {
        if (input[i] == '$') {
            count++;
        }
    }

    // Checks to see if an expansion is needed.
    if (count > 1) {

        // Length of command line.
        int inputLen = strlen(input);

        // The pid number is replace two instances of "$", so for every "$$", we allocate enough space for one string of pid.
        int newCount = (count/2);
        int len = (newCount * pid_len);

        // The final input after expansion is the original input length plus the instances of "$$" being replaced.
        int bufferLen = (len + inputLen + 1);

        // This is the new temp input buffer.
        char inputBuffer[bufferLen];
        int j;
        int k = 0;
        int s;

        for (j = 0; j < inputLen-1; j++) {

            /* If we find back to back instances of "$", then we replace them with the pid number.
                we do an extra increment of the input string/command line so we don't double visit
                the second '$' in "$$". */
            if (input[j] == '$' && input[j+1] == '$') {

                // Advances the input buffer and sets the char to be a char from the pid array.
                for (s = 0; s < pid_len; s++) {
                    inputBuffer[k] = pid_str[s];
                    k++;
                }
                j++;

                // Reset the pointer of pid array incase there are multiple instances of "$$".
                s = 0;

            // Sets the input buffer char to be the input char and moves on.
            } else {
                inputBuffer[k] = input[j];
                k++;
            }

        }

        // Makes sure to end the input buffer with the NULL terminator.
        inputBuffer[k] = '\0';

        // Copies the contents from the input buffer to the orignal input with everything expanded.
        strcpy(input, inputBuffer);
    } 

}


/* Executes a command via fork, exe and waitpid. */
void exeCMD(char *args[], int bg, char inputFile[], char outputFile[], struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, int* exitStatus, int pidArray[], int *count) {
    

    /* Fork a child process, similar to the lectures from class Excuting A New Program. */
    int childStatus;
    pid_t spawnPid = fork();

    switch(spawnPid) {
    case -1:
        perror("fork()\n");
        exit(1);
        break;

    // Child process is successfull for now.
    case 0:

        // Child proceses ignore ctrl^z.
        SIGTSTP_action.sa_handler = SIG_IGN;
        sigaction(SIGTSTP, &SIGTSTP_action, NULL);

        if (bg == 1) {

            // Cancel/Interrupt forground process when us CTRL^C.
            SIGINT_action.sa_handler = SIG_DFL;
            sigaction(SIGINT, &SIGINT_action, NULL);
 
        } else if (bg == 0) {

            // Background processes ignore CTRL^C.
            SIGINT_action.sa_handler = SIG_IGN;
            sigaction(SIGINT, &SIGINT_action, NULL);
        }
        

        // If the inputfile has changed its starting string, need to parse inputfile.
        if (strcmp(inputFile, "standard") != 0) {
            
            // Checks to see if the command is ran in the background. Redirects to "/dev/null" if so.
            if (bg == 0) {

                int inputFD = open("/dev/null", O_RDONLY);
                if (inputFD == -1) {
                    perror("input open() bg");
                    exit(1);    
                }

            /* Regular foreground redirection with dup2()
                Modified from lectures Processes and I/O */
            } else {
                
                int inputFD = open(inputFile, O_RDONLY);
                if (inputFD == -1) {
                    perror("input open() fg");
                    exit(1);    
                }

                int result = dup2(inputFD, 0);
                if (result == -1) {
                    perror("input dup2() fg");
                    exit(2);
                }
            }
        }

        // If the outputfile has changed its starting string, need to parse the outputfile.
        if (strcmp(outputFile, "standard") != 0) {
            
            // Checks to see if the command is ran in the background. Redirects to "/dev/null" if so.
            if (bg == 0) {

                int outputFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (outputFD == -1) {
                    perror("output open() bg");
                    exit(1);
                }

            /* Regular foreground redirection with dup2()
                Modified from lectures Processes and I/O */
            } else {
                
                int outputFD = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (outputFD == -1) {
                    perror("output open() fg");
                    exit(1);
                }
                
                int result2 = dup2(outputFD, 1);
                if (result2 == -1) {
                    perror("output dup2() fg");
                    exit(2);
                }
            }
        }

        // Executes the new program with the forked child process.
        execvp(args[0], args);
        perror("evecvp\n");
        exit(2);
        break;

    default:
        // Checks to see if we want to run the command in the background and if background commands are currently allowed.
        if (bg == 0 && bgIgnore == 1) {
            //Adds the child pid to the pidArray.
            pidArray[*count] = spawnPid;
            *count = *count + 1;

            //Prints child pid of the background process
            printf("background pid is %d\n", spawnPid);
            fflush(stdout);
            waitpid(spawnPid, &childStatus, WNOHANG);

        // Otherwise we run in the forground.
        } else {
            //Adds the child pid to the pidArray.
            pidArray[*count] = spawnPid;
            *count = *count + 1;
	        waitpid(spawnPid, &childStatus, 0);

            // If there is an interuption such as CTRL^C.
            if (childStatus == 2) {
                status(childStatus);
            }

        }
    }

    // Keeps checking to see if the background process is done.
    while ((spawnPid = waitpid(-1, &childStatus, WNOHANG)) > 0) {
        // Prints child pid of the completed process.
        printf("background pid %d is done: ", spawnPid);
        fflush(stdout);
        // Calls the built-in status function to get the exit status.
        status(childStatus);
    } 

    // Updates the exitstatus to be the childstatus.
    *exitStatus = childStatus;


}


/* Parses the command line input into invidual arguments */
void parseInput(char input[], char *args[], int *bg, char inputFile[], char outputFile[]) {
    
    // Break the command line into singular arguments.
    char *token = strtok(input, " \n");
    int i = 0;

    // Need to account for < > eventually.
    while (token != NULL) {
        

        // Determining if the argument(s) is/are input and output files.
        // This is the case for input files. Copy the token name to be the input file.
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " \n");
            strcpy(inputFile, token);
            token = strtok(NULL, " \n");

        // This is the case for output files. Copy the token name to be the output file.
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " \n");
            strcpy(outputFile, token);
            token = strtok(NULL, " \n");

        // Add regular arguments to args and increment the args array.
        } else {
            args[i] = token;
            i++;
            token = strtok(NULL, " \n");
        }

    // Checking to see if the command is to be executed in the background.
    if (token == NULL) {
        if (strcmp(args[i-1], "&") == 0) {
            *bg  = 0;
            args[i-1] = NULL;
        }
    }

    }
}



/* Custom signal handler that handles CTRL^Z modifed from the lectures
   Signal Handling API. bgIgnore is a GLOBAL flag. */
void SIGTSTP_handler(int signo) {

    // If bgIgnore is 1, that means background commands are allowed. Swapping to 0 turns them off.
	if (bgIgnore == 1) {
        bgIgnore = 0;
        char *message = "Entering forground-only mode (& is now ignored) \n";
        write(STDOUT_FILENO, message, 50);
        fflush(stdout);
	}

    // If bgIgnore is 0, that means background commands are ignored. Swapping to 1 turns them on.
	else {
        bgIgnore = 1;
        char *message = "Exiting foreground-only mode \n";
        write(STDOUT_FILENO, message, 31);
        fflush(stdout);
	}
}



int main() {
    // Max chars in a command line.
    const int maxChars = 2048;
    // Max arguments in a command line.
    const int maxArg = 512;

    // Count used to keep track of the idx within the pidArray.
    int count = 0;
    // pidArray holds child processes of the forked() processes.
    int pidArray[1000];

    // Give each index the value 0 because pid won't have a negative value.
    int j;
    for (j = 0; j < 1000; j++) {
        pidArray[j] = -1;
    }


    // Starting exit status of processes.
    int exitStatus = 0;

    // Background flag.
    int bg = 1;


    /* Create custom sigaction structs and handlers*/
    struct sigaction SIGINT_action = {{0}}, SIGTSTP_action = {{0}};

    // Set up the action handler for the SIGINT signal which is ctrl^C.
    SIGINT_action.sa_handler = SIG_IGN;  
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // Set up the action handler for the SIGTSTP signal which is ctrl^Z.
	SIGTSTP_action.sa_handler = SIGTSTP_handler;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);



    while(1) {
        char buffer[maxChars];
        char *args[maxArg];

        // Set the input and output file to be a string called "standard". 
        char inputFile[2048] = "standard";
        char outputFile[2048] = "standard";

        // Sets the index's to NULL to stop bugs down the line.
        int i;
        for (i = 0; i < maxArg; i++) {
            args[i] = '\0';
        }

        printf(": ");
        fflush(stdout);
        fgets(buffer, maxChars+1, stdin);

        // Check for a empty string or a comment on the command line.
        if (strncmp(buffer, " ", 1) == 0 || strncmp(buffer, "#", 1) == 0) {
            continue;
        
        } else {
            
            // Checks for expansion of "$$".
            expandInput(buffer);

            // Exits the program and terminates the processes.
            if (strncmp(buffer, "exit", 4) == 0) {
                exitCMD(pidArray);

            // Shows the status of the last foreground process ran.
            } else if (strncmp(buffer, "status", 6) == 0) {
                status(exitStatus);

            // Enters the environment change function.
            } else if (strncmp(buffer, "cd", 2) == 0) {
                cd(buffer);

            } else {
                // Break the command line into arguments.
                parseInput(buffer, args, &bg, inputFile, outputFile);
                
                // Execute the command or tries to and updates the exit status.
                exeCMD(args, bg, inputFile, outputFile, SIGINT_action, SIGTSTP_action, &exitStatus, pidArray, &count);

                // Reset background flag.
                bg = 1;
            }
        }
    }

    return 0;
}