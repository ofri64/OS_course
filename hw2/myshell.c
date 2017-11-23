#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/prctl.h>

#define ERROR_EXEC_PROG "Error running program: %s\n"
#define AMPER_NOT_AT_END "This shell program only supports '&' character at the end of the executing command\n"
#define WRONG_PIPE_LOC "This shell program does not support using '| operator at the begin or the end of the executing command\n"
#define UNEXPECT_ERROR "The was an unexpected error. Your specified program could not be executed. Please try again\n"
#define ERROR_HANDLER_ASSIGN "Error: Could not assign an interrupt handler %s\n"
#define ERROR_RETURN_ORIG_HANDLER "Error: Could not undo interrupt handler assignment %s\n"
#define ERROR_PROCESS_NOT_OPEN "Could not open new process %s\n"


// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should cotinue, 0 otherwise
int process_arglist(int count, char** arglist);

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void);

int finalize(void);

int execInBackground(int count, char **arglist);

int pipeArgumentLocation(int count, char **arglist);

int runProcess(char** argsList);

int runPipeProcess(char** inputArgsList, char** outputArgsList);

void emptySignalHandler(int signum, siginfo_t* info, void* ptr);

int assignEmptyHandlerToInterrupt(struct sigaction *currentSignalHandler, struct sigaction *newSignalHandler);

int restoreInterruptHandler(struct sigaction* originalSignalHandle);

int runBackgroundProcess(char** argsList);

void parentDeathSignalHandler(int signum, siginfo_t* info, void* ptr);

int assignParentDeathHandler(struct sigaction *currentSignalHandler, struct sigaction *newSignalHandler);


int main(void)
{
    if (prepare() != 0)
        exit(-1);

    while (1)
    {
        char** arglist = NULL;
        char* line = NULL;
        size_t size;
        int count = 0;

        if (getline(&line, &size, stdin) == -1) {
            free(line);
            break;
        }

        arglist = (char**) malloc(sizeof(char*));
        if (arglist == NULL) {
            printf("malloc failed: %s\n", strerror(errno));
            exit(-1);
        }
        arglist[0] = strtok(line, " \t\n");

        while (arglist[count] != NULL) {
            ++count;
            arglist = (char**) realloc(arglist, sizeof(char*) * (count + 1));
            if (arglist == NULL) {
                printf("realloc failed: %s\n", strerror(errno));
                exit(-1);
            }

            arglist[count] = strtok(NULL, " \t\n");
        }

        if (count != 0) {
            if (!process_arglist(count, arglist)) {
                free(line);
                free(arglist);
                break;
            }
        }

        free(line);
        free(arglist);
    }

    if (finalize() != 0)
        exit(-1);

    return 0;
}


int process_arglist(int count, char** arglist){

    // in case of background process that already finished their run - remove them from zombie status
    bool zombiesExists = true;
    while (zombiesExists){
        pid_t p = waitpid(-1, NULL, WNOHANG);
        if (p == -1){
            zombiesExists = false;
        }
        else if (p == 0){
            // child process is still running
            break;
        }
    }

    int execBackground = execInBackground(count, arglist);
    int pipeLocation = pipeArgumentLocation(count, arglist);
    //error handling for unsupported inputs
    if (execBackground == -1){
        printf(AMPER_NOT_AT_END);
        return 1; }
    if (pipeLocation == -1){
        printf(WRONG_PIPE_LOC);
        return 1; }

    if (execBackground == 1){
        // remove the "&" from the arguments
        arglist[count-1] = (char*) NULL;

        runBackgroundProcess(arglist);
        return 1;
    }

    else if(execBackground == 0) {

        // assign the new signal handler with the pointer to the empty handler function
        struct sigaction currentSignalHandler;
        struct sigaction newSignalHandler;
        memset(&newSignalHandler, 0, sizeof(newSignalHandler));
        memset(&currentSignalHandler, 0, sizeof(currentSignalHandler));

        int ret = assignEmptyHandlerToInterrupt(&currentSignalHandler, &newSignalHandler);
        if (ret == 0) {
            return 1;
        }

        if (pipeLocation > 0){
            // run two process communicating using nameless pipe redirect first output to second's input

            // update the arglist array and split to two part
            arglist[pipeLocation] = (char *) NULL;
            char **inputArgs = arglist;
            char **outputArgs = arglist + pipeLocation + 1;

            runPipeProcess(inputArgs, outputArgs);

        }
        else{
            // run a single process and wait for it to end
            pid_t sonPid = runProcess(arglist);
            if (sonPid != -1){
                int execFinishStatus = -1;
                waitpid(sonPid, &execFinishStatus, 0);
            }
        }

        //restore handler from before

        ret = restoreInterruptHandler(&currentSignalHandler);
        if (ret == 0){
            // Not being able to restore the previous SIGINT handler is critical for controlling the program's behaviour
            // As a consequence the program must be shut down gracfuly
            return 0;
        }

    }

    else{
        // This error should not happen in any case, if so it is a bug in the program.
        // print a message to the user and continue the loop
        printf(UNEXPECT_ERROR);
        return 1;
    }

    //flow ended normally - continue to listen to user's commands
    return 1;
}

int prepare(void){return 0;}

int finalize(void){
    while (-1 != wait(NULL));
    return 0;
}

int execInBackground(int count, char** arglist){
    for (int i = 0; i < count; i++){
        if (strcmp(arglist[i], "&") == 0){
            if (i == count-1) return 1; // ampersand is last argument
            else return -1; // ampersand as a middle argument (assuming no more than a single pip)
        }
    }
    return 0; // no ampersand - do not run in background
}

int pipeArgumentLocation(int count, char **arglist){
    for (int i = 0; i < count; i++){
        if (strcmp(arglist[i], "|") == 0){
            if (i == 0 || i == count-1) return -1; //error - pipe in first or last argument
            else return i;
        }
    }
    return 0; // 0 if non pipe found
}

int runProcess(char** argsList) {
    pid_t execPid = fork();

    if (execPid == -1) {
        printf(ERROR_PROCESS_NOT_OPEN, strerror(errno));
        return -1;
    }

    if (execPid == 0) {
        int execRunStatus = execvp(argsList[0], argsList);

        if (execRunStatus == -1) {
            printf(ERROR_EXEC_PROG, strerror(errno));
            exit(1);
        }
    }

    // only executes in father process
    return execPid;
}

int runBackgroundProcess(char** argsList){
    pid_t execPid = fork();

    if (execPid == -1) {
        printf(ERROR_PROCESS_NOT_OPEN, strerror(errno));
        return -1;
    }

    if (execPid == 0) {

        // cancel interrupt signal - process won't end if an interrupt signal will send just non background son process
        signal(SIGINT, SIG_IGN);

        // assigns a new user defined signal for parent kill event
        prctl(PR_SET_PDEATHSIG, SIGUSR1);

        // assign a mathcing signal handler - end background process if shell program (father) is killed
        struct sigaction currentSignalHandler;
        struct sigaction newSignalHandler;
        memset(&newSignalHandler, 0, sizeof(newSignalHandler));
        memset(&currentSignalHandler, 0, sizeof(currentSignalHandler));

        int ret = assignParentDeathHandler(&currentSignalHandler, &newSignalHandler);
        if (ret == 0) {
            return 1;
        }

        int execRunStatus = execvp(argsList[0], argsList);
        if (execRunStatus == -1) {
            printf(ERROR_EXEC_PROG, strerror(errno));
            exit(1);
        }
    }

    // only executes in father process
    return execPid;
}

int runPipeProcess(char** inputArgsList, char** outputArgsList) {

    int pipefd[2];
    int firstPid;
    int secondPid = -100;

    pipe(pipefd);


    // fork for input part of pipe command
    firstPid = fork();
    if (firstPid == 0) {

        //redirect stdout fd to write side of pipe and close read side.
        int originalStdout = dup(1);
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        int status = execvp(inputArgsList[0], inputArgsList);
        if (status == -1) {

            // undo the redirection - fd 1 will point again to stdout
            dup2(originalStdout, 1);
            printf(ERROR_EXEC_PROG, strerror(errno));
            exit(1);
        }
    }
    // close the write side of pipe at parent process - send EOF to pipe and enable reading
    close(pipefd[1]);

    // fork for output part of pipe command
    secondPid = fork();
    if (secondPid == 0) {

        //redirect stdin fd to read side of pipe and close write side
        dup2(pipefd[0], 0);
        close(pipefd[1]);
        int status = execvp(outputArgsList[0], outputArgsList);
        if (status == -1) {
            printf(ERROR_EXEC_PROG, strerror(errno));
            exit(1);
        }
    }

    close(pipefd[0]);

    // wait for both processes to finish before returning (cannot combine & with |)
    int status;
    bool firstFinish = false;
    bool secondFinish = false;
    pid_t son;

    while (firstFinish == false || secondFinish == false) {
        son = wait(&status);

        if (son == firstPid) {
            firstFinish = true;
        } else if (son == secondPid) {
            secondFinish = true;
        } else if (son == -1) {
            break;
        }
    }
    return 1;
}

void emptySignalHandler(int signum, siginfo_t* info, void* ptr) {}

void parentDeathSignalHandler(int signum, siginfo_t* info, void* ptr){
    exit(1);
}

int assignParentDeathHandler(struct sigaction *currentSignalHandler, struct sigaction *newSignalHandler){
    newSignalHandler->sa_sigaction = parentDeathSignalHandler;
    newSignalHandler->sa_flags = SA_SIGINFO;
    if( 0 != sigaction(SIGUSR1, newSignalHandler, currentSignalHandler) ){
        printf(ERROR_HANDLER_ASSIGN, strerror(errno));
        return 0;
    }
    else{
        return 1;
    }
}

int assignEmptyHandlerToInterrupt(struct sigaction *currentSignalHandler, struct sigaction *newSignalHandler){
    newSignalHandler->sa_sigaction = emptySignalHandler;
    newSignalHandler->sa_flags = SA_SIGINFO;
    if( 0 != sigaction(SIGINT, newSignalHandler, currentSignalHandler) ){
        printf(ERROR_HANDLER_ASSIGN, strerror(errno));
        return 0;
    }
    else{
        return 1;
    }
}

int restoreInterruptHandler(struct sigaction* originalSignalHandle){
    if( 0 != sigaction(SIGINT, originalSignalHandle, NULL) ){
        printf(ERROR_RETURN_ORIG_HANDLER, strerror(errno));
        printf("Program will have to terminate\n");
        return 0;
    }
    else{
        return 1;
    }
}