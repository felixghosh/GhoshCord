#include "common.h"
#include <strings.h>
#include <pthread.h>


//Struct type-definitions
typedef struct connection_t connection_t;
typedef struct sockets_t sockets_t;

//Function prototypes
void* handle_connection(void* connfd);
void* accept_connections(void* sockets);
void broadcast_message(char* message, char* username);

//Stuct declarations
struct connection_t{        //Struct for a connection
    int connfd;             //Socket file descriptor
    char* username;         //Connection username
};

struct sockets_t{           //Struct for parameters of accept_connections thread
    int connfd;             //Connection socket file descriptor
    int listenfd;           //Listening socket file descriptor
};

//Struct constructors
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

//Global variables
connection_t* connections[100];                             //Array of connections
int nbr_connections = 0;                                    //Number of connected users
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;   //Lock for accessing shared data


int main(int argc, char **argv){
    int listenfd, connfd;           //Listening socket & connection socket
    struct sockaddr_in servaddr;    //Server address
    
    
    //Initialize listening socket
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        err_n_die("socket error.");

    //Set properties of server address
    memset(&servaddr, 0, sizeof(servaddr));         //zero it out
    servaddr.sin_family = AF_INET;                  //set address familiy to ip
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);   //set it to listen to any address
    servaddr.sin_port = htons(SERVER_PORT);         //set server port to 18000

    //Bind server address to socket and start listening
    if((bind(listenfd, (SA *)  &servaddr, sizeof(servaddr))) < 0)
        err_n_die("bind error.");
    if((listen(listenfd, 10)) < 0)
        err_n_die("listen error.");

    //Initialize parameter struct, declare accept_connections thread, and create the thread
    sockets_t *p_sockets = new_sockets(connfd, listenfd);
    pthread_t ac;
    pthread_create(&ac, NULL, accept_connections, p_sockets);
    

    int finished = 0;   //Boolean for terminating main loop
    char query[20];     //String for server admin inputs

    //Main loop, reads server admin commands
    while(!finished){
        memset(query, 0, 20);                       //Zero out query
        scanf("%19s", query);                       //Read user input
        if(strcmp(query, "quit") == 0){             //Only command as of now is shutting down the server
            pthread_mutex_lock(&mutex);
            printf("\nServer shutting down!\n");
            pthread_mutex_unlock(&mutex);
            broadcast_message("\n#####################\nSERVER SHUTTING DOWN!\n#####################\n", "");
            finished = 1;
        }
    }
    return 0;   //Successful termination
}

//This function runs in a separate thread from main and listens for any new connecting clients
void* accept_connections(void* p_sockets){
    //Grab parameters from param-struct and free the pointer
    int connfd = ((sockets_t*)p_sockets)->connfd;
    int listenfd = ((sockets_t*)p_sockets)->listenfd;
    free(p_sockets);

    int finished = 0;   //Boolean for terminating main loop

    //Main loop, listens for new connetions
    while(!finished){
        struct sockaddr_in client_addr;
        socklen_t addr_len;

        pthread_mutex_lock(&mutex);
        printf("wating for next connection on port %d\n\n", SERVER_PORT);
        fflush(stdout);
        pthread_mutex_unlock(&mutex);

        //This will block until a connetion is established
        connfd = accept(listenfd, (SA *) &client_addr, &addr_len);

        //Grab and print the client address
        char str[40];
        inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str));
        pthread_mutex_lock(&mutex);
        printf("Incomming connection! Address: %s\n\n", str);
        pthread_mutex_unlock(&mutex);
        
        //Initialize parameter pointer, declare handle_connection thread, and create the thread
        int *pclient = malloc(sizeof(int));
        *pclient = connfd;
        pthread_t t;
        pthread_create(&t, NULL, handle_connection, pclient);
    }
}

//This functions is run by several threads, one for each connection.
//It listens to the client for messages and broadcasts them to the other connected clients.
void* handle_connection(void* p_connfd){
    //Grab the parameter and free the pointer
    int connfd = *((int*)p_connfd);
    free(p_connfd);

    char sendbuff[MAXLINE+1];
    char recvbuff[MAXLINE+1];
    char username[MAXLINE+1];       
    int n;
    memset(recvbuff, 0, MAXLINE);      
    memset(username, 0, MAXLINE);
    
    //Read username
    while((n = read(connfd, recvbuff, MAXLINE-1)) > 0){     
        if(recvbuff[n-1] == '\n')       //Check for end of message
            break;
        memset(recvbuff, 0, MAXLINE);
    }
    if(n < 0)
        err_n_die("read error.");


    recvbuff[n-1] = '\0';           //Remove newline from username
    strcpy(username, recvbuff);     //Put the string in the username buffer
    if(strcmp(username, "") == 0){  //Check for empty username
        pthread_mutex_lock(&mutex);
        printf("Username NULL is not allowed! Killing connection!\n");
        pthread_mutex_unlock(&mutex);
        close(connfd);
        pthread_exit(NULL);
    }
    pthread_mutex_lock(&mutex);
    printf("Client connected! Username: %s\n", username);
    pthread_mutex_unlock(&mutex);
    
    //Add connection to list
    pthread_mutex_lock(&mutex);
    connections[nbr_connections] = new_connection(connfd, username);    
    nbr_connections++;
    pthread_mutex_unlock(&mutex);

    //Broadcast that user has joined
    snprintf(sendbuff, sizeof(sendbuff), "%s has joined the server!\n", username);
    broadcast_message(sendbuff, username);
    memset(sendbuff, 0, MAXLINE);

    //Send welcome message to user
    snprintf(sendbuff, sizeof(sendbuff), "Connected! Welcome to the server %s:)\n", username);
    write(connfd, sendbuff, strlen(sendbuff));
    

    int finished = 0;   //Boolean for terminating main loop

    //Main loop, reads messages from client and broadcasts them
    while(!finished){
        //Read user message
        memset(recvbuff, 0, MAXLINE);
        while((n = read(connfd, recvbuff, MAXLINE-1)) > 0){
            pthread_mutex_lock(&mutex);
            fprintf(stdout, "\n%s: %s", username, recvbuff);
            pthread_mutex_unlock(&mutex);

            if(recvbuff[n-1] == '\n')   //check for end of message
                break;
            memset(recvbuff, 0, MAXLINE);
        }
        if(n < 0)
            err_n_die("read error.");
        
        //Detect user disconnecting and broadcast this
        if(n == 0){                     
            snprintf(sendbuff, sizeof(sendbuff), "%s has left the server!\n", username);
            broadcast_message(sendbuff, username);
            int i;
            //Find index of user
            pthread_mutex_lock(&mutex);
            for(i = 0; i < nbr_connections; i++){
                if(strcmp(connections[i]->username, username) == 0)
                    break;
            }
            //Free the pointer, remove user from list of connections, and shift the array to fill the hole
            free(connections[i]);
            for(int j = i; j < nbr_connections-1; j++)
                connections[j] = connections[j+1];
            nbr_connections--;
            pthread_mutex_unlock(&mutex);
            break;
        }
        //Prepend username to message string and broadcast to users
        char message[MAXLINE+3+strlen(username)];
        strcpy(message, username);
        strcat(message, ": ");
        strcat(message, recvbuff);
        broadcast_message(message, username);
    }
    close(connfd);
    pthread_exit(NULL);
}

//Function for broadcasting message to every user except for username
void broadcast_message(char* message, char* username){
    pthread_mutex_lock(&mutex);
    for(int i = 0; i < nbr_connections; i++){
        if(strcmp(connections[i]->username, username) != 0){
            write(connections[i]->connfd, message, strlen(message));
        }
    }
    pthread_mutex_unlock(&mutex);
}