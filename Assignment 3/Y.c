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
#include <errno.h>
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

void sig_handler(int sig){	//Ctrl+C should detach the shared memory and stop the process
	if(sig==SIGINT){
		shmdt(stud);
		shmdt(n);
		shmdt(update);
		exit(0);
	}
}

int main(){
	if(close(open("temp",O_CREAT|O_RDONLY))==-1){	//create temporary file to get keys
		printf("Error in getting key\n");
		exit(-1);
	}
	int rd_B,fd,i,type,rollno;
	float cg;
	char *buf;
	struct sembuf pop,vop;
	struct semid_ds sem_info_buf;
	time_t pre;
	pop.sem_num=vop.sem_num=0;	//semaphores' data structure
	pop.sem_flg=vop.sem_flg=0;
	pop.sem_op=-1;
	vop.sem_op=1;
	signal(SIGINT,sig_handler);
	semid_init=semget(ftok("temp",0), 1, 0777);	//accessing semaphore init
	while(semid_init==-1){	//in case of error
		if(errno==ENOENT){	//init has not been yet created
			sleep(1);
			semid_init=semget(ftok("temp",0), 1, 0777);	//try creating again after 1 second sleep
			continue;
		}
		perror("Could not get semaphore for initializations in Y");	//in case of any other error
		exit(-1);
	}
	if(semctl(semid_init,0,IPC_STAT,&sem_info_buf)==-1){	//get data structure of the semaphore
		perror("Error while retriving info for init");
		exit(-1);
	}
	while(sem_info_buf.sem_otime==0){	//check if the semaphore is initialized or not
		sleep(1);	//semaphore not yet initialized so 1 second sleep
		if(semctl(semid_init,0,IPC_STAT,&sem_info_buf)==-1){
			perror("Error while retriving info for init");
			exit(-1);
		}
	}
	P(semid_init);	//wait for the X process to complete the copying of data from file to shared memory and make other initialisations
	V(semid_init);	//unblock other Y also
	shmid_data=shmget(ftok("temp",1),MX_STUD*sizeof(record),0777);
	if(shmid_data==-1){
		perror("Could not get shared memory for data");
		exit(-1);
	}
	semid_mutex=semget(ftok("temp",2), 1, 0777);
	if(semid_mutex==-1){
		perror("Could not get semaphore for mutex");
		exit(-1);
	}
	shmid_n=shmget(ftok("temp",3),sizeof(int),0777);
	if(shmid_n==-1){
		perror("Could not get shared memory for number of students");
		exit(-1);
	}
	shmid_update=shmget(ftok("temp",4),sizeof(int),0777|IPC_CREAT);
	if(shmid_update==-1){
		perror("Could not get shared memory for number of students");
		exit(-1);
	}
	stud=(record *)shmat(shmid_data,0,0);
	if(stud==(void *)(-1)){
		perror("Could not attach");
		exit(-1);
	}
	n=(int *)shmat(shmid_n,0,0);
	if(n==(void *)(-1)){
		perror("Could not attach");
		exit(-1);
	}
	update=(int *)shmat(shmid_update,0,0);
	if(update==(void *)(-1)){
		perror("Could not attach");
		exit(-1);
	}
	while(1){
		printf("Enter the type of Query (1-Search, 2-Update, 0-Exit) : \n");	//ask for query
		scanf("%d",&type);	//query type
		if(type==1){	//search
			printf("Enter the roll number to be searched : ");
			scanf("%d",&rollno);
			P(semid_mutex);	//wait if updation is going on
			for(i=0;i<*n;i++){
				if(rollno==stud[i].roll){	//read the details
					printf("Student Name : %s %s\n",stud[i].fname,stud[i].lname);
					printf("CGPA : %f\n",stud[i].cgpa);
					break;
				}
			}
			if(i==*n)printf("No such roll found\n");	//the roll is not found
			V(semid_mutex);	//unblock processes to use the shared memory
		}
		else if(type==2){	//update
			printf("Enter the roll number and CGPA to be updated (separated by space) : ");
			scanf("%d%f",&rollno,&cg);	//query the roll and cgpa
			P(semid_mutex);	//block processes to update the shared memory
			for(i=0;i<*n;i++){	//update the shared memory
				if(rollno==stud[i].roll){
					stud[i].cgpa=cg;
					*update=1;
					break;
				}
			}
			if(i==*n)printf("No such roll found\n");
			V(semid_mutex);	//unblock process to access the shared memory
		}
		else break;	//exit
	}
	shmdt(stud);	//detach from the shared memories
	shmdt(n);
	shmdt(update);
	return 0;
}
