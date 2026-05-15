#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

pthread_t my_threads[3];
//sem_t 
//???
//...

void *print_portugal(){
	
	//???
	//...
		printf("Portugal ");
	//???
	//...
	
	pthread_exit(NULL);
	return NULL;
} 

void *print_somos(){
	
	//???
	//...
		printf("Somos ");
	//???
	//...
	
	pthread_exit(NULL);
	return NULL;
}

void *print_nos(){
	
	//???
	//...
		printf("Nos!\n");
	//???
	//...
	
	pthread_exit(NULL);
	return NULL;
}

int main(void){
	
	//sem_init... initial values for each sem: 
	//who should be allowed to start right away and who should initially be blocked??
	//???
	//...
	
	pthread_create(&my_threads[0],NULL, print_portugal,NULL);
	pthread_create(&my_threads[1],NULL, print_somos,NULL);
	pthread_create(&my_threads[2],NULL, print_nos,NULL);
	
	for (int i = 0; i < 3; i++){
		pthread_join(my_threads[i], NULL);
	}
	
	printf("\nExiting main...\n");
	 
	//sem_destroy
	//???
	//...

	exit(0);
}