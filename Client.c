#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define servAddr "192.168.56.25" 

void * doRecieving(void * sockID){

	int clientSocket = *((int *) sockID);

	while(1){

		char data[1024];
		int read = recv(clientSocket,data,1024,0);
		data[read] = '\0';
		printf("%s\n",data);

	}

}

int main(){

	// gniazdo klienta
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serverAddr;

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);	// port serwera proxy
	serverAddr.sin_addr.s_addr = inet_addr(servAddr);
  
	if(connect(clientSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr)) == -1) return 0;

	printf("Connection established ............\n");

	pthread_t thread;
	pthread_create(&thread, NULL, doRecieving, (void *) &clientSocket );
	char userName[30];
	while(1){

		char input[1024];
		scanf("%s",input);

		if(strcmp(input,"LIST") == 0){

			send(clientSocket,input,1024,0);

		}
		if(strcmp(input,"CONNECT") == 0){

			send(clientSocket,input,1024,0);
			
			scanf("%s",input);
			send(clientSocket,input,1024,0);

		}
		if (strcmp(input, "SEND") == 0) {
			while (1)
			{
				char message[1024];
				send(clientSocket, input, 1024, 0);
				printf("%s: ", userName);
				scanf("\n%s", message);
				send(clientSocket, message, 1024, 0);
			}
			
		}
		if(strcmp(input,"NAME") == 0){
			
			send(clientSocket,input,1024,0);
			
			scanf("%s",input);
			sprintf(userName, "%s", input);
			send(clientSocket,input,1024,0);

		}

	}


}