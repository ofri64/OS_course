#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

void *busy_work(void *t)
{
	int i;
	long tid;
	double result=0.0;
	tid = (long)t;
	printf("Thread %ld starting\n", tid);
	
	for (i=0; i<10000000; i++)
	{
		result = result + sin(i) * tan(i);
	}
	
	printf("Thread %ld done. Result = %e\n",tid, result);
	pthread_exit((void*) t);
}

int main (int argc, char *argv[])
{
	pthread_t thread_id;
	int rc;
	long t = 1;
	void *status;

	printf("Main: creating thread %ld\n", t);
	rc = pthread_create(&thread_id, NULL, busy_work, (void *)t); 
	if (rc)
	{
		printf("ERROR in pthread_create(): %s\n", strerror(rc));
		exit(-1);
	}

	rc = pthread_join(thread_id, &status);
	if (rc)
	{
		printf("ERROR in pthread_join(): %s\n", strerror(rc));
		exit(-1);
	}
	printf("Main: completed join with thread %ld having a status of %ld\n",t,(long)status);
 
	printf("Main: program completed. Exiting.\n");
	pthread_exit(NULL);
}
