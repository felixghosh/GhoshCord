#include "common.h"
#include <strings.h>
#include <pthread.h>

typedef struct connection_t connection_t;
typedef struct sockets_t sockets_t;

void* handle_connection(void* connfd);

void* accept_connections(void* sockets);

void broadcast_message(char* message, char* username);

struct connection_t{
    int connfd;
    char* username;
};

struct sockets_t{
    int connfd;
    int listenfd;
};

sockets_t* new_sockets(int connfd, int listenfd){
    sockets_t* sockets = malloc(sizeof(sockets_t));
    sockets->connfd = connfd;
    sockets->listenfd = listenfd;
    return sockets;
}

connection_t* new_connection(int connfd, char* username){
    connection_t* connection = malloc(sizeof(connection_t));
    connection->connfd = connfd;
    connection->username = username;
    return connection;
}

connection_t* connections[100];
int nbr_connections =0;

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

    sockets_t *p_sockets = new_sockets(connfd, listenfd);
    pthread_t ac;
    pthread_create(&ac, NULL, accept_connections, p_sockets);
    

    int finished = 0;
    char query[20];
    while(!finished){
        memset(query, 0, 20);
        scanf("%19s", query);
        if(strcmp(query, "quit") == 0){
            printf("\nServer shutting down!\n");
            broadcast_message("\n#####################\nSERVER SHUTTING DOWN!\n#####################\n", "");
            finished = 1;
            break;
        }
    }
    return 0;
}

void* accept_connections(void* p_sockets){
    int connfd = ((sockets_t*)p_sockets)->connfd;
    int listenfd = ((sockets_t*)p_sockets)->listenfd;
    free(p_sockets);

    int finished = 0;
    while(!finished){
        struct sockaddr_in client_addr;
        socklen_t addr_len;

        printf("wating for next connection on port %d\n", SERVER_PORT);
        fflush(stdout);
        //this will block until a connetion is established
        connfd = accept(listenfd, (SA *) &client_addr, &addr_len);

        char str[40];
        inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str));
        printf("Incomming connection! Address: %s\n", str);

        int *pclient = malloc(sizeof(int));
        *pclient = connfd;
        pthread_t t;
        pthread_create(&t, NULL, handle_connection, pclient);
    }
}

void* handle_connection(void* p_connfd){
    int connfd = *((int*)p_connfd);
    free(p_connfd);
    char sendbuff[MAXLINE+1];
    char recvbuff[MAXLINE+1];
    char username[MAXLINE+1];       
    int n;
    memset(recvbuff, 0, MAXLINE);      
    memset(username, 0, MAXLINE);
    
    
    while((n = read(connfd, recvbuff, MAXLINE-1)) > 0){     //read username
        if(recvbuff[n-1] == '\n')                           //check for end of message
            break;
        memset(recvbuff, 0, MAXLINE);
    }
    if(n < 0)
        err_n_die("read error.");


    recvbuff[n-1] = '\0';           //remove newline from username
    strcpy(username, recvbuff);
    printf("Client connected! Username: %s\n", username);
    
    connections[nbr_connections] = new_connection(connfd, username);    //add connection to list
    nbr_connections++;

    snprintf(sendbuff, sizeof(sendbuff), "%s has joined the server!\n", username);
    broadcast_message(sendbuff, username);
    memset(sendbuff, 0, MAXLINE);

    snprintf(sendbuff, sizeof(sendbuff), "Connected! Welcome to the server %s:)\n", username);
    write(connfd, sendbuff, strlen(sendbuff));
    

    int finished = 0;
    while(!finished){
        memset(recvbuff, 0, MAXLINE);
        while((n = read(connfd, recvbuff, MAXLINE-1)) > 0){
            fprintf(stdout, "\n%s: %s", username, recvbuff);

            if(recvbuff[n-1] == '\n')   //check for end of message
                break;
            memset(recvbuff, 0, MAXLINE);
        }
        if(n < 0)
            err_n_die("read error.");

        if(n == 0){                     //Detect user disconnecting
            snprintf(sendbuff, sizeof(sendbuff), "%s has left the server!\n", username);
            broadcast_message(sendbuff, username);
            break;
        }
        char message[MAXLINE+3+strlen(username)];
        strcpy(message, username);
        strcat(message, ": ");
        strcat(message, recvbuff);
        broadcast_message(message, username);
    }
    close(connfd);
    pthread_exit(NULL);
}

void broadcast_message(char* message, char* username){
    for(int i = 0; i < nbr_connections; i++){
        if(strcmp(connections[i]->username, username) != 0){
            write(connections[i]->connfd, message, strlen(message));
        }
    }
}