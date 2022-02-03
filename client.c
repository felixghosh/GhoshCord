#include "common.h"
#include <strings.h>

int main(int argc, char **argv){
    int sockfd, n, sendbytes;
    struct sockaddr_in servaddr;
    char sendbuff[MAXLINE];
    char recvbuff[MAXLINE];

    if (argc != 2)
        err_n_die("usage: %s <server address>", argv[0]);

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_die("socket error.");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_n_die("inet_pton error.");
    
    if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
        err_n_die("connect error.");
    
    sprintf(sendbuff, "Hello! I am a client!\n");
    sendbytes = strlen(sendbuff);

    if(write(sockfd, sendbuff, sendbytes) != sendbytes)
        err_n_die("write error.");

    memset(recvbuff, 0, MAXLINE);

    while((n = read(sockfd, recvbuff, MAXLINE-1)) > 0){
        printf("%s", recvbuff);
        if(recvbuff[n-1] == '\n')   //check for end of message
                break;
        memset(recvbuff, 0, MAXLINE);

    }

    if(n < 0)
        err_n_die("read error.");

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