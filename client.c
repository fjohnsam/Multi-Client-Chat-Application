/*
  Assignment 3
  Roll No : 19CS60R53
  Febin John Sam
  client.c
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<strings.h>
#include<unistd.h>
#include<sys/stat.h>
#include<netdb.h>
#include<ctype.h>
#include<string.h>
#include<fcntl.h>
#include<sys/wait.h>

#define PORTNO 4000

struct client{
	int socketid;
	int uid;
};

int main(){

	int sockfd ;

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	struct hostent* server;
	server = gethostbyname("localhost");
	struct sockaddr_in serv_addr;
	bzero((char*)&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*)server->h_addr,(char*)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port = htons(PORTNO);

	if(connect(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
		printf(" Error Connecting to server\n");
		exit(-1);
	}

	char buffer[255];
	int connection_exceeded=0;
	int u_id = 0;

	/*To know if Connection limit exceeded at server*/
	if(read(sockfd,buffer,255)<0){
			printf("Error while reading from socket");
			exit(-1);
	}
	if(strncmp(buffer,"ConnectionLimitExceeded",23)==0){
			printf("Connection Limit Exceeded !!");
			connection_exceeded=1;
	}
	else{
		sscanf(buffer,"%d",&u_id);
		printf("Connection Established\n");
		printf("My user id is %d \n",u_id);
	}

	strncpy(buffer,"",255);
	bzero(buffer,255);

	int active_clients[10];
	int clients_connected = 0;

	if(connection_exceeded==1){
		printf("Connection exceeded. Exiting\n");
		close(sockfd);
		exit(0);
		return 0;
	}

	printf("\nCommands list :\n\n1. /active\n2. /send <dest client id> <Message>\n3. /broadcast <Message>\n4. /makegroup <client id1>...<client idn>\n");
	printf("5. /sendgroup <group id> <Message>\n6. /activegroups\n7. /makegroupreq <client id1>...<client idn>\n8. /joingroup <group id>\n");
	printf("9. /declinegroup <group id>\n10. /quit\n11. /activeallgroups\n12. /groupmembers <groupid>\n13.  /acceptmembertogroup <groupid> <clientid>\n\n");

	/*Reads from stdin and writes to server*/
	if(fork()!=0){
		while(1){

			//sending client message
			fgets(buffer,255,stdin);
			
			/*Asking for active list*/
			if(!write(sockfd,buffer,strlen(buffer))){
				printf("Error while writing to socket\n");
	      		exit(-1);
			}

			if(strncmp(buffer,"/quit",5)==0){
				int j=5,flag=0;
				while(buffer[j]!='\n'){
					if(buffer[j]!=' '){
						flag=1;
						break;
					}
					j++;
				}
				if(flag==0)
					printf("Child process quiting.\n");
				break;
			}

			bzero(buffer,255);
			strcpy(buffer,"");
			bzero(buffer,255);
			buffer[0]='\0';
		}
		while(wait(NULL)>0);
	}
	/*Reads from server*/
	else{
		while(1){
			//reading server message
			if(read(sockfd,buffer,255)<0){
				printf("Error while reading from socket");
				exit(-1);
			}
			if(strlen(buffer)==0){
				continue;
			}
			//printf("Message  recieved %s\n",buffer);
			if(strncmp(buffer,"/active",7)==0){

				//Extracting active client details
				int i=7,j=0,clients=0;
				char c[5];
				while(i<strlen(buffer)){
					if(buffer[i]=='-'){
						sscanf(c,"%d",&active_clients[clients]);
						//printf("saving client %d\n",active_clients[clients]);
						clients++;
						j=0;
					}
					else{
						c[j]=buffer[i];
						j++;
					}
					i++;
				}

				clients_connected = clients;

				//printf("Clients connected %d\n",clients_connected);
				printf("The Active client list is\n");
				for(int i=0;i<clients_connected;i++){
					printf("%d\n",active_clients[i]);
				}
			}
			else if(strncmp(buffer,"quit",4)==0){
				printf("Parent process quitting\n");
				break;
			}
			else if(strncmp(buffer,"InvalidCommand",14)==0){
				printf("Invalid Command\n");
				continue;
			}
			else{
				printf("%s\n",buffer);
			}
			bzero(buffer,255);
			strncpy(buffer,"",255);
		}
	}
	close(sockfd);
	exit(0);
}