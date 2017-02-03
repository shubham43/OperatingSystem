#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <stdint.h>
#include <fcntl.h>
#define DIR_PATH_LEN 100
#define CMD_LINE_LEN 100
#define CMD_IN_LINE 50

char *exec_files[CMD_IN_LINE];

void printPrompt(){
	char buf[DIR_PATH_LEN+1];
	if(getcwd(&buf[0],DIR_PATH_LEN+1)==NULL){
		perror("Fetching current directory failed");
		exit(-1);
	}
	printf("%s>",buf);
}

int read_input(char cl[],char cmdl[][CMD_LINE_LEN+1],int *typ){
	int i,j,k,n,flag;
	char c;
	i=0;
	while(scanf("%c",&c)!=EOF){
		if(c=='|')*typ=2;
		else if(c=='<'||c=='>')*typ=1;
		if(c=='\n'){
			cl[i]='\0';
			break;
		}
		cl[i]=c;
		i++;
	}
	n=i;
	for(i=j=k=flag=0;i<n;i++){
		if(flag){
			if(cl[i]==' '){
				cmdl[j][k]=cl[i];
				k++;
				flag=0;
				continue;
			}
			else{
				cmdl[j][k]=cl[i-1];
				k++;
				flag=0;
			}
		}
		if(cl[i]=='\0'||cl[i]=='\t'||cl[i]=='\r'||cl[i]=='\n'||cl[i]=='\a'||cl[i]=='\b'||cl[i]=='\v'||cl[i]=='\f'||cl[i]==' '){
			if(k==0)continue;
			cmdl[j][k]='\0';
			j++;
			k=0;
		}
		else{
			if(cl[i]=='\\'){
				flag=1;
				continue;
			}
			cmdl[j][k]=cl[i];
			k++;
		}
	}
	if(k>0){
		cmdl[j][k]='\0';
		j++;
		k=0;
	}
	return j;
}

int builtin(char cmdl[][CMD_LINE_LEN+1],int n){
	if(n==0)return 0;
	if(strcmp(cmdl[0],"cd")==0){
		if(n==1){
			strcpy(cmdl[1],"/home");
			n=2;
		}
		if(chdir(cmdl[1])==-1)perror("Changing directory failed");
		return 0;
	}
	if(strcmp(cmdl[0],"pwd")==0){
		char buf[DIR_PATH_LEN+1];
		if(getcwd(&buf[0],DIR_PATH_LEN+1)==NULL)perror("Fetching current directory failed");
		else printf("%s\n",buf);
		return 0;
	}
	if(strcmp(cmdl[0],"mkdir")==0){
		if(n==1){
			printf("mkdir: missed operand\nUsage : mkdir <directory_name>");
			return 0;
		}
		if(mkdir(cmdl[1],S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH)==-1)perror("Creating directory failed");
		return 0;
	}
	if(strcmp(cmdl[0],"rmdir")==0){
		if(n==1){
			printf("rmdir: missed operand\nUsage : rmdir <directory_name>");
			return 0;
		}
		if(rmdir(cmdl[1])==-1)perror("Removing directory failed");
		return 0;
	}
	if(strcmp(cmdl[0],"ls")==0){
		if(n>1 && strcmp(cmdl[1],"-l")==0){
			struct stat statbuf;
			struct passwd *pwd;
			struct group *grp;
			struct tm *tm;
			char datestring[256];
			struct dirent **namelist;
			int n;
			n=scandir(".", &namelist, NULL, alphasort);
			if(n<0){
				perror("Error while fetching direcory");
				return 0;
			}
			int i;
			free(namelist[0]);
			free(namelist[1]);
			for(i=2;i<n;i++){
				if(stat(namelist[i]->d_name,&statbuf)==-1){
					perror("Fetching file details failed");
					return 0;
				}
				printf("%6.6lo",(unsigned long)(statbuf.st_mode));
				printf("%2d",(int)(statbuf.st_nlink));
				if((pwd=getpwuid(statbuf.st_uid))!=NULL)printf(" %-8.10s",pwd->pw_name);
				else printf(" %-8d",statbuf.st_uid);
				if((grp=getgrgid(statbuf.st_gid))!=NULL)printf(" %-8.10s",grp->gr_name);
				else printf(" %-8d",statbuf.st_gid);
				printf(" %9jd",(intmax_t)statbuf.st_size);
				tm=localtime(&statbuf.st_mtime);
				strftime(datestring,sizeof(datestring),nl_langinfo(D_T_FMT),tm);
				printf(" %s %s\n",datestring,namelist[i]->d_name);
				free(namelist[i]);
			}
			free(namelist);
			return 0;
		}
		else{
			struct dirent **namelist;
			int n;
			n=scandir(".", &namelist, NULL, alphasort);
			if(n<0){
				perror("Error while fetching direcory");
				return 0;
			}
			int i;
			free(namelist[0]);
			free(namelist[1]);
			for(i=2;i<n;i++){
				printf("%s  ", namelist[i]->d_name);
				free(namelist[i]);
			}
			printf("\n");
			free(namelist);
			return 0;
		}
	}
	if(strcmp(cmdl[0],"cp")==0){
		if(n<3){
			printf("cp: missed operand\nUsage : cp <file1> <file2>");
			return 0;
		}
		struct stat statbuf1;
		struct stat statbuf2;
		if(stat(cmdl[1],&statbuf1)==-1){
			perror("Fetching file1 details failed");
			return 0;
		}
		if(stat(cmdl[2],&statbuf2)==-1){
			perror("Fetching file2 details failed");
			return 0;
		}
		if(statbuf2.st_mtime-statbuf1.st_mtime<0){
			int frd,fwr,read_bytes;
			char buf[5];
			frd=open(cmdl[1],O_RDONLY);
			if(frd==-1){
				perror("File1 could not be opened");
				return 0;
			}
			fwr=open(cmdl[2],O_TRUNC | O_WRONLY ,S_IRUSR | S_IWUSR);
			if(fwr==-1){
				perror("File2 could not be opened");
				return 0;
			}
			while(1){
				read_bytes=read(frd,buf,1);
				if(read_bytes==-1){
					perror("Error while reading from file1");
					return 0;
				}
				if(read_bytes==0){
					close(frd);
					close(fwr);
					printf("File successfully copied\n");
					return 0;
				}
				write(fwr,buf,read_bytes);
			}
		}
		else printf("File2 is more recent than File1, nothing copied.\n");
		return 0;
	}
	if(strcmp(cmdl[0],"exit")==0){
		printf("Bye\n");
		for(int i=0;i<CMD_IN_LINE;i++)free(exec_files[i]);
		exit(1);
	}
	return 1;
}

void execute_piping(char cmdl[][CMD_LINE_LEN+1],int n){
	int id,id2,id3,fd[2],fd2[2],i,j,k,l;
	char *out1[CMD_LINE_LEN+1];
	char *out2[CMD_LINE_LEN+1];
	char *out3[CMD_LINE_LEN+1];
	int waiting=1,status=0;
	if(strcmp(cmdl[n-1],"&")==0){
		waiting=0;
		n--;
	}
	pipe(fd);
	pipe(fd2);
	for(l=0;l<CMD_IN_LINE;l++)out1[l]=(char *)malloc((CMD_LINE_LEN+1)*sizeof(char));
	for(i=j=0;i<n;i++){
		if(strcmp(cmdl[i],"|")==0)break;
		strcpy(out1[j],cmdl[i]);
		j++;
	}
	out1[j]=(char *)NULL;
	for(l=0;l<CMD_IN_LINE;l++)out2[l]=(char *)malloc((CMD_LINE_LEN+1)*sizeof(char));
	for(k=0,i++;i<n;i++){
		if(strcmp(cmdl[i],"|")==0)break;
		strcpy(out2[k],cmdl[i]);
		k++;
	}
	out2[k]=(char *)NULL;
	if(k==0){
		printf("Error while executing, Usage : executable_file1 | executable_file2\n");
		return;
	}
	for(l=0;l<CMD_IN_LINE;l++)out3[l]=(char *)malloc((CMD_LINE_LEN+1)*sizeof(char));
	for(l=0,i++;i<n;i++){
		strcpy(out3[l],cmdl[i]);
		l++;
	}
	out3[l]=(char *)NULL;
	id=fork();
	if(id==0){
		close(1);
		dup(fd[1]);
		close(fd[0]);
		execvp(out1[0],out1);
		perror("Error while executing file 1");
		exit(-1);
	}
	else{
		if(waiting)waitpid(id,&status,0);
		id2=fork();
		if(id2==0){
			close(0);
			dup(fd[0]);
			close(fd[1]);
			if(l>0){
				close(1);
				close(fd2[0]);
				dup(fd2[1]);
			}
			execvp(out2[0],out2);
			perror("Error while executing file 2");
			exit(-1);
		}
		else{
			if(waiting)waitpid(id2,&status,0);
			if(l>0){
				id3=fork();
				if(id3==0){
					close(0);
					dup(fd2[0]);
					close(fd2[1]);
					execvp(out3[0],out3);
					perror("Error while executing file 3");
					exit(-1);
				}
				else if(waiting)waitpid(id3,&status,0);
			}
		}
	}
	return;
}

int main(){
	char cl[CMD_LINE_LEN+1];
	char cmdl[CMD_IN_LINE][CMD_LINE_LEN+1];
	int cmd_code;
	int n,i;
	for(i=0;i<CMD_IN_LINE;i++)exec_files[i]=(char *)malloc((CMD_LINE_LEN+1)*sizeof(char));
	while(1){
		printPrompt();
		int type=0;
		n=read_input(cl,cmdl,&type);
//		printf("read_input = %s, n = %d\n",cl,n);
		if(builtin(cmdl,n)){
			int waiting=1;
			if(strcmp(cmdl[n-1],"&")==0)waiting=0;
			int id=fork();
			if(id==0){
				if(type==2){
					execute_piping(cmdl,n);
					exit(1);
				}
				else{
					int into=0;
					int outfrom=0;
					int j;
					char intofile[CMD_LINE_LEN+1],outfromfile[CMD_LINE_LEN+1];
					for(i=j=0;i<n;i++){
						if(strcmp(cmdl[i],"&")==0)continue;
						if(cmdl[i][0]=='<'){
							if(strcmp(cmdl[i],"<")==0){
								i++;
								if(i<n){
									outfrom=1;
									strcpy(outfromfile,cmdl[i]);
									continue;
								}
								else{
									printf("Incorrect implementation, Usage : < outputfile\n");
									exit(-1);
								}
							}
							outfrom=1;
							strcpy(outfromfile,&cmdl[i][1]);
							continue;
						}
						if(cmdl[i][0]=='>'){
							if(strcmp(cmdl[i],">")==0){
								i++;
								if(i<n){
									into=1;
									strcpy(intofile,cmdl[i]);
									continue;
								}
								else{
									printf("Incorrect implementation, Usage : > inputfile\n");
									exit(-1);
								}
							}
							into=1;
							strcpy(intofile,&cmdl[i][1]);
							continue;
						}
						strcpy(exec_files[j],cmdl[i]);
						j++;
					}
					if(into){
						close(1);
						if(dup(open(intofile,O_TRUNC | O_WRONLY | O_CREAT,S_IRUSR | S_IWUSR))==-1){
							perror("Redirection of output failed");
							exit(-1);
						}
					}
					if(outfrom){
						close(0);
						if(dup(open(outfromfile,O_RDONLY))==-1){
							perror("Redirection of output failed");
							exit(-1);
						}
					}
					exec_files[j]=(char *)NULL;
					if(execvp(exec_files[0],exec_files)==-1){
						perror("Execution command failed");
						exit(-1);
					}
					exit(1);
				}
			}
			else{
				if(waiting){
					int status=0;
					waitpid(id,&status,0);
					continue;
				}
			}
		}
	}
}
