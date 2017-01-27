#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define MX 1000
#define MXf 30

void sort(int arr[],int n){
	int i,j,temp;
	for(i=0;i<n-1;i++){
		for(j=0;j<n-i-1;j++){
			if(arr[j]>arr[j+1]){
				temp=arr[j];
				arr[j]=arr[j+1];
				arr[j+1]=temp;
			}
		}
	}
}

int main(int argc, char *argv[]){
	if(argc!=2){
		printf("Usage : sort1 filename\n");
		exit(-1);
	}
	FILE *fp;
	char f[MXf];
	int arr[MX],i,j;
	strcpy(f,argv[1]);
	fp=fopen(f,"r");
	i=0;
	while(fscanf(fp,"%d",&arr[i])!=EOF)i++;
	printf("Array elements read from the file : ");
	for(j=0;j<i;j++){
		printf("%d ",arr[j]);
	}
	sort(arr,i);
	printf("\nArray Elements read from the file : ");
	for(j=0;j<i;j++){
		printf("%d ",arr[j]);
	}
	printf("\n");
	fclose(fp);
	exit(1);
}
