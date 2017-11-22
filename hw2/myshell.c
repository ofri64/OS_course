#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

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
    

    int execBackground = execInBackground(count, arglist);
    int pipeLocation = pipeArgumentLocation(count, arglist);
    //TODO: check for errors from return function - what happens with return value -1

    if (execBackground == 1){
        // remove the "&" from the arguments
        arglist[count-1] = (char*) NULL;

        pid_t sonPid = runProcess(arglist);
        if (sonPid == -1){
            //TODO: some error process didn't run or maybe not needed
        }
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
            //TODO: handle error
            return 1;
        }

        if (pipeLocation > 0){
            // run two process communicating using nameless pipe redirect first output to second's input

            // update the arglist array and split to two part
            arglist[pipeLocation] = (char *) NULL;
            char **inputArgs = arglist;
            char **outputArgs = arglist + pipeLocation + 1;

            int status = runPipeProcess(inputArgs, outputArgs);
            if (status != 1){
                //TODO:: implement error
            }
        }
        else{
            // run a single process and wait for it to end
            pid_t sonPid = runProcess(arglist);
            if (sonPid == -1){
                //TODO:: implement error
            }

            int execFinishStatus = -1;
            pid_t finishPid = waitpid(sonPid, &execFinishStatus, 0);
        }

        //restore handler from before

        ret = restoreInterruptHandler(&currentSignalHandler);
        if (ret == 0){
            //TODO: handle error
        }
        return 1;
    }

    else{
        //TODO: error that should not happen. print error and contiue
        return 1;
    }
}

int prepare(void){return 0;}

int finalize(void){
//    int status;
//    while (-1 != wait(&status)) {};
    return 0;
    //TODO: implement wait for all sons to prevent zombies.
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
        //TODO: implement error handling
        return -1;
    }

    if (execPid == 0) {
        int execRunStatus = execvp(argsList[0], argsList);

        if (execRunStatus == -1) {
            printf("Error running program %s\n", strerror(errno));
            exit(1);
        }
    }

    // only executes in father process
    return execPid;
}

int runPipeProcess(char** inputArgsList, char** outputArgsList) {

    int pipefd[2];
    int firstPid;
    int secondPid;

    pipe(pipefd);

    int errorInputProcess[1] = {0};

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
            printf("Error running program %s\n", strerror(errno));
            *errorInputProcess = 1;
            exit(1);
        }
    }
    // close the write side of pipe at parent process - send EOF to pipe and enable reading
    close(pipefd[1]);

    if (!*errorInputProcess) {

        // fork for output part of pipe command
        secondPid = fork();
        if (secondPid == 0) {

            //redirect stdin fd to read side of pipe and close write side
            dup2(pipefd[0], 0);
            close(pipefd[1]);
            int status = execvp(outputArgsList[0], outputArgsList);
            if (status == -1) {
                printf("Error running program %s\n", strerror(errno));
                exit(1);
            }
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
    //TODO: check for error options
}

void emptySignalHandler(int signum, siginfo_t* info, void* ptr) {}

int assignEmptyHandlerToInterrupt(struct sigaction *currentSignalHandler, struct sigaction *newSignalHandler){
    newSignalHandler->sa_sigaction = emptySignalHandler;
    newSignalHandler->sa_flags = SA_SIGINFO;
    if( 0 != sigaction(SIGINT, newSignalHandler, currentSignalHandler) ){
        //TODO:: handle error
        printf("error");
        return 0;
    }
    else{
        return 1;
    }
}

int restoreInterruptHandler(struct sigaction* originalSignalHandle){
    if( 0 != sigaction(SIGINT, originalSignalHandle, NULL) ){
        //TODO:: handle error
        printf("error");
        return 0;
    }
    else{
        return 1;
    }
}