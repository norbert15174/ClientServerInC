#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/utsname.h>
#include <linux/un.h>
#include <net/if.h>
#include <fcntl.h>
#include <pthread.h>

// TCP
#define servAddr "192.168.56.25"
// MULTICAST
#define MAXLINE 1024
#define SA struct sockaddr

#define INTERFACE "eth1"
#define PORT 6000
#define MCAST_ADDRESS "230.0.0.1"

// TCP
void *doRecieving(void *sockID)
{

    int clientSocket = *((int *)sockID);

    while (1)
    {

        char data[1024];
        int read = recv(clientSocket, data, 1024, 0);
        data[read] = '\0';
        printf("%s\n", data);
    }
}

// MULTICAST

unsigned int _if_nametoindex(const char *ifname)
{
    int s;
    struct ifreq ifr;
    unsigned int ni;
    s = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (s != -1)
    {

        memset(&ifr, 0, sizeof(ifr));
        strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

        if (ioctl(s, SIOCGIFINDEX, &ifr) != -1)
        {
            close(s);
            return (ifr.ifr_ifindex);
        }
        close(s);
        return -1;
    }
}

int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp)
{
    int sockfd, n;
    struct addrinfo hints, *res, *ressave;
    struct sockaddr_in *pservaddrv4;
    *saptr = malloc(sizeof(struct sockaddr_in));
    pservaddrv4 = (struct sockaddr_in *)*saptr;
    bzero(pservaddrv4, sizeof(struct sockaddr_in));
    if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0)
    {
        fprintf(stderr, "AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
        return -1;
    }
    else
    {
        pservaddrv4->sin_family = AF_INET;
        pservaddrv4->sin_port = htons(port);
        *lenp = sizeof(struct sockaddr_in);
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            fprintf(stderr, "AF_INET socket error : %s\n", strerror(errno));
            return -1;
        }
    }
    return (sockfd);
}

int mcast_join(int sockfd, const SA *grp, socklen_t grplen,
               const char *ifname, u_int ifindex)
{
    struct group_req req;
    if (ifname != NULL)
    {
        if ((req.gr_interface = if_nametoindex(ifname)) == 0)
        {
            errno = ENXIO;
            return (-1);
        }
    }
    if (grplen > sizeof(req.gr_group))
    {
        errno = EINVAL;
        return -1;
    }
    memcpy(&req.gr_group, grp, grplen);
    return (setsockopt(sockfd, IPPROTO_IP,
                       MCAST_JOIN_GROUP, &req, sizeof(req)));
}

void recv_all(int, socklen_t);
void send_all(int, SA *, socklen_t);

void send_all(int sendfd, SA *sadest, socklen_t salen)
{
    char line[MAXLINE];
    char text[MAXLINE];
    int a;
    for (;;)
    {
        a = scanf("%[^\n]%*c", text);
        if ( a == 0)  // gdy brak tekstu     
        {
            getchar();
            continue;
        }
        snprintf(line, sizeof(line), "%s", text);
        if (sendto(sendfd, line, strlen(line), 0, sadest, salen) < 0)
            fprintf(stderr, "sendto() error : %s\n", strerror(errno));
    }
}

void recv_all(int recvfd, socklen_t salen)
{
    int n;
    char line[MAXLINE + 1];
    socklen_t len;
    struct sockaddr *safrom;
    char str[128];
    struct sockaddr_in *cliaddrv4;
    char addr_str[INET_ADDRSTRLEN + 1];

    safrom = malloc(salen);

    for (;;)
    {
        len = salen;
        if ((n = recvfrom(recvfd, line, MAXLINE, 0, safrom, &len)) < 0)
            perror("recvfrom() error");

        line[n] = 0;
        cliaddrv4 = (struct sockaddr_in *)safrom;
        inet_ntop(AF_INET, (struct sockaddr *)&cliaddrv4->sin_addr, addr_str, sizeof(addr_str));
        printf("%s \n", line);
        fflush(stdout);
    }
}

int main()
{

    while (1)
    {
        printf("Make a choice...\nPRIVATE or GROUP chat\n");
        char CHOICE[50];
        scanf("%s", CHOICE);
        getchar();
        if (strcmp(CHOICE, "PRIVATE") == 0)
        {

            // gniazdo klienta
            int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

            struct sockaddr_in serverAddr;

            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8080); // port serwera proxy
            serverAddr.sin_addr.s_addr = inet_addr(servAddr);

            if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
                return 0;

            printf("Connection established ............\n");

            pthread_t thread;
            pthread_create(&thread, NULL, doRecieving, (void *)&clientSocket);
            char userName[1024] = "NAME"; // nazwa użytkownika
            send(clientSocket, userName, 1024, 0);
            printf("Give me your name!!!\n");
            scanf("%s", userName);
            sprintf(userName, "%s", userName);
            send(clientSocket, userName, 1024, 0);

            while (1)
            {

                char input[1024];
                scanf("%s", input);

                if (strcmp(input, "LIST") == 0)
                {

                    send(clientSocket, input, 1024, 0);
                }
                if (strcmp(input, "CONNECT") == 0)
                {

                    send(clientSocket, input, 1024, 0);

                    scanf("%s", input);
                    send(clientSocket, input, 1024, 0);
                }
                if (strcmp(input, "SEND") == 0)
                {
                    while (1)
                    {
                        char message[1024];
                        send(clientSocket, input, 1024, 0);
                        printf("%s: ", userName);
                        scanf("\n%s", message);
                        send(clientSocket, message, 1024, 0);
                    }
                }
                if (strcmp(input, "NAME") == 0)
                {

                    send(clientSocket, input, 1024, 0);

                    scanf("%s", input);
                    sprintf(userName, "%s", input);
                    send(clientSocket, input, 1024, 0);
                }
            }
        }
        else if (strcmp(CHOICE, "GROUP") == 0)
        {
            int sendfd, recvfd;
            const int on = 1;
            socklen_t salen;
            struct sockaddr *sasend, *sarecv;
            struct sockaddr_in *ipv4addr;
            sendfd = snd_udp_socket(MCAST_ADDRESS, PORT, &sasend, &salen);
            if ((recvfd = socket(sasend->sa_family, SOCK_DGRAM, 0)) < 0)
            {
                fprintf(stderr, "socket error : %s\n", strerror(errno));
                return 1;
            }
            if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
            {
                fprintf(stderr, "setsockopt error : %s\n", strerror(errno));
                return 1;
            }
            sarecv = malloc(salen);
            memcpy(sarecv, sasend, salen);

            setsockopt(sendfd, SOL_SOCKET, SO_BINDTODEVICE, INTERFACE, strlen(INTERFACE));

            int32_t ifindex;
            ifindex = if_nametoindex(INTERFACE);

            if (sarecv->sa_family == AF_INET)
            {
                ipv4addr = (struct sockaddr_in *)sarecv;
                ipv4addr->sin_addr.s_addr = htonl(INADDR_ANY);

                struct ifreq ifr;
                ifr.ifr_addr.sa_family = AF_INET;
                struct in_addr localInterface;

                strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ - 1);
                ioctl(sendfd, SIOCGIFADDR, &ifr);
                localInterface.s_addr = inet_addr(inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

                if (setsockopt(sendfd, IPPROTO_IP, IP_MULTICAST_IF,
                               (char *)&localInterface,
                               sizeof(localInterface)) < 0)
                {
                    perror("setting local interface");
                    exit(1);
                }
            }

            if (bind(recvfd, sarecv, salen) < 0)
            {
                fprintf(stderr, "bind error : %s\n", strerror(errno));
                return 1;
            }

            if (mcast_join(recvfd, sasend, salen, INTERFACE, 0) < 0)
            {
                fprintf(stderr, "mcast_join() error : %s\n", strerror(errno));
                return 1;
            }

            if (fork() == 0)
                recv_all(recvfd, salen); /* child -> receives */

            send_all(sendfd, sasend, salen); /* parent -> sends */
        }
        printf("\nWRONG CHOICE!!!\n");
    }
}
