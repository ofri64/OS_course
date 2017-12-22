#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* thread_func(void* thread_param)
{
	long tid;
	tid = (long) thread_param;
	printf("Hello World! It's me, thread #%ld!\n", tid);

	pthread_exit(NULL); // same as:	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t thread_id;
	int rc;
	long t = 1;

	printf("In main: creating thread\n");
	rc = pthread_create(&thread_id, NULL, thread_func, (void *)t);
	if (rc)
	{
		printf("ERROR in pthread_create(): %s\n", strerror(rc));
		exit(-1);
	}

	/* Last thing that main() should do */
	pthread_exit(NULL);	// DIFFERENT from:	return 0; (!!!)
}
