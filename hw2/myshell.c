/*
 * Student Name: Ofri Kleinfeld
 * Student ID: 302893680
 *
 * This file is used to complete the building of a shell program.
 * The main function of this file is "process_arglist". rest of the functions are helper function.
 * The flow of the program is quiet simple:
 * - complete the parsing of the user arguments and check for '&' or '|' characters
 * - If '&' char is found, run the process in the background
 * - In any other case run the process in foreground
 *
 * - For foreground process:
 * - If '|' char is found, pipe the two programs from the left and right hand of the pipe
 * - If not, execute the given program.


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>

#define ERROR_EXEC_PROG "Error running program: %s\n"
#define AMPER_NOT_AT_END "This shell program only supports '&' character at the end of the executing command\n"
#define WRONG_PIPE_LOC "This shell program does not support using '| operator at the begin or the end of the executing command\n"
#define UNEXPECT_ERROR "The was an unexpected error. Your specified program could not be executed. Please try again\n"
#define ERROR_HANDLER_ASSIGN "Error: Could not assign an interrupt handler %s\n"
#define ERROR_PROCESS_NOT_OPEN "Could not open new process %s\n"

int execInBackground(int count, char **arglist);

int pipeArgumentLocation(int count, char **arglist);

int runProcess(char** argsList);

int runPipeProcess(char** inputArgsList, char** outputArgsList);

int runBackgroundProcess(char** argsList);

int assignSignalHandler(bool ignore);

int process_arglist(int count, char** arglist){

    // in case of background process that already finished their run - remove them from zombie status
    bool zombiesExists = true;
    while (zombiesExists){
        pid_t p = waitpid(-1, NULL, WNOHANG);
        if (p == -1){
            // no waiting child processes at all
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
        fprintf(stderr, AMPER_NOT_AT_END);
        return 1; }
    if (pipeLocation == -1){
        fprintf(stderr, WRONG_PIPE_LOC);
        return 1; }

    if (execBackground == 1){
        // remove the "&" from the arguments
        arglist[count-1] = (char*) NULL;

        runBackgroundProcess(arglist);
        return 1;
    }

    else if(execBackground == 0) {

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

    }

    else{
        // This error should not happen in any case, if so it is a bug in the program.
        // print a message to the user and continue the loop
        fprintf(stderr, UNEXPECT_ERROR);
        return 1;
    }

    //flow ended normally - continue to listen to user's commands
    return 1;
}

int prepare(void){

    int res = assignSignalHandler(true);
    if (res < 1){
        return 1;
    } else{
        return 0;
    }
}

int finalize(void){
    return 0;
}

int execInBackground(int count, char** arglist){
    int i;
    for (i = 0; i < count; i++){
        if (strcmp(arglist[i], "&") == 0){
            if (i == count-1) return 1; // ampersand is last argument
            else return -1; // ampersand as a middle argument (assuming no more than a single pip)
        }
    }
    return 0; // no ampersand - do not run in background
}

int pipeArgumentLocation(int count, char **arglist){
    int i;
    for (i = 0; i < count; i++){
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
        fprintf(stderr, ERROR_PROCESS_NOT_OPEN, strerror(errno));
        return -1;
    }

    if (execPid == 0) {

        int res = assignSignalHandler(false);
        if (res == -1){
            perror(ERROR_HANDLER_ASSIGN);
            exit(1);
        }

        int execRunStatus = execvp(argsList[0], argsList);

        if (execRunStatus == -1) {
            fprintf(stderr, ERROR_EXEC_PROG, strerror(errno));
            exit(1);
        }
    }

    // only executes in father process
    return execPid;
}

int runBackgroundProcess(char** argsList){
    pid_t execPid = fork();

    if (execPid == -1) {
        fprintf(stderr, ERROR_PROCESS_NOT_OPEN, strerror(errno));
        return -1;
    }

    if (execPid == 0) {

        int execRunStatus = execvp(argsList[0], argsList);
        if (execRunStatus == -1) {
            fprintf(stderr, ERROR_EXEC_PROG, strerror(errno));
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

        int res = assignSignalHandler(false);
        if (res == -1){
            perror(ERROR_HANDLER_ASSIGN);
            exit(1);
        }

        //redirect stdout fd to write side of pipe and close read side.
        int originalStdout = dup(1);
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        int status = execvp(inputArgsList[0], inputArgsList);
        if (status == -1) {

            // undo the redirection - fd 1 will point again to stdout
            dup2(originalStdout, 1);
            fprintf(stderr, ERROR_EXEC_PROG, strerror(errno));
            exit(1);
        }
    }
    // close the write side of pipe at parent process - send EOF to pipe and enable reading
    close(pipefd[1]);

    // fork for output part of pipe command
    secondPid = fork();
    if (secondPid == 0) {

        int res = assignSignalHandler(false);
        if (res == -1){
            perror(ERROR_HANDLER_ASSIGN);
            exit(1);
        }

        //redirect stdin fd to read side of pipe and close write side
        dup2(pipefd[0], 0);
        close(pipefd[1]);
        int status = execvp(outputArgsList[0], outputArgsList);
        if (status == -1) {
            fprintf(stderr, ERROR_EXEC_PROG, strerror(errno));
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

int assignSignalHandler(bool ignore){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    if (ignore){
        // Setup the built in ignore signal handler
        sa.sa_handler = SIG_IGN;
    } else{
        // Setup the built in default interrupt signal handler
        sa.sa_handler = SIG_DFL;
    }

    // Restart the system call, if at all possible
    sa.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigfillset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror(ERROR_HANDLER_ASSIGN);
        return -1;
    }

    return 1;
}