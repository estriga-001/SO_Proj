#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *slow_thread() {
	
	for(int i=0; i<3; i++)
	{
		printf("Hello, I'm Slooooooooowww thread \n");
		sleep(3);
	}

	pthread_exit(NULL);
	return NULL;
}

void *fast_thread() {
	
	for(int i=0; i<9; i++)
	{
		printf("\t\t\tHello, I'm FAST thread \n");
		sleep(1);
	}

	pthread_exit(NULL);
	return NULL;
}

int main(void){
	
	pthread_t thr_slow, thr_fast;
	
	pthread_create(&thr_slow, NULL, slow_thread, NULL);
	pthread_create(&thr_fast, NULL, fast_thread, NULL);

	pthread_join(thr_slow, NULL);
	pthread_join(thr_fast, NULL);
	
	return 0;
}

