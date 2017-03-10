#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define MX 10

struct thread_data {
	int start;
	int end;
	int k;
};

int mat[MX][MX];
int row_count=0;
int column_count=0;
int n;
pthread_t thread_id[MX];
pthread_mutex_t mutex;
pthread_cond_t row;
pthread_cond_t column;

/*
	synchronisation details :-
		in each phase i : the rows wait if any column from the i-1 phase are still in updation,
			else it updates the row for the i phase.
			Then the columns wait if any row of this phase is still in updation,
			else it updates the columns for the i phase.
		while in rows/columns : one element swapping is mutually exclusive from other
*/
void *start_routine(void *params){
	struct thread_data *param=(struct thread_data *)params;
	int i,j,l,temp,k=param->k;
	while(k--){
		i=param->start;
		j=param->end;
		while(i<j){
			pthread_mutex_lock(&mutex);
			while(column_count!=0&&column_count!=n)pthread_cond_wait(&row,&mutex);
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex);
//			printf("row=%d k=%d\n",i,k+1);
			temp=mat[i][0];
			pthread_mutex_unlock(&mutex);
			for(l=0;l<n-1;l++){
				pthread_mutex_lock(&mutex);
				mat[i][l]=mat[i][l+1];
				pthread_mutex_unlock(&mutex);
			}
			pthread_mutex_lock(&mutex);
			mat[i][l]=temp;
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex);
			row_count++;
			if(row_count==n){
				pthread_cond_broadcast(&column);
				row_count=0;
			}
			pthread_mutex_unlock(&mutex);
			i++;
		}
		i=param->start;
		j=param->end;
		while(i<j){
			pthread_mutex_lock(&mutex);
			while(row_count!=0&&row_count!=n)pthread_cond_wait(&column,&mutex);
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex);
//			printf("column=%d k=%d\n",i,k+1);
			temp=mat[n-1][i];
			pthread_mutex_unlock(&mutex);
			for(l=n-1;l>0;l--){
				pthread_mutex_lock(&mutex);
				mat[l][i]=mat[l-1][i];
				pthread_mutex_unlock(&mutex);
			}
			pthread_mutex_lock(&mutex);
			mat[l][i]=temp;
			pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex);
			column_count++;
			if(column_count==n){
				pthread_cond_broadcast(&row);
				column_count=0;
			}
			pthread_mutex_unlock(&mutex);
			i++;
		}
	}
}

int main(){
	int i,j,x,k,d;
	struct thread_data params[MX];
	pthread_mutex_init(&mutex,NULL);
	pthread_cond_init(&row,NULL);
	pthread_cond_init(&column,NULL);
	printf("Enter the size of square matrix (n) : ");
	scanf("%d",&n);
	printf("Enter the elements of square matrix row-wise : ");
	for(i=0;i<n;i++)for(j=0;j<n;j++)scanf("%d",&mat[i][j]);
	printf("The Initial Matrix :-\n");
	for(i=0;i<n;i++){
		for(j=0;j<n;j++)printf("%5d ",mat[i][j]);
		printf("\n");
	}
	printf("\n");
	printf("Enter the value of k for k-shuffle (k) : ");
	scanf("%d",&k);
	printf("Enter the value of threads to be created (x<=n) : ");
	scanf("%d",&x);
	d=n/x;
	for(i=0;i<x;i++){
		params[i].start=i*d;
		if(i==x-1){
			params[i].end=n;
		}
		else params[i].end=(i+1)*d;
		params[i].k=k;
		pthread_create(&thread_id[i],NULL,start_routine,(void *)(&params[i]));
	}
	for(i=0;i<x;i++){
		pthread_join(thread_id[i],NULL);
	}
	printf("The Final Matrix after shuffle:-\n");
	for(i=0;i<n;i++){
		for(j=0;j<n;j++)printf("%5d ",mat[i][j]);
		printf("\n");
	}
	printf("\n");
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&row);
	pthread_cond_destroy(&column);
}