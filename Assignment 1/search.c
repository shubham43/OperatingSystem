#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MXf 30
#define MXs 100

void find(int key,int arr[],int n,int root){
	int i;
	if(n<=10){
		for(i=0;i<n;i++){
			if(arr[i]==key){
				if(root){
					printf("found\n");
					return;
				}
				exit(1);
			}
		}
		if(root){
			printf("not found\n");
			return;
		}
		exit(0);
	}
	int id1=fork(),id2,status1=0,status2=0;
	if(id1==0){
		find(key,arr,n/2,0);
	}
	else{
		id2=fork();
		if(id2==0){
			find(key,&arr[n/2],n-n/2,0);
		}
		else{
			waitpid(id1,&status1,0);
			waitpid(id2,&status2,0);
			if(status1==0&&status2==0){
				if(root){
					printf("not found\n");
					return;
				}
				exit(0);
			}
			if(root){
				printf("found\n");
				return;
			}
			exit(1);
		}
	}
}

int main(){
	FILE *fp;
	char f[MXf];
	int arr[MXs],i,k,j;
	printf("Enter the filename : ");
	scanf("%s",&f[0]);
	fp=fopen(f,"r");
	i=0;
	while(fscanf(fp,"%d",&arr[i])!=EOF)i++;
	printf("Array Elements read from the file : ");
	for(j=0;j<i;j++){
		printf("%d ",arr[j]);
	}
	printf("\nEnter positive keys to search ( less than or equal to 0 to stop) :-\n");
	while(1){
		scanf("%d",&k);
		if(k<=0)break;
		find(k,arr,i,1);
	}
	fclose(fp);
	return 0;
}
