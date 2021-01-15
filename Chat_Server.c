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
#include <syslog.h>
#include <signal.h>
#include <errno.h>

#define servAddr "192.168.56.25" 
#define	MAXFD	64
#define MAXLINE 1024

int clientCount = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;


int daemon_init(const char* pname, int facility, uid_t uid, int socket)
{
	int		i, p;
	pid_t	pid;

	if ((pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			
	if (setsid() < 0)			
		return (-1);

	signal(SIGHUP, SIG_IGN);
	if ((pid = fork()) < 0)
		return (-1);
	else if (pid)
		exit(0);			
	chdir("/tmp");		
	for (i = 0; i < MAXFD; i++) {
		if (socket != i)
			close(i);
	}
	p = open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);
	openlog(pname, LOG_PID, facility);
	setuid(uid); 
	return (0);			
}

struct client{
	char clientName[MAXLINE];
	int index;
	int sockID;
	struct sockaddr_in clientAddr;
	int len;

};

struct client Client[MAXLINE];
pthread_t thread[1024];

void * doNetworking(void * ClientDetail){

	struct client* clientDetail = (struct client*) ClientDetail;
	int index = clientDetail -> index;
	int clientSocket = clientDetail -> sockID;
	int connectedUser = 0;

	syslog(LOG_INFO, "Client %d connected.\n", index + 1);

	while(1){

		char data[MAXLINE];
		int read;
		if ((read = recv(clientSocket, data, MAXLINE, 0)) < 0)
		{
			syslog(LOG_ERR, "Recv doesn't work\n");
		}
		data[read] = '\0';

		char output[MAXLINE];

		if(strcmp(data,"LIST") == 0){
			
			int l = 0;
			int i;
			l += snprintf(output + l, MAXLINE, "ACTIVE USERS (%d): \n", clientCount-1);
			for(i = 0 ; i < clientCount ; i ++){
				if(i != index)
					l += snprintf(output + l, MAXLINE,"Client name is %s\n", Client[i].clientName);
			}
			if ((send(clientSocket, output, MAXLINE, 0)) < 0)
			{
				syslog(LOG_ERR, "Send doesn't work\n");
			}
			continue;
		}
		
		if (strcmp(data, "SEND") == 0)
		{

			char buff[MAXLINE];
			sprintf(buff, "%s: ", Client[index].clientName);
			if ((read = recv(clientSocket, data, MAXLINE, 0)) < 0)
			{
				syslog(LOG_ERR, "Recv doesn't work\n");
			}
			data[read] = '\n';
			strcat(buff, data);
			if ((send(Client[connectedUser].sockID, buff, MAXLINE, 0)) < 0)
			{
				syslog(LOG_ERR, "Send doesn't work\n");
			}
		}

		if(strcmp(data,"CONNECT") == 0){
			
            if ((read = recv(clientSocket,data, MAXLINE,0)) < 0)
			{
				syslog(LOG_ERR, "Recv doesn't work\n");
			}
            data[read] = '\0';
			int i;
            for(i = 0; i<clientCount;i++) if(strcmp(Client[i].clientName,data) == 0) connectedUser = i; 
            sprintf(data, "You have been connected to user: %s\n", Client[connectedUser].clientName);
			if ((send(Client[index].sockID,data, MAXLINE,0)) < 0)
			{
				syslog(LOG_ERR, "Send doesn't work\n");
			}
			sprintf(data, "User %s want to connect\n", Client[index].clientName);
			if ((send(Client[connectedUser].sockID, data, MAXLINE, 0)) < 0)
			{
				syslog(LOG_ERR, "Send doesn't work\n");
			}
        }

		if(strcmp(data,"NAME") == 0){
			if ((read = recv(clientSocket,data, MAXLINE,0)) < 0)
			{
				syslog(LOG_ERR, "Recv doesn't work\n");
			}
			data[read] = '\0';
			strcpy(Client[index].clientName, data);	
			sprintf(data, "Your name has been set to: %s \n", Client[index].clientName);
			if ((send(Client[index].sockID,data, MAXLINE,0)) < 0)
			{
				syslog(LOG_ERR, "Send doesn't work\n");
			}
		}
	}
	return NULL;
}

int main(int argc, char** argv){

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serverAddr;

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(8080);
	serverAddr.sin_addr.s_addr = inet_addr(servAddr);

	int true = 1;
	setsockopt(serverSocket,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int));

	if(bind(serverSocket,(struct sockaddr *) &serverAddr , sizeof(serverAddr)) == -1) return 0;

	if(listen(serverSocket,1024) == -1) return 0;

	daemon_init(argv[0], LOG_USER, 1000, serverSocket);
	syslog(LOG_INFO, "Chat server started listenting on port 8080...");

	while(1){

		if ((Client[clientCount].sockID = accept(serverSocket, (struct sockaddr*) &Client[clientCount].clientAddr, &Client[clientCount].len)) < 0)
		{
			syslog(LOG_ERR, "accept error : %s\n", strerror(errno));
			continue;
		}
		Client[clientCount].index = clientCount;

		pthread_create(&thread[clientCount], NULL, doNetworking, (void *) &Client[clientCount]);

		clientCount ++;
 
	}

	int i;
	for(i = 0 ; i < clientCount ; i ++)
		pthread_join(thread[i],NULL);

	close(serverSocket);
	return 0;
}