#include "common.h"
#include <strings.h>

int main(int argc, char **argv){
    int listenfd, connfd, n;
    struct sockaddr_in servaddr;
    uint8_t buff[MAXLINE+1];        //sen buffer
    uint8_t recvline[MAXLINE+1];    //receive buffer
    
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
        struct sockaddr_in addr;
        socklen_t addr_len;

        printf("wating for a connection on port %d\n", SERVER_PORT);
        fflush(stdout);
        //this will block until a connetion is established
        connfd = accept(listenfd, (SA *) NULL, NULL);

        memset(recvline, 0, MAXLINE);       //zero the buffer to make sure the string we receive is null terminated

        while((n = read(connfd, recvline, MAXLINE-1)) > 0){
            fprintf(stdout, "\n%s\n\n%s", bin2hex(recvline, n), recvline);

            if(recvline[n-1] == '\n')   //check for end of message
                break;
            memset(recvline, 0, MAXLINE);
        }

        if(n < 0)
            err_n_die("read error.");

        //put response in buffer
        snprintf((char*)buff, sizeof(buff), "HTTP/1.0 200 OK\r\n\r\nHello");
        //write the response using the connection socket and then close it
        write(connfd, (char*)buff, strlen((char*)buff));
        close(connfd);
    }


    return 0;
}