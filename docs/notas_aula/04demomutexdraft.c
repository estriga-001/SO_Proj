#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define RUNS 1000000

pthread_t tid[2];
long counter = 0;
pthread_mutex_t countLock;

void* increment()
{
	for(long i=0; i<RUNS; i++)
	{
		//I am about to change a shared variable...
		//??? add code...
		counter = counter + 1;
		//???
		
		usleep(10); //simulate some work
	}

	pthread_exit(NULL);
	return NULL;
}

int main(void)
{
	/* mutex is declared, initialized and destroyed.*
	 * Run the program to check its behaviour and	*
	 * complete the code to fix it					*/
	if (pthread_mutex_init(&countLock, NULL) != 0)
	{
		printf("::::MUTEX error on init\n");
		return 3;
	}

	printf("Counter initial value: %ld\n", counter);
	
	if (pthread_create(&(tid[0]), NULL, &increment, NULL) != 0)
	{
			printf("::::Thread [0] error on create\n");
	}

	if (pthread_create(&(tid[1]), NULL, &increment, NULL) != 0)
	{
			printf("::::Thread [1] error on create\n");
	}

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	
	printf("Counter final value: %ld\n", counter);
	
	pthread_mutex_destroy(&countLock);

	return 0;
}