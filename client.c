#include "common.h"
#include <strings.h>
#include <pthread.h>

void* listen_to_server(void* p_sockfd);

int main(int argc, char **argv){
    int sockfd, n, sendbytes;
    struct sockaddr_in servaddr;
    char sendbuff[MAXLINE];
    char recvbuff[MAXLINE];

    if (argc != 3)
        err_n_die("usage: %s <server address> <username>", argv[0]);
    char* username = argv[2];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("socket error.");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_n_die("inet_pton error.");
    
    if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
        err_n_die("connect error.");
    
    sprintf(sendbuff, "%s\n", username);    
    sendbytes = strlen(sendbuff);

    if(write(sockfd, sendbuff, sendbytes) != sendbytes) //send username to server
        err_n_die("write error.");

    memset(recvbuff, 0, MAXLINE);

    while((n = read(sockfd, recvbuff, MAXLINE-1)) > 0){ //read wlcome message
        printf("%s", recvbuff);
        if(recvbuff[n-1] == '\n')   //check for end of message
                break;
        memset(recvbuff, 0, MAXLINE);

    }

    memset(recvbuff, 0, MAXLINE);

    if(n < 0)
        err_n_die("read error.");

    int *p_sockfd = malloc(sizeof(int));
    *p_sockfd = sockfd;
    pthread_t t;
    pthread_create(&t, NULL, listen_to_server, p_sockfd);

    int finished = 0;
    while(!finished){
        memset(sendbuff, 0, MAXLINE);

        if (fgets (sendbuff, MAXLINE, stdin) == NULL)
            err_n_die("no input.");
            
        sendbytes = strlen(sendbuff);
        if(write(sockfd, sendbuff, sendbytes) != sendbytes)
            err_n_die("write error.");
    }
    

    return 0;
}

void* listen_to_server(void* p_sockfd){
    int sockfd = *((int*)p_sockfd);
    free(p_sockfd);
    char recvbuff[MAXLINE];
    int n;
    memset(recvbuff, 0, MAXLINE); 

    int finished = 0;
    while(!finished){
        memset(recvbuff, 0, MAXLINE);
        while((n = read(sockfd, recvbuff, MAXLINE-1)) > 0){
            printf("%s", recvbuff);
            if(recvbuff[n-1] == '\n')
                break;
            memset(recvbuff, 0, MAXLINE);
        }
        if(n < 0)
                err_n_die("read error.");
    }
    pthread_exit(NULL);
}