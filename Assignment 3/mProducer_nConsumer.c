#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#define BUF_SIZE 20
#define CNT 50
#define P(s) semop(s, &pop, 1)
#define V(s) semop(s, &vop, 1)

void perf_prod(int shmid_buffer,int shmid_index,int semid_empty,int semid_full,int semid_mutex,struct sembuf pop,struct sembuf vop){
	printf("Prod Perf %d started\n",getpid());
	int *buff=(int *)shmat(shmid_buffer,0,0);
	int *idx=(int *)shmat(shmid_index,0,0);
	int i;
	for(i=1;i<=CNT;i++){
		P(semid_empty);
		P(semid_mutex);
		printf("Producer %d writes %d\n",getpid(),i);
		buff[(*idx)]=i;
		(*idx)++;
		(*idx)=(*idx)%BUF_SIZE;
		V(semid_mutex);
		V(semid_full);
	}
	shmdt(buff);
	shmdt(idx);
	printf("Prod Perf %d completed\n",getpid());
	return;
}

void perf_cons(int m,int shmid_buffer,int shmid_outdex,int shmid_SUM,int semid_empty,int semid_full,int semid_mutex,struct sembuf pop,struct sembuf vop){
	printf("Cons Perf %d started\n",getpid());
	int *buff=(int *)shmat(shmid_buffer,0,0);
	int *odx=(int *)shmat(shmid_outdex,0,0);
	int *sum=(int *)shmat(shmid_SUM,0,0);
	while(1){
		P(semid_full);
		P(semid_mutex);
		if(*sum==m*25*51){
			V(semid_mutex);
			V(semid_empty);
			V(semid_full);
			shmdt(buff);
			shmdt(odx);
			shmdt(sum);
			printf("Cons Perf %d completed\n",getpid());
			exit(0);
		}
		printf("Consumer %d reads %d\n",getpid(),buff[(*odx)]);
		(*sum)=(*sum)+buff[(*odx)];
		(*odx)++;
		(*odx)=(*odx)%BUF_SIZE;
		V(semid_mutex);
		V(semid_empty);
		if(*sum==m*25*51){
			V(semid_full);
			shmdt(buff);
			shmdt(odx);
			shmdt(sum);
			printf("Cons Perf %d completed\n",getpid());
			exit(0);
		}
	}
	shmdt(buff);
	shmdt(odx);
	shmdt(sum);
	printf("Cons Perf %d completed\n",getpid());
	return;
}

void create_prod(int n,int shmid_buffer,int shmid_index,int semid_empty,int semid_full,int semid_mutex,struct sembuf pop,struct sembuf vop){
	if(n==1){
		perf_prod(shmid_buffer,shmid_index,semid_empty,semid_full,semid_mutex,pop,vop);
		return;
	}
	int id=fork();
	if(id==0)create_prod(n-1,shmid_buffer,shmid_index,semid_empty,semid_full,semid_mutex,pop,vop);
	else{
		perf_prod(shmid_buffer,shmid_index,semid_empty,semid_full,semid_mutex,pop,vop);
		int status=0;
		wait(&status);
	}
}

void create_cons(int m,int n,int shmid_buffer,int shmid_outdex,int shmid_SUM,int semid_empty,int semid_full,int semid_mutex,struct sembuf pop,struct sembuf vop){
	if(n==1){
		perf_cons(m,shmid_buffer,shmid_outdex,shmid_SUM,semid_empty,semid_full,semid_mutex,pop,vop);
		return;
	}
	int id=fork();
	if(id==0)create_cons(m,n-1,shmid_buffer,shmid_outdex,shmid_SUM,semid_empty,semid_full,semid_mutex,pop,vop);
	else{
		perf_cons(m,shmid_buffer,shmid_outdex,shmid_SUM,semid_empty,semid_full,semid_mutex,pop,vop);
		int status=0;
		wait(&status);
	}
}

int main(){
	//Create shared memory for buffer,in,out and Sum
	//full,empty,mutex semaphores
	int shmid_buffer=-1,shmid_index=-1,shmid_outdex=-1,shmid_SUM=-1,semid_full=-1,semid_empty=-1,semid_mutex=-1;
	struct sembuf pop,vop;
	shmid_buffer=shmget(IPC_PRIVATE,BUF_SIZE*sizeof(int),0777|IPC_CREAT);
	if(shmid_buffer==-1){
		perror("Could not get shared memory for buffer");
		exit(-1);
	}
	shmid_SUM=shmget(IPC_PRIVATE,sizeof(int),0777|IPC_CREAT);
	if(shmid_SUM==-1){
		perror("Could not get shared memory for SUM");
		exit(-1);
	}
	shmid_index=shmget(IPC_PRIVATE,sizeof(int),0777|IPC_CREAT);
	if(shmid_index==-1){
		perror("Could not get shared memory for index");
		exit(-1);
	}
	shmid_outdex=shmget(IPC_PRIVATE,sizeof(int),0777|IPC_CREAT);
	if(shmid_outdex==-1){
		perror("Could not get shared memory for out-index");
		exit(-1);
	}
	semid_mutex=semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	if(semid_mutex==-1){
		perror("Could not get semaphore for mutex");
		exit(-1);
	}
	semid_empty=semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	if(semid_empty==-1){
		perror("Could not get semaphore for empty");
		exit(-1);
	}
	semid_full=semget(IPC_PRIVATE, 1, 0777|IPC_CREAT);
	if(semid_full==-1){
		perror("Could not get semaphore for full");
		exit(-1);
	}
	if(semctl(semid_mutex,0,SETVAL,1)==-1){
		perror("Error while setting the mutex value");
		exit(-1);
	}
	if(semctl(semid_empty,0,SETVAL,BUF_SIZE)==-1){
		perror("Error while setting the empty value");
		exit(-1);
	}
	if(semctl(semid_full,0,SETVAL,0)==-1){
		perror("Error while setting the full value");
		exit(-1);
	}
	pop.sem_num=vop.sem_num=0;
	pop.sem_flg=vop.sem_flg=0;
	pop.sem_op=-1;
	vop.sem_op=1;
	int m,n;
	scanf("%d%d",&m,&n);
	printf("Worked till scanning %d %d \n",m,n);
	int id=fork();
	if(id==0){
		int *idx=(int *)shmat(shmid_index,0,0);
		if(idx==(void *)(-1)){
			perror("Could not attach");
			exit(-1);
		}
		int *odx=(int *)shmat(shmid_outdex,0,0);
		if(odx==(void *)(-1)){
			perror("Could not attach");
			exit(-1);
		}
		int *sum=(int *)shmat(shmid_SUM,0,0);
		if(sum==(void *)(-1)){
			perror("Could not attach");
			exit(-1);
		}
		*idx=0;
		*odx=0;
		*sum=0;
		printf("Worked fine\n");
		shmdt(idx);
		shmdt(odx);
		shmdt(sum);
		int id0=fork();
		if(id0==0){
			printf("going step1\n");
			int id1=fork();
			if(id1==0)create_prod(m,shmid_buffer,shmid_index,semid_empty,semid_full,semid_mutex,pop,vop);
			else{
				printf("going production complete step2\n");
				int status;
				wait(&status);
			}
		}
		else{
			int id1=fork();
			if(id1==0)create_cons(m,n,shmid_buffer,shmid_outdex,shmid_SUM,semid_empty,semid_full,semid_mutex,pop,vop);
			else{
				int status;
				wait(&status);
			}
			int status;
			wait(&status);
		}
	}
	else{
		int status;
		wait(&status);
		int *sum=shmat(shmid_SUM,0,0);
		printf("%d\n",*sum);
		shmdt(sum);
		//print the Sum
		shmctl(shmid_SUM,IPC_RMID,0);
		shmctl(shmid_buffer,IPC_RMID,0);
		shmctl(shmid_index,IPC_RMID,0);
		shmctl(shmid_outdex,IPC_RMID,0);
		semctl(semid_mutex,0,IPC_RMID,0);
		semctl(semid_empty,0,IPC_RMID,0);
		semctl(semid_full,0,IPC_RMID,0);
		//delete shared space and semaphores..
	}
}
