#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MXf 30
#define MXs 100

int main(int argc,char *argv[]){
	if(argc!=2){
		printf("Usage : xsort filename\n");
		exit(-1);
	}
	int id=fork();
	if(id==0){
		execlp("xterm","xterm","-hold","-e","./sort1",argv[1],(char*)(NULL));
		perror("Execlp failed\n");
	}
	return 0;
}
