#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

/*
THIS PROGRAM IS NOT ACUTALLY RUN
IT IS ONLY A TEST PROGRAM FOR USING NCURSES
*/


int main(){
    setlocale(LC_CTYPE, "");
    initscr();
    cbreak();
    noecho();
    start_color();
    

    int yMax, xMax, borderTop, borderSide, chatHeight, chatWidth, chatHeight2, inputHeight, inputHeight2;
    borderTop = 1;
    borderSide = 2;
    getmaxyx(stdscr, yMax, xMax);
    chatHeight = (int)(yMax*0.75);
    chatWidth = xMax - 2*borderSide;
    chatHeight2 = chatHeight-2;
    inputHeight = yMax - chatHeight - borderTop*2;
    inputHeight2 = inputHeight - 2;

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

    char messageHistory[chatHeight2][1024];
    for(int j = 0; j < chatHeight2; j++)
        memset(messageHistory[j], 0, 1024);

    

    char message[1024];

    
    unsigned int c, i, row, x, y;
    row = x = y = 0;
    
    while(1){
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
        
    }
    getch();
    endwin();
    return 0;
}