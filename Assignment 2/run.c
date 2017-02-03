#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(){
	int id=fork(),status=0;
	if(id==0){
		execlp("./shell","./shell",(char *)NULL);
	}
	else{
		waitpid(id,&status,0);
	}
	return 0;
}