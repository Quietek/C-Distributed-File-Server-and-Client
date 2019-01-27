#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define LISTENENQ 10
#define LineSize 256
#define FourKB 4096
#define ConfigName "dfs.conf"


int Authorize(char *username, char *password);
void error(char* msg);

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		printf("Usage: %s <Directory> <port>",argv[0]);
		exit(1);
	}
	pid_t pid;
	char directory[LineSize];
	strcpy(directory,argv[1]);
	int port = atoi(argv[2]);
	int optval;
	int sockfd, *listenfd;
	struct sockaddr_in clientaddr,serveraddr;
	int serverlen = sizeof(struct sockaddr_in);
	int clientlen = sizeof(struct sockaddr_in);
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	optval = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
	
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(port);

	bind(sockfd, (struct sockaddr *) &serveraddr, serverlen);
	int flags = fcntl(sockfd,F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK); 
    listen(sockfd,LISTENENQ);
	while(1)
	{
		listenfd = malloc(sizeof(clientaddr));
		*listenfd=accept(sockfd,(struct sockaddr*)&clientaddr,&clientlen);
		if(*listenfd == -1)
		{
			if(errno == EWOULDBLOCK)
			{
			}
			else
			{
				error("ERROR when accepting connection.");
			}
			pid = 1;
		} 
		else
		{ 
			pid=fork();
		}
		if (pid!=0)
		{
			close(*listenfd);
		}
		else
		{
			char buffer[FourKB];
			char buf[FourKB];
			char newbuffer[FourKB];
			char Username[LineSize];
			char Password[LineSize];
			char cmd[LineSize];
			char filename[LineSize];
			char cwd[LineSize];
			char *token;
			int n;
			recv(*listenfd, buffer, FourKB, 0);
			token = strtok(buffer," ");
			strcpy(Username,token);
			token = strtok(NULL," ");
			strcpy(Password,token);
			token = strtok(NULL," ");
			strcpy(cmd,token);
			int flag = Authorize(Username, Password);
			if (flag)
			{
				printf("authorization failed\n");
				strcpy(newbuffer, "Authorization Failed");
				send(*listenfd,newbuffer,strlen(newbuffer),0);
			}
			else
			{
				bzero(newbuffer, FourKB);
				printf("Authorized\n");
				strcpy(newbuffer, "Authorization Okay");
				n = send(*listenfd,newbuffer,strlen(newbuffer),0);
				printf("%d\n",strcmp(cmd,"put"));
				if (!strcmp(cmd,"put"))
				{
					printf("put command received\n");
					token = strtok(NULL,"");
					getcwd(cwd, sizeof(cwd));
					sprintf(filename,"%s%s/%s",cwd,directory,token);
					//strcpy(filename,token);
					//sprintf(filename,"%s/%s",directory,token);
					bzero(buf,FourKB);
					FILE *FileOut = fopen(filename,"w");
					do
					{
						n = recv(*listenfd, buf, FourKB, 0);
						printf("Buffer: %s\n",buf);
						if(n > 0)
						{
							fwrite(buf, 1, n, FileOut);
						}
						bzero(buf,FourKB);
					}while(n>0);
					fclose(FileOut);
					printf("file written\n");
				}
				else if (strcmp(cmd,"list"))
				{
				}
				else if (strcmp(cmd,"get"))
				{
				}
			}
			
		}
	}
	
	close(*listenfd);
	return 0;
}

int Authorize(char *username, char *password)
{
	return 0;
}

void error(char* msg)
{
    perror(msg);
    exit(0);
}
