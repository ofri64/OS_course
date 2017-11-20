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

void replacePipeArg(char** argList, int pipeLocation);

int runProcess(char** argsList);

int runProcessMode(bool backgroundRun, char** argsList);

int runPipeProcess(int* pipfd, char** argList, int processType);

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


    if (pipeLocation > 0){

        int pipefd[2];
        int firstPid;
        int secondPid;

        char *cat_args[] = {"cat", "scores", NULL, NULL};
        char *grep_args[] = {"grep", "hi", NULL};
        char *python[] = {"python", "print_args.py", "hi", NULL};

        // make a pipe (fds go in pipefd[0] and pipefd[1])

        pipe(pipefd);

        firstPid = fork();

        if (firstPid == 0)
        {
            // child gets here and handles "grep Villanova"

            // replace standard input with input part of pipe

            dup2(pipefd[0], 0);

            // close unused hald of pipe

            close(pipefd[1]);

            // execute grep

            execvp("grep", grep_args);
        }

        secondPid = fork();

        if (secondPid == 0){
            // parent gets here and handles "cat scores"

            // replace standard output with output part of pipe

            dup2(pipefd[1], 1);

            // close unused unput half of pipe

            close(pipefd[0]);

            // execute cat

            execvp("python", python);
        }

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
    int status = -1;
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


void replacePipeArg(char** argList, int pipeLocation){
    argList[pipeLocation] = (char*) NULL;
}


int runProcess(char** argsList) {
    pid_t execPid = fork();

    if (execPid == -1) {
        //TODO: implement error handling
        return -1;
    }

    if (execPid == 0) {
        int execRunStatus = execv(argsList[0], argsList);

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


int runPipeProcess(int* pipfd, char** argList, int processType){
    int pid = fork();
    if (pid < 0 ){
        //TODO: handle error - return from function and raise error
    }

    if (pid == 0){
        if (processType == 0){
            // for input son process replace stdout with pipe write side and run program

            close(pipfd[0]);

            dup2(pipfd[1], 1);


        } else {
            // for output son process replace stdin with pipe read side and run program

            close(pipfd[1]);

            dup2(pipfd[0], 0);
        }

        int s = execvp(argList[0], argList);
    }

    // father process waits for the input to return
    if (pid > 0){

        close(pipfd[1]);
        int status;
        pid_t retPid = waitpid(pid, &status, 0);
        if (retPid != -1) {
            //TODO: implement error
        }
        return 1;
    }
}
