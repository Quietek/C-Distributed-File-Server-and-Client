#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#define LineSize 256
#define FourKB 4096

struct config
{
	char ip_DFS1[15];
	char ip_DFS2[15];
	char ip_DFS3[15];
	char ip_DFS4[15];
	int port_DFS1;
	int port_DFS2;
	int port_DFS3;
	int port_DFS4;
	char username[LineSize];
	char password[LineSize];
};

struct config ReadConfig(char* FileName);
void error(char *msg);
void PutFile(char *filename, struct config configuration);
void GetFile(char *filename);
void ListFiles();
char cmd[LineSize];
int MD5File(FILE *file);
int ConnectServer(char *address, int port);

int main(int argc, char** argv) {
	char *token;
	if (argc != 2)
	{
		printf("Usage: %s <ConfigFileName>",argv[0]);
		exit(1);
	}
	struct config Configuration = ReadConfig(argv[1]);
	printf("Configuration successfully read with the following values...\n");
	printf("ip_DFS1: %s Port:%d\nip_DFS2: %s Port:%d\nip_DFS3: %s Port:%d\nip_DFS4: %s Port:%d\nUsername: %s\nPassword: %s",Configuration.ip_DFS1,Configuration.port_DFS1,Configuration.ip_DFS2,Configuration.port_DFS2,Configuration.ip_DFS3,Configuration.port_DFS3,Configuration.ip_DFS4,Configuration.port_DFS4,Configuration.username,Configuration.password);
    printf("Plese use one of the following commands:\nlist: list files available on DFS\nput <filename>: distribute the specified file on the various DFS systems\nget <filename>: retrieve the file from the DFS filesystems\n");
    while(1)
    {
		fgets (cmd, LineSize, stdin);
		token = strtok(cmd, " ");
		if(!strcmp(token,"put"))
		{
			token = strtok(NULL,"\n");
			PutFile(token,Configuration);
		}
		else if (!strcmp(token,"list"))
		{
			ListFiles();
		}
		else if (!strcmp(token,"get"))
		{
			token = strtok(NULL,"\n");
			GetFile(token);
		}
	}
    
    
    printf("Client Closing...\n");
    return 0;
}
/*
 * ReadConfig - reads information in from the config file specified on command line
 */ 
struct config ReadConfig(char* FileName){
	//variable declarations
	struct config Conf;
	char line[FourKB];
	char *token;
	//open the config file
	FILE* ConfigFile = fopen(FileName, "r");
	//make sure the file opened properly
	if (ConfigFile != NULL)
	{
		//read the file line by line
		while(fgets(line,FourKB,ConfigFile))
		{
			//read the first part of the line before the first space
			token = strtok(line, " ");
			//match that you're looking at information on one of the servers
			if (!strcmp("Server",token))
			{
				//get the next part of the line
				token = strtok(NULL, " ");
				{
					//find which DFS server we're reading information on
					//repearted for DFS1,2,3 and 4
					if (!strcmp("DFS1",token))
					{
						//find the ip
						token = strtok(NULL,":");
						//copy the ip into the Config struct
						strcpy(Conf.ip_DFS1,token);
						//find the port
						token = strtok(NULL,"");
						//copy the port into the Config struct
						Conf.port_DFS1 = atoi(token);
					}
					else if (!strcmp("DFS2",token))
					{
						token = strtok(NULL,":");
						strcpy(Conf.ip_DFS2,token);
						token = strtok(NULL,"");
						Conf.port_DFS2 = atoi(token);
					}
					else if (!strcmp("DFS3",token))
					{
						token = strtok(NULL,":");
						strcpy(Conf.ip_DFS3,token);
						token = strtok(NULL,"");
						Conf.port_DFS3 = atoi(token);
					}
					else if (!strcmp("DFS4",token)) 
					{
						token = strtok(NULL,":");
						strcpy(Conf.ip_DFS4,token);
						token = strtok(NULL,"");
						Conf.port_DFS4 = atoi(token);
					}
				}
			}
			//match on username
			else if (!strcmp("Username:",token))
			{
				//copy the rest of the line into the Config struct
				token = strtok(NULL,"\n");
				strcpy(Conf.username,token);
			}
			//match on password
			else if (!strcmp("Password:",token))
			{
				//copy the rest of the line into the Config struct
				token = strtok(NULL,"");
				strcpy(Conf.password,token);
			}
		}
	}
	//throw an error if the file doesn't open
	else
	{
		error("Failed to open config file...");
	}
	fclose(ConfigFile);
	return Conf;
}

/*
 * PutFile: splits file into four pieces and pushes each piece to the server
 */
void PutFile(char *filename, struct config configuration)
{
	printf("Put Filename: %s\n",filename);
	char Buffer[FourKB];
	char tempBuffer[FourKB];
	char msg[FourKB];
	char OutFileName[LineSize];
	int ReadAmount = 0;
	int FileSize = 0;
	int PartSize = 0;
	int Overflow = 0;
	int connections[4];
	connections[0] = ConnectServer(configuration.ip_DFS1, configuration.port_DFS1);
	connections[1] = ConnectServer(configuration.ip_DFS2, configuration.port_DFS2);
	connections[2] = ConnectServer(configuration.ip_DFS3, configuration.port_DFS3);
	connections[3] = ConnectServer(configuration.ip_DFS4, configuration.port_DFS4);
	FILE *FileIn = fopen(filename, "r");
	int flag = MD5File(FileIn);
	int part1[2];
	int part2[2];
	int part3[2];
	int part4[2];

	int AuthorizationFlag;
	if (FileIn != NULL)
	{
		fseek(FileIn, 0L, SEEK_END);
		FileSize = ftell(FileIn);
		PartSize = FileSize/4;
		Overflow = FileSize % 4;
		rewind(FileIn);
		switch(flag)
		{
			case 0 :
			part1[0] = 3;
			part1[1] = 0;
			part2[0] = 0;
			part2[1] = 1;
			part3[0] = 1;
			part3[1] = 2;
			part4[0] = 2;
			part4[1] = 3;
			break;
			case 1 :
			part1[0] = 0;
			part1[1] = 1;
			part2[0] = 1;
			part2[1] = 2;
			part3[0] = 2;
			part3[1] = 3;
			part4[0] = 3;
			part4[1] = 0;
			break;
			case 2 :
			part1[0] = 1;
			part1[1] = 2;
			part2[0] = 2;
			part2[1] = 3;
			part3[0] = 3;
			part3[1] = 0;
			part4[0] = 0;
			part4[1] = 1;
			break;
			case 3 :
			part1[0] = 2;
			part1[1] = 3;
			part2[0] = 3;
			part2[1] = 0;
			part3[0] = 0;
			part3[1] = 1;
			part4[0] = 1;
			part4[1] = 2;
			break;
		}
		for(int i=1;i<=4;i++)
		{
			sprintf(OutFileName,"%s.%d",filename,i);
			sprintf(tempBuffer,"%s %s put %s",configuration.username, configuration.password, OutFileName);
			int n;
			switch (i)
			{
				case 1:
					n = send(connections[part1[0]], tempBuffer, strlen(tempBuffer), 0);
					if(n < 0)
					{
						printf("Failed to send authorization to server %d",part1[0]);
					}
					else
					{
						n = recv(connections[part1[0]], msg, FourKB, 0);
						if(!strcmp(msg,"Authorization Okay"))
						{
							AuthorizationFlag = 1;
						}
					}
					printf("Authorized first server for part 1...\n");
					n = send(connections[part1[1]], tempBuffer, strlen(tempBuffer), 0);
					if(n < 0)
					{
						printf("Failed to send authorization to server %d",part1[1]);
					}
					else
					{
						n = recv(connections[part1[1]], msg, FourKB, 0);
						if(!strcmp(msg,"Authorization Okay"))
						{
							AuthorizationFlag = 1;
						}
					}
					printf("Authorized second server for part 1...\n");
					break;
				case 2:
					n = send(connections[part2[0]], tempBuffer, strlen(tempBuffer), 0);
					if(n < 0)
					{
						printf("Failed to send authorization to server %d",part2[0]);
					}
					else
					{
						n = recv(connections[part2[0]], msg, FourKB, 0);
						if(!strcmp(msg,"Authorization Okay"))
						{
							AuthorizationFlag = 1;
						}
					}
					printf("Authorized first server for part 2...\n");
					n = send(connections[part2[1]], tempBuffer, strlen(tempBuffer), 0);
					if(n < 0)
					{
						printf("Failed to send authorization to server %d",part2[1]);
					}
					else
					{
						n = recv(connections[part2[1]], msg, FourKB, 0);
						if(!strcmp(msg,"Authorization Okay"))
						{
							AuthorizationFlag = 1;
						}
					}	
					printf("Authorized second server for part 2...\n");
					break;			
				case 3:
					n = send(connections[part3[0]], tempBuffer, strlen(tempBuffer), 0);
					if(n < 0)
					{
						printf("Failed to send authorization to server %d",part3[0]);
					}
					else
					{
						n = recv(connections[part3[0]], msg, FourKB, 0);
						if(!strcmp(msg,"Authorization Okay"))
						{
							AuthorizationFlag = 1;
						}
					}
					printf("Authorized first server for part 3...\n");
					n = send(connections[part3[1]], tempBuffer, strlen(tempBuffer), 0);
					if(n < 0)
					{
						printf("Failed to send authorization to server %d",part3[1]);
					}
					else
					{
						n = recv(connections[part3[1]], msg, FourKB, 0);
						if(!strcmp(msg,"Authorization Okay"))
						{
							AuthorizationFlag = 1;
						}
					}
					printf("Authorized second server for part 1...\n");
					break;
				case 4:
					n = send(connections[part4[0]], tempBuffer, strlen(tempBuffer), 0);
					if(n < 0)
					{
						printf("Failed to send authorization to server %d",part4[0]);
					}
					else
					{
						n = recv(connections[part4[0]], msg, FourKB, 0);
						if(!strcmp(msg,"Authorization Failed"))
						{
							AuthorizationFlag = 1;
						}
					}
					printf("Authorized first server for part 4...\n");
					n = send(connections[part4[1]], tempBuffer, strlen(tempBuffer), 0);
					if(n < 0)
					{
						printf("Failed to send authorization to server %d",part4[1]);
					}
					else
					{
						n = recv(connections[part4[1]], msg, FourKB, 0);
						if(!strcmp(msg,"Authorization Failed"))
						{
							AuthorizationFlag = 1;
						}
					}
					printf("Authorized second server for part 4...\n");
					break;
			}
			int temp = PartSize;
			if (i == 4)
			{
				temp += Overflow;
			}
			while(temp > 0 && !AuthorizationFlag)
			{
				printf("Authorized! sending file\n");
				if (temp >= FourKB)
				{
					ReadAmount = FourKB;
				}
				else
				{
					ReadAmount = temp;
				}
				fread(Buffer, 1, ReadAmount, FileIn);
				char *Buf1[FourKB];
				char *Buf2[FourKB];
				bcopy(Buffer, Buf1, ReadAmount);
				bcopy(Buffer, Buf2, ReadAmount);
				int bytes_sent[2];
				switch (i)
				{
					case 1:
						bytes_sent[0] = send(connections[part1[0]], Buf1, ReadAmount, 0);
						bytes_sent[1] = send(connections[part1[1]], Buf2, ReadAmount, 0);
						if(bytes_sent[0] < 0)
						{
							printf("Failed to send file to server %d\n",part1[0]);
						}
						if(bytes_sent[1] < 0)
						{
							printf("Failed to send file to server %d\n",part1[1]);
						}
						printf("sent pt 1!\n");
						break;
					case 2:
						bytes_sent[0] = send(connections[part2[0]], Buf1, ReadAmount, 0);
						bytes_sent[1] = send(connections[part2[1]], Buf2, ReadAmount, 0);
						if(bytes_sent[0] < 0)
						{
							printf("Failed to send file to server %d\n",part2[0]);
						}
						if(bytes_sent[1] < 0)
						{
							printf("Failed to send file to server %d\n",part2[1]);
						}
						printf("sent pt 2!\n");
						break;
					case 3:
						bytes_sent[0] = send(connections[part3[0]], Buf1, ReadAmount, 0);
						bytes_sent[1] = send(connections[part3[1]], Buf2, ReadAmount, 0);
						if(bytes_sent[0] < 0)
						{
							printf("Failed to send file to server %d\n",part3[0]);
						}
						if(bytes_sent[1] < 0)
						{
							printf("Failed to send file to server %d\n",part3[1]);
						}
						printf("sent pt 3!\n");
						break;
					case 4:
						bytes_sent[0] = send(connections[part4[0]], Buf1, ReadAmount, 0);
						bytes_sent[1] = send(connections[part4[1]], Buf2, ReadAmount, 0);
						if(bytes_sent[0] < 0)
						{
							printf("Failed to send file to server %d\n",part4[0]);
						}
						if(bytes_sent[1] < 0)
						{
							printf("Failed to send file to server %d\n",part4[1]);
						}
						printf("sent pt 4!\n");
						break;
				}
				temp -= ReadAmount;
				printf("%d\n",temp);
			}
		}
		fclose(FileIn);
	}
	else
	{
		printf("Error: Unable to open file %s\n",filename);
	}
	
}


/*
 * GetFile: grabs the various pieces from the DFSs and puts the together then writes the file to disk
 */ 
void GetFile(char *filename)
{
	printf("Get Filename: %s\n",filename);
	char InFileName[LineSize];
	char *Buffer[FourKB];
	FILE *FileOut = fopen(filename, "w");
	for(int i=1;i<=4;i++)
	{
		sprintf(InFileName,"%s.%d",filename,i);
		FILE *FileIn = fopen(InFileName, "r");
		size_t BytesRead = 0;
		while(!feof(FileIn))
		{
			BytesRead = fread(Buffer, 1, FourKB, FileIn);
			fwrite(Buffer, 1, BytesRead,FileOut); 
		}
		fclose(FileIn);
	}
	fclose(FileOut);
	printf("Read in each part of the file and wrote new file to local directory\n");
}
/*
 * ListFiles: checks which files are available on the DFS
 */ 
void ListFiles()
{
} 


int MD5File(FILE *file)
{
	MD5_CTX md5ctx;
	unsigned char md5sum[MD5_DIGEST_LENGTH];
	char *readin[FourKB];
	int bytesread = 0;
	int returnval = 0;
	MD5_Init(&md5ctx);
	bytesread = fread(readin, 1, FourKB, file);
	while(bytesread)
	{
		MD5_Update(&md5ctx, readin, bytesread);
		bytesread = fread(readin, 1, FourKB, file);
	}
	rewind(file);
	if(!MD5_Final(md5sum, &md5ctx))
	{
		printf("Failed to compute md5sum...\n");
		printf("Using 0 instead of computing md5sum...\n");
		returnval = 0;
	}
	for(int i=0; i<MD5_DIGEST_LENGTH;i++)
	{
		returnval += md5sum[i];
	}
	//printf("MD5Sum: %d\n",returnval);
	returnval = returnval % 4;
	return returnval;
}

/*
 * ConnectServer - connect to the specified server
 */ 
int ConnectServer(char *address, int port)
{
	int Socket;
	int server_size;
	struct sockaddr_in server;
	Socket = socket(AF_INET, SOCK_STREAM, 0);
	if(Socket < 0)
	{
		return -1;
	}
	server_size = sizeof(server);
	memset(&server, 0, server_size);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(address);
	server.sin_port = htons(port);
	int connectsocket = connect(Socket, (struct sockaddr *) &server, server_size);
	if(connectsocket < 0)
	{
		return -1;
	}
	return Socket;
	
}

/*
 * error - wrapper for perror
 */
void error(char* msg)
{
    perror(msg);
    exit(0);
}
