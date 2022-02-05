#include "common.h"
#include <strings.h>
#include <pthread.h>
#include <locale.h>
#include <ncurses.h>


void* listen_to_server(void* p_sockfd);
void err_n_exit_win(const char *fmt, ...);

typedef struct thread_params_t thread_params_t;

struct thread_params_t{
    char*** messageHistory;
    int* row;
    int chatWidth;
    int chatHeight2;
    int* x;
    WINDOW *chat2;
    int sockfd;
};

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

int main(int argc, char **argv){
    //Init data used by sockets
    int sockfd, n, sendbytes;
    struct sockaddr_in servaddr;
    char sendbuff[MAXLINE];
    char recvbuff[MAXLINE];

    //Initialize ncurses
    setlocale(LC_CTYPE, "");    //Set locale for utf8
    initscr();                  //Initialize screen
    cbreak();                   //Allow exit with ctrl+c
    noecho();                   //don't print all user input
    start_color();              //start colors

    //Init date used by ncurses
    int yMax, xMax, borderTop, borderSide, chatHeight, chatWidth, chatHeight2, inputHeight, inputHeight2;
    borderTop = 1;
    borderSide = 2;
    getmaxyx(stdscr, yMax, xMax);
    chatHeight = (int)(yMax*0.75);
    chatWidth = xMax - 2*borderSide;
    chatHeight2 = chatHeight-2;
    inputHeight = yMax - chatHeight - borderTop*2;
    inputHeight2 = inputHeight - 2;

    //char messageHistory[chatHeight2][1024];
    char** messageHistory = calloc(chatHeight2, sizeof(char*));
    for(int a = 0; a < chatHeight2; a++)
        messageHistory[a] = calloc(1024, sizeof(char));
    char message[1024];
    unsigned int c, i, row, x, y;
    row = x = y = 0;

    //Init and draw GUI
    WINDOW *chat = newwin(chatHeight, chatWidth,borderTop,borderSide);
    WINDOW *chat2 = newwin(chatHeight-2, chatWidth-2, borderTop+1,borderSide+1);
    WINDOW *input = newwin(inputHeight, chatWidth, chatHeight + borderTop, borderSide);
    WINDOW *input2 = newwin(inputHeight2, chatWidth-2, chatHeight + borderTop + 1, borderSide + 1);
    box(chat,0,0);
    box(input, 0,0);
    refresh();
    wrefresh(chat);
    wrefresh(input);
    keypad(input2, 1);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(0, xMax/2 - 8, "GhoshCord 1.0.0!");
    attroff(COLOR_PAIR(1) | A_BOLD);
    refresh();

    if (argc != 3){
        err_n_exit_win("usage: %s <server address> <username>", argv[0]);
    }
        
    char* username = argv[2];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        err_n_exit_win("socket error.");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    if(inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_n_exit_win("inet_pton error.");
    
    if(connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
        err_n_exit_win("connect error.");
    
    sprintf(sendbuff, "%s\n", username);    
    sendbytes = strlen(sendbuff);

    if(write(sockfd, sendbuff, sendbytes) != sendbytes) //send username to server
        err_n_exit_win("write error.");

    memset(recvbuff, 0, MAXLINE);

    //read welcome message
    while((n = read(sockfd, recvbuff, MAXLINE-1)) > 0){ 
        for(int j = 0; j < strlen(recvbuff); j++)
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

    thread_params_t* p_params = new_params(&messageHistory, &row, chatWidth, chatHeight2, &x, chat2, sockfd);

    //int *p_sockfd = malloc(sizeof(int));
    //*p_sockfd = sockfd;
    pthread_t t;
    pthread_create(&t, NULL, listen_to_server, p_params);

    int finished = 0;
    while(!finished){
        memset(sendbuff, 0, MAXLINE);

        memset(message, 0, 1024);
        i = 0;
        wmove(input2, y,x);
        while((c = wgetch(input2)) != '\n'){
             
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
            case 127:   //backspace
                if(x == 0)
                    break;
                for(int j = --x; j < i; j++){
                    message[j] = message[j+1];
                }
                werase(input2);
                mvwprintw(input2, y, 0, "%s", message);
                wmove(input2, y,x);
                break;
            
            default:
                if(x!=i){
                    for(int j = i+1; j>x; j--)
                        message[j] = message[j-1];
                }
                message[x++] = c;
                
                mvwprintw(input2, y, 0, "%s ", message);
                werase(input2);     //hacky way to get utf8 characters to work in the input box
                wrefresh(input2);
                mvwprintw(input2, y, 0, "%s", message);
                wmove(input2, y, x);
                wrefresh(input2);
                i++;
            }
            
        }

        if(row < chatHeight2){
            for(int j = 0; j < i; j++)
                messageHistory[row + j/(chatWidth-2)][j%(chatWidth-2)] = message[j];
            for(int j = 0; j < chatHeight2; j++)
                mvwprintw(chat2, j, 0, "%s\n", messageHistory[j]);
            row += x/(chatWidth-1)+1;
            
            

        } else{
            int messageRowLength = x/(chatWidth-1)+1;
            for(int j = 0; j < messageRowLength; j++){
                for(int k = 0; k < chatHeight2-1; k++){
                    strcpy(messageHistory[k], messageHistory[k+1]);
                    mvwprintw(chat2, k, 0, "%s\n", messageHistory[k]);
                }
                memset(messageHistory[row-1], 0 , 1024);
            }
            
            for(int k = 0; k < x; k++)
                messageHistory[row-1 - messageRowLength+1 + k/(chatWidth-2)][k%(chatWidth-2)] = message[k];
            
            for(int k = row-1 - messageRowLength; k <= row-1; k++)
                mvwprintw(chat2, k, 0, "%s\n", messageHistory[k]);
            
            wrefresh(chat2);
            
        }
        
        wrefresh(chat2);
        werase(input2);
        wrefresh(input2);
        x = y = 0;

        char temp[2] ="0";
        temp[0] = '\n';
        strcat(message, temp);
        sendbytes = strlen(message);
        if(write(sockfd, message, sendbytes) != sendbytes)
            err_n_exit_win("write error.");
            
        /*sendbytes = strlen(sendbuff);
        if(write(sockfd, sendbuff, sendbytes) != sendbytes)
            err_n_exit_win("write error.");*/
    }
    

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

    int finished = 0;
    while(!finished){

        memset(recvbuff, 0, 1024);
        int i = 0;
        while((n = read(sockfd, recvbuff, MAXLINE-1)) > 0){ 
            i += n;
            if(recvbuff[n-1] == '\n'){   //check for end of message
                    
                    break;
            }
        }
        if(*row < chatHeight2){
            for(int j = 0; j < i; j++)
                messageHistory[*row + j/(chatWidth-2)][j%(chatWidth-2)] = recvbuff[j];
            for(int j = 0; j < chatHeight2; j++)
                mvwprintw(chat2, j, 0, "%s\n", messageHistory[j]);
            *row += *x/(chatWidth-1)+1;
            wrefresh(chat2);
        } else {
            int messageRowLength = i/(chatWidth-1)+1;
            for(int j = 0; j < messageRowLength; j++){
                for(int k = 0; k < chatHeight2-1; k++){
                    strcpy(messageHistory[k], messageHistory[k+1]);
                    mvwprintw(chat2, k, 0, "%s\n", messageHistory[k]);
                }
                memset(messageHistory[*row-1], 0 , 1024);
            }

            for(int k = 0; k < i; k++)
                messageHistory[*row-1 - messageRowLength+1 + k/(chatWidth-2)][k%(chatWidth-2)] = recvbuff[k];
            for(int k = *row-1 - messageRowLength; k <= *row-1; k++)
                mvwprintw(chat2, k, 0, "%s\n", messageHistory[k]);
            
            wrefresh(chat2);
        }
        wrefresh(chat2);
        *x = 0;
    }
    pthread_exit(NULL);
}

void err_n_exit_win(const char *fmt, ...){
    int errno_save;
    va_list ap;

    errno_save = errno; //any system or library call can set errno, so we need to save it here

    //print out the fmt+args to standard out
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    fprintf(stdout, "\n");
    fflush(stdout);

    //print out error message if erno was set
    if(errno_save) {
        fprintf(stdout, "(errono = %d) : %s\n", errno_save, strerror(errno_save));
        fprintf(stdout, "\n");
        fflush(stdout);
    }
    va_end(ap);

    endwin();
    exit(1);
}