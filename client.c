#include "common.h"
#include <strings.h>
#include <pthread.h>
#include <locale.h>
#include <ncurses.h>

//Struct type-definition
typedef struct thread_params_t thread_params_t;

//Function prototypes
void* listen_to_server(void* p_sockfd);
void err_n_exit_win(const char *fmt, ...);
void print_to_chat(char*** messageHistory, char message[], WINDOW* chat2, int*row, int chatWidth, int chatHeight2, int i, int* x);

//Struct declaration
struct thread_params_t{
    char*** messageHistory;
    int* row;
    int chatWidth;
    int chatHeight2;
    int* x;
    WINDOW *chat2;
    int sockfd;
};

//Struct constructor
thread_params_t* new_params(char*** messageHistory, int* row, int chatWidth, int chatHeight2, int* x, WINDOW *chat2, int sockfd){
    thread_params_t* params = malloc(sizeof(thread_params_t));
    params->messageHistory = messageHistory;
    params->row = row;
    params->chatWidth = chatWidth;
    params->chatHeight2 = chatHeight2;
    params->x = x;
    params->chat2 = chat2;
    params->sockfd = sockfd;
    return params;

}

//Global variables
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;   //Lock for accessing shared data
WINDOW *chat2;                                              //Inner chat window
WINDOW *input2;                                             //Inner input window
int finished;                                               //Bolean for terminating program

int main(int argc, char **argv){
    //Declare data used by sockets
    int sockfd, n, sendbytes;
    struct sockaddr_in servaddr;
    char sendbuff[MAXLINE];
    char recvbuff[MAXLINE];

    //Check for proper usage of args
    if (argc != 3){
        err_n_die("usage: %s <server address> <username>", argv[0]);
    }

    //Initialize ncurses
    setlocale(LC_CTYPE, "");    //Set locale for utf8
    initscr();                  //Initialize screen
    cbreak();                   //Allow exit with ctrl+c
    noecho();                   //don't print all user input
    start_color();              //start colors

    //Init data used by ncurses
    int yMax, xMax, borderTop, borderSide, chatHeight, chatWidth, chatHeight2, inputHeight, inputHeight2;
    borderTop = 1;
    borderSide = 2;
    getmaxyx(stdscr, yMax, xMax);
    chatHeight = (int)(yMax*0.75);
    chatWidth = xMax - 2*borderSide;
    chatHeight2 = chatHeight-2;
    inputHeight = yMax - chatHeight - borderTop*2;
    inputHeight2 = inputHeight - 2;

    char** messageHistory = calloc(chatHeight2, sizeof(char*));
    for(int a = 0; a < chatHeight2; a++)
        messageHistory[a] = calloc(1024, sizeof(char));
    char message[1024];
    int c, i, row, x, y;
    row = x = y = 0;

    //Init and draw GUI
    WINDOW *chat = newwin(chatHeight, chatWidth,borderTop,borderSide);                              //Border for char
    chat2 = newwin(chatHeight-2, chatWidth-2, borderTop+1,borderSide+1);                    //Internal chat box
    WINDOW *input = newwin(inputHeight, chatWidth, chatHeight + borderTop, borderSide);             //Border for input window
    input2 = newwin(inputHeight2, chatWidth-2, chatHeight + borderTop + 1, borderSide + 1); //Internal input window
    box(chat,0,0);
    box(input, 0,0);
    refresh();
    wrefresh(chat);
    wrefresh(input);
    keypad(input2, 1);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, xMax/2 - 8, "GhoshCord 1.0.1!");
    attroff(COLOR_PAIR(1) | A_BOLD);
    refresh();

    
        
    char* username = argv[2];

    //Initialize socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_exit_win("socket error.");

    //Set properties of server address
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    //Convert address from standard presentation from to binary form
    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_n_exit_win("inet_pton error.");
    
    //Connect to server address
    if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
        err_n_exit_win("connect error.");
    
    //Write username into sendbuffer and send to server
    sprintf(sendbuff, "%s\n", username);    
    sendbytes = strlen(sendbuff);
    if(write(sockfd, sendbuff, sendbytes) != sendbytes)
        err_n_exit_win("write error.");

    memset(recvbuff, 0, MAXLINE);

    //read welcome message
    while((n = read(sockfd, recvbuff, MAXLINE-1)) > 0){ 
        char errCheck[6] = "";
        for(int i = 0; i < 5; i++)
            errCheck[i] = recvbuff[i];
        printf("%s", errCheck);
        refresh();
        if(strcmp(errCheck, "Error") == 0)
            err_n_exit_win(recvbuff);
        for(unsigned int j = 0; j < strlen(recvbuff); j++)
            messageHistory[row + j/(chatWidth-2)][j%(chatWidth-2)] = recvbuff[j];
        for(int j = 0; j < chatHeight2; j++)
            mvwprintw(chat2, j, 0, "%s\n", messageHistory[j]);
        row += x/(chatWidth-1)+1;
        wrefresh(chat2);

        if(recvbuff[n-1] == '\n')   //check for end of message
                break;
        memset(recvbuff, 0, MAXLINE);

    }
    memset(recvbuff, 0, MAXLINE);
    if(n < 0)
        err_n_exit_win("read error.");

    //Initialize parameter pointer, create and start listen_to_server thread
    thread_params_t* p_params = new_params(&messageHistory, &row, chatWidth, chatHeight2, &x, chat2, sockfd);
    pthread_t t;
    pthread_create(&t, NULL, listen_to_server, p_params);

    finished = 0;   //Boolean for terminating main loop

    //Main loop, takes user input and writes it to server
    while(!finished){
        memset(sendbuff, 0, MAXLINE);
        memset(message, 0, 1024);
        i = 0;
        //Move cursor to top left of input window
        pthread_mutex_lock(&mutex);
        wmove(input2, y,x);
        pthread_mutex_unlock(&mutex);
        //Check input for arrow keys or backspace, otherwise just echo out the input character to the input window
        while((c = wgetch(input2)) != '\n' && !finished && x < 1024){
            pthread_mutex_lock(&mutex);
            switch (c)
            {
            case KEY_LEFT:
                if(x > 0)
                    wmove(input2, y, --x);
                break;

            case KEY_RIGHT:
                if(x < chatWidth-3)
                    wmove(input2, y, ++x);
                break;

            case KEY_DOWN:
                if(y < inputHeight2-1)
                    wmove(input2, ++y, x);
                break;
            case KEY_UP:
                if(y > 0)
                    wmove(input2, --y, x);
                break;
            case KEY_BACKSPACE:   //backspace
                if(x == 0)
                    break;
                for(int j = --x; j < i; j++){
                    message[j] = message[j+1];
                    i--;
                }
 
                werase(input2);
                mvwprintw(input2, y, 0, "%s", message);
                wmove(input2, y,x);
                break;
            
            default:
                message[i++] = c;
                x++;

                //Check if second byte of utf-8 char, then back cursor up one space
                if((c & 0xC0) == 0x80){
                    x--;
                }

                //Only print char if this is not first byte of utf8-char
                if((c & 0xC0) != 0xC0){
                    mvwprintw(input2, y, 0, "%s", message);
                    wmove(input2, y, x);
                    wrefresh(input2);
                }
            }
            pthread_mutex_unlock(&mutex);
            
        }

        if(strcmp(message, "quit") == 0){
            finished = 1;
            break;
        }
        //Print user message to chat
        print_to_chat(&messageHistory, message, chat2, &row, chatWidth, chatHeight2, i, &x);

        werase(input2);
        wrefresh(input2);
        pthread_mutex_lock(&mutex);
        x = y = 0;
        pthread_mutex_unlock(&mutex);

        //Append newline to end of message and send to server
        char temp[2] ="0";
        temp[0] = '\n';
        strcat(message, temp);
        sendbytes = strlen(message);
        if(write(sockfd, message, sendbytes) != sendbytes)
            err_n_exit_win("write error.");
    }

    //Free memmory and terminate ncurses
    for(int a = 0; a < chatHeight2; a++)
        free(messageHistory[a]);
    free(messageHistory);
    pthread_cancel(t);
    pthread_join(t, NULL);
    delwin(stdscr);
    endwin();
    return 0;
}

void* listen_to_server(void* p_params){
    thread_params_t* params = (thread_params_t *) p_params;
    int sockfd = params->sockfd;
    char*** p_messageHistory = params->messageHistory;
    char** messageHistory = *p_messageHistory;
    int* row = params->row;
    int chatWidth = params->chatWidth;
    int chatHeight2 = params->chatHeight2;
    int* x = params->x;
    WINDOW *chat2 = params->chat2;
    free(p_params);


    char recvbuff[MAXLINE];
    int n;
    memset(recvbuff, 0, MAXLINE); 

    finished = 0;   //Boolean to terminate main loop

    //Main loop, listens for messages from server and prints them to the chat
    while(!finished){
        memset(recvbuff, 0, 1024);
        int i = 0;
        while((n = read(sockfd, recvbuff, MAXLINE-1)) > 0){ 
            i += n;
            if(recvbuff[n-1] == '\n')   //check for end of message
                break;
        }

        //Check for server termination
        if(strcmp(recvbuff, "") == 0){
            wprintw(chat2, "The server has shut down. Killing connection!");
            wrefresh(chat2);
            finished = 1;
        }
        print_to_chat(&messageHistory, recvbuff, chat2, row, chatWidth, chatHeight2, i, x);
    }
    pthread_exit(NULL);
}

//Prints error message and terminates program. Needed local version to terminate ncurses
void err_n_exit_win(const char *fmt, ...){
    int errno_save;

    errno_save = errno; //any system or library call can set errno, so we need to save it here
    wprintw(chat2, fmt);
    wrefresh(chat2);

    //print out error message if erno was set
    if(errno_save) {
        fprintf(stdout, "(errono = %d) : %s\n", errno_save, strerror(errno_save));
        fprintf(stdout, "\n");
        fflush(stdout);
    }

    getch();
    endwin();
    exit(1);
}

//Takes a message string and breaks it up into several strings to be put on different rows in messageHistory, then prints the history
void print_to_chat(char*** p_messageHistory, char message[], WINDOW* chat2, int*row, int chatWidth, int chatHeight2, int i, int* x){
    char** messageHistory = *p_messageHistory;
    pthread_mutex_lock(&mutex);
    //If we still haven't filled the screen we can just print the message history
        if(*row < chatHeight2){
            //Split the message into as many rows as needed in messageHistory
            for(int j = 0; j < i; j++)
                messageHistory[*row + j/(chatWidth-2)][j%(chatWidth-2)] = message[j];
            //Print the messageHistory
            for(int j = 0; j < chatHeight2; j++)
                mvwprintw(chat2, j, 0, "%s\n", messageHistory[j]);
            //Update value of row
            *row += i/(chatWidth-1)+1;

        }
        //Otherwise we need to shift out old messages and fill in the new one(s)
        else{
            int messageRowLength = i/(chatWidth-1)+1;
            for(int j = 0; j < messageRowLength; j++){
                for(int k = 0; k < chatHeight2-1; k++){
                    strcpy(messageHistory[k], messageHistory[k+1]);
                    mvwprintw(chat2, k, 0, "%s\n", messageHistory[k]);
                }
                memset(messageHistory[*row-1], 0 , 1024);
            }
            
            for(int k = 0; k < i; k++)
                messageHistory[*row-1 - messageRowLength+1 + k/(chatWidth-2)][k%(chatWidth-2)] = message[k];
            
            for(int k = *row-1 - messageRowLength; k <= *row-1; k++)
                mvwprintw(chat2, k, 0, "%s\n", messageHistory[k]);
        }
        wrefresh(chat2);
        wmove(input2, 0, *x);
        wrefresh(input2);
        pthread_mutex_unlock(&mutex);
}