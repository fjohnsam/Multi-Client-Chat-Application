
/*
	Assignment 3
	Roll No : 19CS60R53
	Febin John Sam
	server.c
*/

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<strings.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<string.h>
#include<ctype.h>
#include<fcntl.h>
#include<time.h>
#include<sys/time.h>

#define PORTNO 4000
#define clientlimit 50
#define maxgroups 5
#define maxgroupmembers 10

struct group{
	int groupid;
	int admin;
	int members;
	int memberid[maxgroupmembers];
	int joined[maxgroupmembers];
};

void activeclients(int clients_connected, int *active_client_ids,int id ){
	char client_id_string[5];
	char active_client_details[250]="/active";
	for(int k=0 ; k<clients_connected ; k++){
		//printf("Active client id %d\n",*(active_client_ids+k));
		sprintf(client_id_string,"%d-",*(active_client_ids+k));
		strncat(active_client_details,client_id_string,5);
	}
	if(write(id,active_client_details,255)<=0){
		printf("Error writing to client. Exiting\n");
		exit(-1);
	}
	printf("Sending active client list to %d\n",id);
	strncpy(active_client_details,"",255);
}

void sendmessage(char *message, int reciever_id){
	if(write(reciever_id,message,255)<=0){
		printf("Error writing to client. Exiting\n");
		exit(-1);
	}
}

void remove_client(int clients_connected,int *list , int id){
	int i;
	for( i=0 ; i < clients_connected ; i++){
		if(*(list + i) == id)
			break;
	}
	for(;i < clients_connected-1 ; i++){
		*(list + i) = *(list + (i+1)); 
	}
}

void removespaces(char *buffer){
  int i=0;
  while(buffer[i]==' '){
    i++;
  }
  for(int j=0;j<=255-i-1;j++){
    buffer[j]=buffer[j+i];
  }
}
/*
  Quit command process
*/
void Quit(int newsockfd,char buffer[])
{
  char message[]="quit";
  if(write(newsockfd,message,strlen(message)))
    printf("Client %d quit\n",newsockfd);
  else
    printf("Error in quitting\n");
}

int main(){

	struct group groups[maxgroups];
	int n_groups=0;
	int ngrp=0;

	// Generating random numbers
	srand(time(NULL));

	fd_set master;
	fd_set read_fds;
	int fdmax;
	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	int x;

	int active_client_ids[clientlimit];//list of ids of active clients
	int clients_connected = 0;//number of clients connected

	int sockfd;
	/*Creating new connections listner socket*/
	struct sockaddr_in serv_addr;
	bzero((char*)&serv_addr,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORTNO);
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd <0){
		perror("server socket failed\n");
		exit(-1);
	}
  	printf("Socket created\n");
  	/*binding ip and socket*/
	if(bind(sockfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
		printf(" Error on Binding\n");
		exit(-1);
	}
	printf("Binding success\n");

	/*Listening on socket*/
	listen(sockfd,1);

	/*for listening from client socket*/
	struct sockaddr_in cli_addr;
	socklen_t clilen;
	clilen=sizeof(cli_addr);
	int newsockfd;

	/*Initializing select function master file descriptor list*/
	FD_SET(sockfd,&master);
	fdmax = sockfd;

	int n;
	char buffer[255],id[255];

	while(1){

		read_fds = master;

		if(select(fdmax+1,&read_fds,NULL,NULL,NULL)==-1){
			printf("Error in select");
			exit(-1);
		}

		for(int i=0;i<=fdmax;i++){

			if(FD_ISSET(i,&read_fds)){

				/*handle new connections*/
				if(i==sockfd){

					clilen=sizeof(cli_addr);
					newsockfd = accept(sockfd,(struct sockaddr*)&cli_addr,&clilen);
					if(newsockfd <0)
						{
							printf("Error in accepting connection from client \n");
							exit(-1);
						}
					else{		
						/*Return Connection limit exceeded*/
						if(clients_connected > clientlimit){
							strncpy(buffer,"ConnectionLimitExceeded",255);
							write(newsockfd,buffer,strlen(buffer));
							break;
						}
						else{
							//x = rand()%99999 +1;
							sprintf(buffer,"%d",newsockfd);
							strncpy(buffer,buffer,255);

							active_client_ids[clients_connected]=newsockfd;

							write(newsockfd,buffer,strlen(buffer));
							clients_connected++;

							FD_SET(newsockfd,&master);

							if(newsockfd > fdmax){
								fdmax = newsockfd;
							}

							printf(" Connected with client %d \n",newsockfd);
						}
					}
				}

				/*handle data from client*/
				else{
					bzero(buffer,255);
					strncpy(buffer,"",255);

					if(n = read(i,buffer,255)<=0){
						if(n==0){
							printf("This client connection closed\n");
						}
						else{
							printf("Client %d quit\n",i);
						}
						close(i);
						FD_CLR(i,&master);
					}

					else{
						removespaces(buffer);
				
						/*Client sending message to other clients*/
						if(strncmp(buffer,"/broadcast",10) == 0){
							int j;
							char buffer2[255];
							strncpy(buffer2,"broadcast message: ",19);
							for(j=0;j<strlen(buffer)-10;j++){
								buffer2[j+19]=buffer[j+10];
							}
							buffer2[j+19]='\0';
							for(int l=0;l<clients_connected;l++){
								if(l!=i){
									sendmessage(buffer2,active_client_ids[l]);
								}
							}
						}
						/*makegroupreq*/
						else if(strncmp(buffer,"/makegroupreq",12) == 0){
							groups[n_groups].groupid=ngrp+1;
							groups[n_groups].members=0;
							groups[n_groups].admin=0;
							int j=0,k;
							int flag=0;
							int x;
							char c[10];
							char d[5];
							char buffer2[255];
							for(k=14;k<strlen(buffer);k++){
								if((buffer[k]==' ')||(buffer[k]=='\n')){
									c[j]='\0';
									sscanf(c,"%d",&x);
									if(i==x){
										groups[n_groups].admin=i;
										groups[n_groups].joined[groups[n_groups].members]=1;
									}
									else{
										groups[n_groups].joined[groups[n_groups].members]=0;
										
									}
									//printf("Member is %d\n",x);
									groups[n_groups].memberid[groups[n_groups].members]=x;
									groups[n_groups].members++;
									flag=0;
									for(int l=0;l<clients_connected;l++){
										if(x==active_client_ids[l])
											flag=1;
									}
									if(flag==0){
										strncpy(buffer,"Some clients dont exist",255);
										if(write(i,buffer,255)<=0){
											printf("Client quit\n");
										}
										continue;
									}
									j=0;
									removespaces(buffer);
								}
								else{
									c[j]=buffer[k];
									j++;
								}
							}
							if((groups[n_groups].admin!=0)&&flag==1){
								
								
								for(int k=0;k<groups[n_groups].members;k++){
									if(groups[n_groups].memberid[k]!=i){
										//printf("%d!=%d\n",groups[n_groups].memberid[k],i);
										strncpy(buffer2,"join group ",255);
										sprintf(d,"%d ",groups[n_groups].groupid);
										strncat(buffer2,d,5);
										if(write(groups[n_groups].memberid[k],buffer2,255)<=0){
											printf("Client quit\n");
										}
										bzero(buffer2,255);
										strncpy(buffer2,"",255);
									}
								}
								n_groups++;
								ngrp++;
								printf("Group %d formed\n",n_groups);
							}
							else{
								strncpy(buffer2,"Invalid!",255);
								if(write(i,buffer2,255)<=0){
									printf("Client quit\n");
								}
								bzero(buffer2,255);
								strncpy(buffer2,"",255);
								printf("Group not formed\n");
							}
						}
						/*Join group*/
						else if(strncmp(buffer,"/joingroup",10) ==0 ){
							int k=11,j=0;
							char c[10];
							int x;
							while((buffer[k]!=' ')||(buffer[k]!='\n')){
								c[j]=buffer[k];
								k++;
								j++;
								if(j>4)
									break;
							}
							c[j]='\0';
							sscanf(c,"%d",&x);
							//printf("Checking for group %d\n",x);
							//check if group exists
							int grouploc=0,flag=0;
							for(int l=0;l<n_groups;l++){
								//printf("l=%d,group id=%d\n",l,groups[l].groupid);
								if(x==groups[l].groupid){
									grouploc=l;
									flag=1;
								}
							}
							if(flag==0){
								strncpy(buffer,"Group doesnt exist",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
								continue;
							}
							//checking if client is a member of group
							flag=0;
							int loc=0;
							for(int l=0;l<groups[grouploc].members;l++){
								if((i==groups[grouploc].memberid[l])){
									flag=1;
									loc=l;
								}
							}
							if(flag==0){
								//sending request to admin for joining group
								groups[grouploc].memberid[groups[grouploc].members]=i;
								groups[grouploc].joined[groups[grouploc].members]=-2;
								groups[grouploc].members++;
								sprintf(buffer,"Client %d requesting to join group %d",i,x);
								strncpy(buffer,buffer,255);
								if(write(groups[grouploc].admin,buffer,255)<=0){
									printf("Client quit\n");
								}
							}
							else if(groups[grouploc].joined[loc]==0){
								groups[grouploc].joined[loc]=1;
								strncpy(buffer,"You have joined the group",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
							}
							else if(groups[grouploc].joined[loc]==1){
								//printf("checking joined for %d\n",groups[grouploc].memberid[loc]);
								strncpy(buffer,"You are already in the group",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
							}
							else if(groups[grouploc].joined[loc]==-1){
								groups[grouploc].joined[loc]=-2;
								sprintf(buffer,"Client %d requesting to join group %d",i,x);
								strncpy(buffer,buffer,255);
								if(write(groups[grouploc].admin,buffer,255)<=0){
									printf("Client quit\n");
								}
							}

						}
						/*Decline group request*/
						else if(strncmp(buffer,"/declinegroup",13) ==0) {
							int k=14,j=0;
							char c[10];
							int x;
							while((buffer[k]!=' ')||(buffer[k]!='\n')){
								c[j]=buffer[k];
								k++;
								j++;
								if(j>4)
									break;
							}
							c[j]='\0';
							sscanf(c,"%d",&x);
							//check if group exists
							int grouploc=0,flag=0;
							for(int l=0;l<n_groups;l++){
								if(x==groups[l].groupid){
									grouploc=l;
									flag=1;
								}
							}
							if(flag==0){
								strncpy(buffer,"Group doesnt exist",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
								continue;
							}
							//checking if client is a member of group
							flag=0;
							int loc=0;
							for(int l=0;l<groups[grouploc].members;l++){
								if((i==groups[grouploc].memberid[l])){
									flag=1;
									loc=l;
								}
							}
							if(flag==0){
								strncpy(buffer,"You do not exist in this group",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
								continue;
							}
							if(groups[grouploc].joined[loc]==0){
								groups[grouploc].joined[loc]=-1;
								strncpy(buffer,"You have declined the group",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
							}
							else if(groups[grouploc].joined[loc]==1){
								strncpy(buffer,"You have already joined the group",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
							}
							else if(groups[grouploc].joined[loc]==-1){
								strncpy(buffer,"You had already declined the group request",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
							}

						}
						/*makegroup*/
						else if(strncmp(buffer,"/makegroup",10) ==0 ){
							//printf("command = %s",buffer);
							groups[n_groups].groupid=ngrp+1;
							groups[n_groups].members=0;
							groups[n_groups].admin=0;
							int j=0,k;
							int flag=0;
							int x;
							char c[10];
							char buffer2[255];
							for(k=11;k<strlen(buffer);k++){
								flag=0;
								if((buffer[k]==' ')||(buffer[k]=='\n')){
									c[j]='\0';
									sscanf(c,"%d",&x);
									//printf("k is %d, x is %d\n",k,x);
									if(i==x)
										groups[n_groups].admin=i;
									groups[n_groups].memberid[groups[n_groups].members]=x;
									groups[n_groups].joined[groups[n_groups].members]=1;
									//printf("storing %d as member %d\n",groups[n_groups].memberid[groups[n_groups].members],groups[n_groups].members);
									groups[n_groups].members++;
									for(int l=0;l<clients_connected;l++){
										if(x==active_client_ids[l])
											flag=1;
									}
									if(flag==0){
										strncpy(buffer,"Some clients dont exist",255);
										if(write(i,buffer,255)<=0){
											printf("Client quit\n");
										}
										break;
									}
									j=0;
								}
								else{
									c[j]=buffer[k];
									//printf("c[%d] is %c\n",j,c[j]);
									if(!isdigit(c[j])){
										flag=0;
										break;
									}
									j++;
								}
							}
							if((groups[n_groups].admin!=0)&&(flag==1)){
								n_groups++;
								ngrp++;
								printf("Group %d formed\n",n_groups);
							}
							else{
								strncpy(buffer2,"Invalid!",255);
								if(write(i,buffer2,255)<=0){
									printf("Client quit\n");
								}
								bzero(buffer2,255);
								strncpy(buffer2,"",255);
								printf("Group not formed\n");
							}
						}
						/*Active Groups*/
						else if(strncmp(buffer,"/activegroups",13) ==0 ){
							//checking for invalid command
							int j=13,flag=0;
							while(buffer[j]!='\n'){
								if(buffer[j]!=' '){
									strncpy(buffer,"Invalid command",255);
									if(write(i,buffer,255)<=0){
										printf("Client quit\n");
									}
									flag=1;
									break;
								}
								j++;
							}
							if(flag==1)
								break;
							char c[5];
							flag=0;		
							char message[250]="Your active groups are: ";
							for(j=0;j<n_groups;j++){
								sprintf(c,"%d ",groups[j].groupid);
								//checking if client present in the group
								for(int k=0;k<groups[j].members;k++){
									if((i==groups[j].memberid[k])&&groups[j].joined[k]==1){
										flag=1;
									}
								}
								if(flag==1){
									strncat(message,c,5);
								}
								flag=0;
							}
							if(write(i,message,255)<=0){
								printf("Client quit\n");
							}
						}
						/*Active All Groups*/
						else if(strncmp(buffer,"/activeallgroups",16) ==0 ){
							//checking for invalid command
							int j=16,flag=0;
							while(buffer[j]!='\n'){
								if(buffer[j]!=' '){
									strncpy(buffer,"Invalid command",255);
									if(write(i,buffer,255)<=0){
										printf("Error writing to client. Exiting\n");
									}
									flag=1;
									break;
								}
								j++;
							}
							if(flag==1)
								break;
							char c[5];
							char message[250]="All active groups are: ";
							for(j=0;j<n_groups;j++){
								sprintf(c,"%d ",groups[j].groupid);
								strncat(message,c,5);
							}
							if(write(i,message,255)<=0){
								printf("Client quit\n");
							}
						}
						/*Send Group Message*/
						else if(strncmp(buffer,"/sendgroup",10) ==0 ){
							int k=11,j;
							char c[2];
							int x;
							while(buffer[k]!=' '){
								c[k-11]=buffer[k];
								k++;
								if(k>12)
									break;
							}
							sscanf(c,"%d",&x);
							//check if group exists
							int grouploc=0,flag=0;
							for(int l=0;l<n_groups;l++){
								if(x==groups[l].groupid){
									grouploc=l;
									flag=1;
								}
							}
							if(flag==0){
								strncpy(buffer,"Group doesnt exist",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit \n");
								}
								continue;
							}
							//message
							sprintf(c,"%d",x);
							char buffer2[255];
							strncpy(buffer2,"Message from Group ",19);
							strncat(buffer2,c,2);
							if(!isdigit(buffer2[20]))
								buffer2[20]=' ';
							buffer2[21]=':';
							for(j=k+1;j<255-25;j++){
								buffer2[j-(k+1)+22]=buffer[j];
								if(buffer[j]=='\0')
									break;
							}

							buffer2[254]='\0';
							//checking if client is a member of group
							flag=0;
							for(int l=0;l<groups[grouploc].members;l++){
								if((i==groups[grouploc].memberid[l])&&(groups[grouploc].joined[l])==1)
									flag=1;
							}
							if(flag==0){
								strncpy(buffer2,"You do not exist in this group",255);
								if(write(i,buffer2,255)<=0){
									printf("Client quit\n");
								}
								continue;
							}
							//Sending message to each client in group
							for(int l=0;l<groups[grouploc].members;l++){
								if(groups[grouploc].joined[l]==1){
									if(write(groups[grouploc].memberid[l],buffer2,255)<=0){
										//if a member had quit
										for(int m=l;m<groups[grouploc].members-1;m++){
											groups[grouploc].memberid[l]=groups[grouploc].memberid[l+1];
										}
										groups[grouploc].members--;
										l--;
									}
								}
							}
						}
						/*Client sending message to other clients*/
						else if(strncmp(buffer,"/send",5) == 0){
							int k=6,j;
							char c[2];
							int x;
							while(buffer[k]!=' '){
								c[k-6]=buffer[k];
								k++;
								if(k>7)
									break;
							}
							sscanf(c,"%d",&x);
							/*sender and reciever are same*/
							if(x==i){
								strncpy(buffer,"Cant send to yourself",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
								continue;
							}
							/*If reciever doesnt exist*/
							int reciever_exists = 0;
							for(int l=0;l<clients_connected;l++){
								if(x==active_client_ids[l]){
									reciever_exists=1;
								}
							}
							if(reciever_exists==0){
								strncpy(buffer,"Reciever doesnt exist",255);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
								continue;
							}
							sprintf(c,"%d",i);
							char buffer2[255];
							strncpy(buffer2,"Message from client ",20);
							strncat(buffer2,c,2);
							if(!isdigit(buffer2[21]))
								buffer2[21]=' ';
							buffer2[22]=':';
							for(j=k+1;j<255-23;j++){
								buffer2[j-(k+1)+23]=buffer[j];
							}
							
							buffer2[254]='\0';
							printf("Client %d sent message to client %d\n",i,x);
							sendmessage(buffer2,x);
						}
						/*Client asking for active clients list*/
						else if(strncmp(buffer,"/active",7) == 0){
							//checking for invalid command
							int j=7,flag=0;
							while(buffer[j]!='\n'){
								if(buffer[j]!=' '){
									strncpy(buffer,"Invalid command",255);
									if(write(i,buffer,255)<=0){
										printf("Client quit\n");
									}
									flag=1;
									break;
								}
								j++;
							}
							if(flag==0)
								activeclients(clients_connected,active_client_ids,i);
						}
						/*Client wants to quit*/
						else if( strncmp(buffer,"/quit",5) == 0 ){
							//checkingif proper command
							int j=5,flag=0;
							while(buffer[j]!='\n'){
								if(buffer[j]!=' '){
									strncpy(buffer,"Invalid command",255);
									if(write(i,buffer,255)<=0){
										printf("Client quit\n");
									}
									flag=1;
									break;
								}
								j++;
							}
							if(flag==1)
								break;
							//removing groups 
							int k,l,present=0,loc=0;
							for(j=0;j<n_groups;j++){
								for(k=0;k<groups[j].members;k++){
									if(i==groups[j].memberid[k]){
										present=1;
										loc=k;
									}
								}
								if(present==1){
									//groups where admin
									if(i==groups[j].admin){
										for(k=j;k<n_groups-1;k++){
											groups[k].groupid = groups[k+1].groupid;
											groups[k].members = groups[k+1].members;
											groups[k].admin = groups[k+1].admin;
											for(l=0;l<groups[k+1].members;l++){
												groups[k].memberid[l] = groups[k+1].memberid[l];
												groups[k].joined[l] = groups[k+1].joined[l];
											}
										}
										j--;
										n_groups--;
									}
									//groups where not admin
									else{
										for(k=loc;k<groups[j].members-1;k++){
											groups[j].memberid[k]=groups[j].memberid[k+1];
											groups[j].joined[k]=groups[j].joined[k+1];
											groups[j].members--;
										}
									}
								}
								present=0;
							}
							//removing client
							remove_client(clients_connected,active_client_ids,i);
							Quit(i,buffer);
							close(i);
							FD_CLR(i,&master);
							clients_connected--;
							break;
						}
						else if(strncmp(buffer,"/groupmembers",13) == 0){
							int k=14,j,l,flag=0;
							char c[15];
							int x;
							while((buffer[k]!=' ')&&(buffer[k]!='\n')){
								c[k-14]=buffer[k];
								k++;
								if(k>28)
									break;
							}
							sscanf(c,"%d",&x);
							//printf("x is %d\n",x);
							while(buffer[k]!='\n'){
								if(buffer[k]!=' '){
									strncpy(buffer,"Invalid Command1",255);
									if(write(i,buffer,255)<=0){
										printf("Client quit\n");
									}
									flag=1;
									break;
								}
								k++;
							}
							if(flag==1)
								break;
							//checking if group exists
							flag=0;
							for(j=0;j<n_groups;j++){
								if(groups[j].groupid==x){
									flag=1;
								}
							}
							if(flag==0){
								strncpy(buffer,"Group doesnt exist",18);
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
								break;
							}
							strncpy(buffer,"Group members are:",18);
							for(j=0;j<n_groups;j++){
								if(groups[j].groupid==x){
									for(l=0;l<groups[j].members;l++){
										sprintf(c,"%d ",groups[j].memberid[l]);
										strncat(buffer,c,sizeof(c));
									}
								}
							}
							//printf("buffer is %s\n",buffer);
							if(write(i,buffer,255)<=0){
								printf("Client quit\n");
							}

						}
						else if(strncmp(buffer,"/acceptmembertogroup",20) == 0){
							int x=0,y=0,j=21,k=0,flag1=0,flag2=0;
							char c[5]="";
							while(buffer[j]!=' '){
								c[k]=buffer[j];
								//printf("c[%d]=%c",k,c[k]);
								j++;
								k++;
								if(k>3)
									break;
							}
							c[k]='\0';
							sscanf(c,"%d",&x);
							k=0;
							j++;
							strncpy(c,"",5);
							while((buffer[j]!=' ')&&(buffer[j]!='\n')){
								c[k]=buffer[j];
								//printf("c[%d]=%c",k,c[k]);
								j++;
								k++;
								if(k>3)
									break;
							}
							c[k]='\0';
							sscanf(c,"%d",&y);
							//printf("x=%d,y=%d\n",x,y);
							//check if group exist and client is admin
							for(j=0;j<n_groups;j++){
								if((groups[j].groupid==x)&&(groups[j].admin==i)){
									flag2=1;
									//printf("Check0\n");
									for(k=groups[j].members-1;k>-1;k--){
										//printf("member %d\n",groups[j].memberid[k]);
										//if client in group list
										if(groups[j].memberid[k]==y){
											flag1=1;
											//printf("Check1\n");
											//client already in group
											if(groups[j].joined[k]==1){
												strncpy(buffer,"Client already in group",255);
												if(write(i,buffer,255)<=0){
													printf("Client quit\n");
												}
											}
											//client accept
											else if(groups[j].joined[k]==-2){
												groups[j].joined[k]=1;
												sprintf(buffer,"You are accepted in group %d",x);
												if(write(y,buffer,255)<=0){
													printf("Client quit\n");
												}
											}
											//client already requested to join group
											else if(groups[j].joined[k]==0){
												sprintf(buffer,"Client %d has been asked to join group %d",y,x);
												if(write(y,buffer,255)<=0){
													printf("Client quit\n");
												}
											}
										}
									}
									if(flag1=0){
										//client didnt request join
										sprintf(buffer,"Client %d didnt request to join group %d",y,x);
										if(write(i,buffer,255)<=0){
											printf("Client quit\n");
										}
									}
								}
							}
							if(flag2=0){
								sprintf(buffer,"Invalid");
								if(write(i,buffer,255)<=0){
									printf("Client quit\n");
								}
							}
						}
						else{
							strncpy(buffer,"Invalid Command",255);
							if(write(i,buffer,255)<=0){
								printf("Client quit\n");
							}
							continue;
						}
					}
				}
			}
		}		
	}
}