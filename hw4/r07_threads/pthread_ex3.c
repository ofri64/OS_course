#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define NUM_THREADS	5
int result=0;

void *busy_work(void *t)
{
	int i;
	long tid;
	tid = (long)t;
	printf("Thread %ld starting\n", tid);
	
	for (i=0; i<10000000; i++)
	{
		result += 1;
	}
	
	printf("Thread %ld done. Result = %d\n",tid, result);
	pthread_exit((void*) t);
}

int main (int argc, char *argv[])
{
	pthread_t thread[NUM_THREADS];
	int rc;
	long t;
	void *status;

	for(t=0; t<NUM_THREADS; t++)
	{
		printf("Main: creating thread %ld\n", t);
		rc = pthread_create(&thread[t], NULL, busy_work, (void *)t); 
		if (rc)
		{
			printf("ERROR in pthread_create(): %s\n", strerror(rc));
			exit(-1);
		}
	}

	for(t=0; t<NUM_THREADS; t++)
	{
		rc = pthread_join(thread[t], &status);
		if (rc)
		{
			printf("ERROR in pthread_join(): %s\n", strerror(rc));
			exit(-1);
		}
		printf("Main: completed join with thread %ld having a status of %ld\n",t,(long)status);
	}
 
	printf("Main: program completed. Result: %d. Exiting.\n", result);
	pthread_exit(NULL);
}
