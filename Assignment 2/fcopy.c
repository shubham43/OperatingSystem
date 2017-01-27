#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argv,char *argc[]){
	if(argv!=3){
		printf("fcopy file1 file2\n");
		exit(-1);
	}
	int frd,fwr;
	frd=open(argc[1],O_RDONLY);//Opening file to be copied
	if(frd==-1){
		perror("File1 could not be opened.\n");
		exit(-1);
	}
	fwr=open(argc[2],O_TRUNC | O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR);//Opening file to be written
	if(fwr==-1){
		perror("File2 could not be opened.\n");
		exit(-1);
	}
	int fd1[2],fd2[2],buff_size;
	pipe(fd1);
	pipe(fd2);
	char buff[110];//not reading more than 100 bytes.
	char ack[5];
	int id=fork();
	if(id!=0){
		close(fd1[0]);
		close(fd2[1]);
		while(1){
			buff_size=read(frd,buff,100);//reading from frd and writing to fwr
			write(fd1[1],buff,buff_size);
			if(read(fd2[0],ack,1)>0){
				if(ack[0]=='0'){
					if(buff_size<100){
						close(fd1[1]);
						close(fd2[0]);
						exit(1);
					}
				}
				else{
					close(fd1[1]);
					close(fd2[0]);
					exit(-1);
				}
			}
			else{
				close(fd1[1]);
				close(fd2[0]);
				exit(-1);
			}
		}
	}
	else{
		close(fd1[1]);
		close(fd2[0]);
		while(1){
			buff_size=read(fd1[0],buff,100);
			if(write(fwr,buff,buff_size)==-1){
				perror("Error while copying\n");
				write(fd2[1],"-1",2);
				close(fd1[0]);
				close(fd2[1]);
				exit(-1);
			}
			if(buff_size==100){
				write(fd2[1],"0",1);
			}
			else{
				printf("File copied successfully\n");
				write(fd2[1],"0",1);
				close(fd1[0]);
				close(fd2[1]);
				exit(1);
			}
		}
	}
	return 0;
}
