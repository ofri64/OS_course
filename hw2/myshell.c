#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// arglist - a list of char* arguments (words) provided by the user
// it contains count+1 items, where the last item (arglist[count]) and *only* the last is NULL
// RETURNS - 1 if should cotinue, 0 otherwise
int process_arglist(int count, char** arglist);

// prepare and finalize calls for initialization and destruction of anything required
int prepare(void);
int finalize(void);

int execInBackground(int count, char **arglist);

int pipeArgumentLocation(int count, char **arglist);

int splitArgumentsOnPipe(char*** ptrForInputArgs, char*** ptrForOutputArgs,char** originalArgs,int inputCount, int outputCount, int pipeLocation);

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

        int inputCount = pipeLocation;
        char** inputProgArgList;

        int outputCount = count - pipeLocation -1;
        char** outputProgArgList;

        int splitStatus = -1;

        while (splitStatus == -1){
            splitStatus = splitArgumentsOnPipe(&inputProgArgList, &outputProgArgList, arglist, inputCount, outputCount, pipeLocation);
        }

        for (int i = 0; i < inputCount; i++){
            printf("input[%d] is: %s\n", i, inputProgArgList[i]);
        }
        for (int i = 0; i < outputCount; i++){
            printf("input[%d] is: %s\n", i, outputProgArgList[i]);
        }

    }


    if (execBackground == 1){
        //TODO: implement running in background mode
    }

    return 1;

}

int prepare(void){;}
int finalize(void){;}

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

int splitArgumentsOnPipe(char*** ptrForInputArgs, char*** ptrForOutputArgs,char** originalArgs,int inputCount, int outputCount, int pipeLocation){
    char** inputArgs = (char**) malloc((inputCount+1) * sizeof(char*));
    char** outputArgs = (char**) malloc((outputCount+1) * sizeof(char*));

    if (inputArgs == NULL || outputArgs == NULL){
        return -1; // allocation failed
    }

    // copy argument to new separated arrays
    int totalCount = inputCount + outputCount +1;
    for (int i = 0; i < totalCount; i++){

        if(i < pipeLocation){
            inputArgs[i] = originalArgs[i];
        }
        else if (i > pipeLocation){
            outputArgs[i-(pipeLocation+1)] = originalArgs[i];
        }
    }

    // append NULL at the end of the arguments array
    inputArgs[inputCount] = NULL;
    outputArgs[outputCount] = NULL;

    // update pointer to point the new created arguments array and return valid value
    *ptrForInputArgs = inputArgs;
    *ptrForOutputArgs = outputArgs;

    return 1;
}