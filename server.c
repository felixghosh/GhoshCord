#include "common.h"
#include <strings.h>
#include <pthread.h>

void* handle_connection(void* connfd);

int main(int argc, char **argv){
    int listenfd, connfd;
    struct sockaddr_in servaddr;
    
    
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) //init socket
        err_n_die("socket error.");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVER_PORT);

    if((bind(listenfd, (SA *)  &servaddr, sizeof(servaddr))) < 0)
        err_n_die("bind error.");

    if((listen(listenfd, 10)) < 0)
        err_n_die("listen error.");

    int finished = 0;
    while(!finished){
        struct sockaddr_in client_addr;
        socklen_t addr_len;

        printf("wating for a connection on port %d\n", SERVER_PORT);
        fflush(stdout);
        //this will block until a connetion is established
        connfd = accept(listenfd, (SA *) &client_addr, &addr_len);

        char str[40];
        inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str));
        printf("Client connected! Address: %s\n", str);

        int *pclient = malloc(sizeof(int));
        *pclient = connfd;
        pthread_t t;
        pthread_create(&t, NULL, handle_connection, pclient);
    }
    return 0;
}

void* handle_connection(void* p_connfd){
    int connfd = *((int*)p_connfd);
    free(p_connfd);
    char sendbuff[MAXLINE+1];        //send buffer
    char recvbuff[MAXLINE+1];    //receive buffer
    int n;
    memset(recvbuff, 0, MAXLINE);       //zero the buffer to make sure the string we receive is null terminated

    while((n = read(connfd, recvbuff, MAXLINE-1)) > 0){
        fprintf(stdout, "\n%s\n\n%s", bin2hex(recvbuff, n), recvbuff);

        if(recvbuff[n-1] == '\n')   //check for end of message
            break;
        memset(recvbuff, 0, MAXLINE);
    }

    if(n < 0)
        err_n_die("read error.");

    //put response in buffer
    snprintf(sendbuff, sizeof(sendbuff), "Hello, I am a server!\n");
    //write the response using the connection socket and then close it
    write(connfd, sendbuff, strlen(sendbuff));

    int finished = 0;
    while(!finished){
        memset(recvbuff, 0, MAXLINE);
        while((n = read(connfd, recvbuff, MAXLINE-1)) > 0){
            fprintf(stdout, "\n%s", recvbuff);

            if(recvbuff[n-1] == '\n')   //check for end of message
                break;
            memset(recvbuff, 0, MAXLINE);
        }
        if(n < 0)
            err_n_die("read error.");
    }
    close(connfd);
    return NULL;
}

