#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>

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

int runProcessMode(bool backgroundRun, char** argsList);

int runPipeProcess(char** inputArgsList, char** outputArgsList);


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


    if (pipeLocation > 0) {

        // update the arglist array and split to two part
        arglist[pipeLocation] = (char *) NULL;
        char **inputArgs = arglist;
        char **outputArgs = arglist + pipeLocation + 1;

        int status = runPipeProcess(inputArgs, outputArgs);
        if (status != 1){
            //TODO:: implement error
        }
    }

    else {
        int excStatus;

        if (execBackground == 1){

        // remove the "&" from the arguments
            arglist[count-1] = (char*) NULL;
            excStatus = runProcessMode(true, arglist);
        }

        else {
            excStatus = runProcessMode(false, arglist);
        }

        if (excStatus == -1){
            //TODO: implement error handling for this case
        }
    }
    return 1;
}

int prepare(void){;}

int finalize(void){
    int status;
    while (-1 != wait(&status)) {};
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
            //TODO: implement error handling for process could not run
            return -1;
        }
    }

    // only executes in father process
    return execPid;
}

int runProcessMode(bool backgroundRun, char** argsList) {
    pid_t sonPid = runProcess(argsList);
    if (backgroundRun) {
        return 1;
    }
    else {
        int execFinishStatus = -1;
        pid_t finishPid = waitpid(sonPid, &execFinishStatus, 0);
        //TODO: implenment what to do when process already finished (call returns -1)
        return 1;
    }
}

int runPipeProcess(char** inputArgsList, char** outputArgsList){

    int pipefd[2];
    int firstPid;
    int secondPid;

    pipe(pipefd);

    // fork for input part of pipe command
    firstPid = fork();
    if (firstPid == 0){

        //redirect stdout fd to write side of pipe and close read side.
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        execvp(inputArgsList[0], inputArgsList);
    }
    // close the write side of pipe at parent process - send EOF to pipe and enable reading
    close(pipefd[1]);

    // fork for output part of pipe command
    secondPid = fork();
    if (secondPid == 0)
    {
        //redirect stdin fd to read side of pipe and close write side
        dup2(pipefd[0], 0);
        close(pipefd[1]);
        execvp(outputArgsList[0], outputArgsList);
    }

//    close(pipefd[0]);
    // wait for both processes to finish before returning (cannot combine & with |)
    int status;
    bool firstFinish = false;
    bool secondFinish = false;
    pid_t son;

    while (firstFinish == false || secondFinish == false){
        son = wait(&status);

        if (son == firstPid){
            firstFinish = true;

        } else if (son == secondPid){
            secondFinish = true;

        } else if(son == -1){
            break;
        }
    }
    return 1;
    //TODO: check for error options
}