#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(void) {

	for(int i = 0; i < 3; i++){
	
		fork();
		printf("%d: %d\n", getpid(),i);
	}
	
	sleep(5);
	return 0;
}