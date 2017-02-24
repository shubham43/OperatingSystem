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
#define MX_STUD 100
#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

typedef struct record_{
	char fname[21];
	char lname[21];
	int roll;
	float cgpa;
}record;

int main(int argc,char * argv[]){
	if(argc!=2){
		printf("Error while passing arguments, Usage : X filename\n");
		exit(-1);
	}
	if(open("temp",O_CREAT|O_RDONLY)==-1){
		printf("Error in getting key\n");
		exit(-1);
	}
	int rd_B,fd,shmid_data=-1,shmid_n=-1,semid_mutex=-1,semid_init=-1,type,space,i,j;
	int *n;
	record *stud;
	char *buf;
	struct sembuf pop,vop;
	struct shmid_ds shm_info_buf;
	struct semid_ds sem_info_buf;
	time_t pre;
	pop.sem_num=vop.sem_num=0;
	pop.sem_flg=vop.sem_flg=0;
	pop.sem_op=-1;
	vop.sem_op=1;
	fd=open(argv[1],O_RDONLY);
	if(fd==-1){
		perror("Error while opening the data file");
		semctl(semid_init,0,IPC_RMID,0);
		exit(-1);
	}
	//Created shared memory for : data,number_of_students and semaphores : mutex
	semid_init=semget(ftok("temp",0), 1, 0777|IPC_CREAT);
	if(semid_init==-1){
		perror("Could not get semaphore for initializations in X");
		exit(-1);
	}
	if(semctl(semid_init,0,IPC_STAT,&sem_info_buf)==-1){
		perror("Error while retriving info for init");
		semctl(semid_init,0,IPC_RMID,0);
		exit(-1);
	}
	if(sem_info_buf.sem_otime==0){
		if(semctl(semid_init,0,SETVAL,0)==-1){
			perror("Error while setting the init semaphore value");
			semctl(semid_init,0,IPC_RMID,0);
			exit(-1);
		}
	}
	shmid_data=shmget(ftok("temp",1),MX_STUD*sizeof(record),0777|IPC_CREAT);
	if(shmid_data==-1){
		perror("Could not get shared memory for data");
		semctl(semid_init,0,IPC_RMID,0);
		exit(-1);
	}
	semid_mutex=semget(ftok("temp",2), 1, 0777|IPC_CREAT);
	if(semid_mutex==-1){
		perror("Could not get semaphore for mutex");
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		exit(-1);
	}
	if(semctl(semid_mutex,0,SETVAL,1)==-1){
		perror("Error while setting the mutex value");
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		exit(-1);
	}
	shmid_n=shmget(ftok("temp",3),sizeof(int),0777|IPC_CREAT);
	if(shmid_n==-1){
		perror("Could not get shared memory for number of students");
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		semctl(semid_init,0,IPC_RMID,0);
		exit(-1);
	}
	stud=(record *)shmat(shmid_data,0,0);
	if(stud==(void *)(-1)){
		perror("Could not attach");
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		shmctl(shmid_n,IPC_RMID,0);
		exit(-1);
	}
	n=(int *)shmat(shmid_data,0,0);
	if(n==(void *)(-1)){
		perror("Could not attach");
		semctl(semid_init,0,IPC_RMID,0);
		shmctl(shmid_data,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		shmctl(shmid_n,IPC_RMID,0);
		exit(-1);
	}
	//write from file to shared memory
	type=0;
	space=1;
	j=0;
	i=0;
	while(1){
		rd_B=read(fd,buf,1);
		if(rd_B==-1){
			perror("Error while reading from file");
			shmdt(stud);
			semctl(semid_init,0,IPC_RMID,0);
			shmctl(shmid_data,IPC_RMID,0);
			semctl(semid_mutex,0,IPC_RMID,0);
			shmctl(shmid_n,IPC_RMID,0);
			exit(-1);
		}
		if(rd_B==0){
			if(type==0)*n=i;
			else *n=i+1;
			break;
		}
		if(isspace((int)(*buf))){
			if(type==4||type==5){
				i++;
				type=0;
				space=1;
				j=0;
			}
			if(space==0)space=1;
			continue;
		}
		if(space){
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
		if(type==1){
			stud[i].fname[j]=(*buf);
			j++;
			continue;
		}
		if(type==2){
			stud[i].lname[j]=(*buf);
			j++;
			continue;
		}
		if(type==3){
			stud[i].roll*=10;
			stud[i].roll+=((*buf)-'0');
			continue;
		}
		if(type==4){
			if(*buf=='.'){
				j=1;
				type++;
				continue;
			}
			stud[i].cgpa*=10.0;
			stud[i].cgpa+=(float)((*buf)-'0');
			continue;
		}
		if(type==5){
			j*=10;
			stud[i].cgpa+=(float)(((float)((*buf)-'0'))/((float)(j)));
			continue;
		}
	}
	shmctl(shmid_data,IPC_STAT,&shm_info_buf);
	pre=shm_info_buf.shm_ctime;
	close(fd);
	shmdt(stud);
	shmdt(n);
	V(semid_init);
	while(1){
		sleep(5);
		P(semid_mutex);
		shmctl(shmid_data,IPC_STAT,&shm_info_buf);
		if(pre-(shm_info_buf.shm_ctime)<0){
			//copy back to file
			fd=open(argv[1],O_WRONLY|O_TRUNC);
			stud=(record *)shmat(shmid_data,0,0);
			if(stud==(void *)(-1)){
				perror("Could not attach");
				semctl(semid_init,0,IPC_RMID,0);
				shmctl(shmid_data,IPC_RMID,0);
				semctl(semid_mutex,0,IPC_RMID,0);
				shmctl(shmid_n,IPC_RMID,0);
				exit(-1);
			}
			n=(int *)shmat(shmid_data,0,0);
			if(n==(void *)(-1)){
				perror("Could not attach");
				semctl(semid_init,0,IPC_RMID,0);
				shmctl(shmid_data,IPC_RMID,0);
				semctl(semid_mutex,0,IPC_RMID,0);
				shmctl(shmid_n,IPC_RMID,0);
				exit(-1);
			}
			for(i=0;i<*n;i++){
				write(fd,stud[i].fname,sizeof(stud[i].fname));
				write(fd," ",sizeof(" "));
				write(fd,stud[i].lname,sizeof(stud[i].lname));
				write(fd," ",sizeof(" "));
				write(fd,&(stud[i].roll),sizeof(stud[i].roll));
				write(fd," ",sizeof(" "));
				write(fd,&(stud[i].cgpa),sizeof(stud[i].cgpa));
				write(fd,"\n",sizeof("\n"));
			}
			close(fd);
			shmdt(stud);
			shmdt(n);
		}
		V(semid_mutex);
	}
	semctl(semid_init,0,IPC_RMID,0);
	shmctl(shmid_data,IPC_RMID,0);
	semctl(semid_mutex,0,IPC_RMID,0);
	shmctl(shmid_n,IPC_RMID,0);
	return 0;
}
