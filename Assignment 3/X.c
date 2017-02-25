#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#define MX_STUD 100
#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

typedef struct record_{	//structure of the data for one student
	char fname[21];
	char lname[21];
	int roll;
	float cgpa;
}record;

int shmid_data=-1,shmid_n=-1,shmid_update=-1,semid_mutex=-1,semid_init=-1;
int *n;
int *update;
record *stud;

void sig_handler(int sig){
	if(sig==SIGINT){	//Ctrl+C should detach the shared memory,delete the shared memory and stop the process
		shmdt(stud);
		shmdt(n);
		shmdt(update);
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		shmctl(shmid_n,IPC_RMID,0);
		shmctl(shmid_update,IPC_RMID,0);
		exit(0);
	}
}

int main(int argv,char *argc[]){
	if(argv!=2){	//the call should contain the filename as an argument
		printf("Error while passing arguments, Usage : X filename\n");
		exit(-1);
	}
	if(close(open("temp",O_CREAT|O_RDONLY))==-1){	//created a temp file for key generation
		printf("Error in getting key\n");
		exit(-1);
	}
	int rd_B,fd=-1,type,space,i,j;
	char buf[1];
	char sp[1];
	char ln[1];
	struct sembuf pop,vop;
	struct semid_ds sem_info_buf;
	time_t pre;
	pop.sem_num=vop.sem_num=0;	//the semaphore data structure.
	pop.sem_flg=vop.sem_flg=0;
	pop.sem_op=-1;
	vop.sem_op=1;
	signal(SIGINT,sig_handler);	//the signal Ctrl+C should go to the signal handler
	fd=open(argc[1],O_RDONLY);	//open the filename received from the aregument
	if(fd==-1){	//in case of error in opening the file
		perror("Error while opening the data file");
		semctl(semid_init,0,IPC_RMID,0);
		exit(-1);
	}
	semid_init=semget(ftok("temp",0), 1, 0777|IPC_CREAT);	//creating semaphore to prevent Y from starting before X
	if(semid_init==-1){
		perror("Could not get semaphore for initializations in X");	//in case of error
		exit(-1);
	}
	if(semctl(semid_init,0,SETVAL,0)==-1){	//setting the value for semaphore
		perror("Error while setting the init semaphore value");	//in case of error
		semctl(semid_init,0,IPC_RMID,0);
		exit(-1);
	}
	shmid_data=shmget(ftok("temp",1),MX_STUD*sizeof(record),0777|IPC_CREAT);	//creating shared memory for data of students
	if(shmid_data==-1){	//in case of error
		perror("Could not get shared memory for data");
		semctl(semid_init,0,IPC_RMID,0);
		exit(-1);
	}
	semid_mutex=semget(ftok("temp",2), 1, 0777|IPC_CREAT);	//creating semaphore for exclusion while accessing shared memory
	if(semid_mutex==-1){	//in case of error
		perror("Could not get semaphore for mutex");
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		exit(-1);
	}
	if(semctl(semid_mutex,0,SETVAL,1)==-1){	//setting the value of the semaphore
		perror("Error while setting the mutex value");	//in case of error
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		exit(-1);
	}
	shmid_n=shmget(ftok("temp",3),sizeof(int),0777|IPC_CREAT);	//creating shared memory to store the number of students
	if(shmid_n==-1){	//in case of error
		perror("Could not get shared memory for number of students");
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		semctl(semid_init,0,IPC_RMID,0);
		exit(-1);
	}
	shmid_update=shmget(ftok("temp",4),sizeof(int),0777|IPC_CREAT);	//creating shared memory to store if the data has been updated
	if(shmid_update==-1){	//in case of error
		perror("Could not get shared memory for number of students");
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_n,IPC_RMID,0);
		exit(-1);
	}
	stud=(record *)shmat(shmid_data,0,0);	//ataching to the shared memory - data
	if(stud==(void *)(-1)){	//in case of error
		perror("Could not attach");
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		shmctl(shmid_n,IPC_RMID,0);
		shmctl(shmid_update,IPC_RMID,0);
		exit(-1);
	}
	n=(int *)shmat(shmid_n,0,0);	//attaching to the shared memory - n
	if(n==(void *)(-1)){	//in case of error
		perror("Could not attach");
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		shmctl(shmid_n,IPC_RMID,0);
		shmctl(shmid_update,IPC_RMID,0);
		exit(-1);
	}
	update=(int *)shmat(shmid_update,0,0);	//attaching to the shared memory - update
	if(update==(void *)(-1)){	//in case of error
		perror("Could not attach");
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		shmctl(shmid_n,IPC_RMID,0);
		shmctl(shmid_update,IPC_RMID,0);
		exit(-1);
	}
	//write from file to shared memory
	type=0;	//iterate over the different types of data in file
	space=1;	//to check if the last character was space
	j=0;	//iterate over the data characters
	i=0;	//iterate over the number of students
	while(1){	//read 1 byte in a loop
		rd_B=read(fd,buf,1);	//read a byte from file
		if(rd_B==-1){	//in case of error
			perror("Error while reading from file");
			shmdt(stud);
			semctl(semid_init,0,IPC_RMID,0);
			shmctl(shmid_data,IPC_RMID,0);
			semctl(semid_mutex,0,IPC_RMID,0);
			shmctl(shmid_n,IPC_RMID,0);
			shmctl(shmid_update,IPC_RMID,0);
			exit(-1);
		}
		if(rd_B==0){	//end of file
			if(type==0)*n=i;	//judging the value of number of students
			else *n=i+1;
			break;
		}
		if(isspace((int)(*buf))){	//in case of a space character read
			if(type==4||type==5){
				i++;
				type=0;
				space=1;
				j=0;
			}
			if(space==0)space=1;
			continue;
		}
		if(space){	//not a space but last character was space
			space=0;
			if(type==1){
				stud[i].fname[j]='\0';
			}
			else if(type==2){
				stud[i].lname[j]='\0';
			}
			type++;
			j=0;
		}
		if(type==1){	//reading fname
			stud[i].fname[j]=(*buf);
			j++;
			continue;
		}
		if(type==2){	//reading lname
			stud[i].lname[j]=(*buf);
			j++;
			continue;
		}
		if(type==3){	//reading roll
			stud[i].roll*=10;
			stud[i].roll+=((*buf)-'0');
			continue;
		}
		if(type==4){	//reading cgpa(integral part)
			if(*buf=='.'){
				j=1;
				type++;
				continue;
			}
			stud[i].cgpa*=10.0;
			stud[i].cgpa+=(float)((*buf)-'0');
			continue;
		}
		if(type==5){	//reading cgpa(fractional part)
			j*=10;
			stud[i].cgpa+=(float)(((float)((*buf)-'0'))/((float)(j)));
			continue;
		}
	}
	*update=0;	//setting update to zero
	close(fd);	//closing the data file as the complete data file is now read
	V(semid_init);	//unblocking Y
	while(1){
		sleep(5);	//sleep for 5 seconds
		P(semid_mutex);	//ask to enter critical region to access update
		if(*update){	//if updation is required do the update
			//copy back to file
			*update=0;
			FILE *fp=fopen(argc[1],"w");
			if(fp==NULL){
				perror("Data file could not be opened");
				semctl(semid_init,0,IPC_RMID,0);
				shmctl(shmid_data,IPC_RMID,0);
				semctl(semid_mutex,0,IPC_RMID,0);
				shmctl(shmid_n,IPC_RMID,0);
				shmctl(shmid_update,IPC_RMID,0);
				exit(-1);
			}
			for(i=0;i<*n;i++)fprintf(fp,"%s %s %d %f\n",stud[i].fname,stud[i].lname,stud[i].roll,stud[i].cgpa);
			fclose(fp);
		}
		V(semid_mutex);	//copying back to file is executed now unblock preocesses to update
	}
	//wont reach here though.
	shmdt(update);
	shmdt(stud);
	shmdt(n);
	semctl(semid_init,0,IPC_RMID,0);
	shmctl(shmid_data,IPC_RMID,0);
	semctl(semid_mutex,0,IPC_RMID,0);
	shmctl(shmid_n,IPC_RMID,0);
	shmctl(shmid_update,IPC_RMID,0);
	return 0;
}
