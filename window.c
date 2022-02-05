#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

int main(){
    setlocale(LC_CTYPE, "");
    initscr();
    cbreak();
    noecho();
    

    int yMax, xMax, borderTop, borderSide, chatHeight, chatWidth, chatHeight2;
    borderTop = 1;
    borderSide = 2;
    getmaxyx(stdscr, yMax, xMax);
    chatHeight = (int)(yMax*0.75);
    chatWidth = xMax - 2*borderSide;
    chatHeight2 = chatHeight-2;

    WINDOW *chat = newwin(chatHeight, chatWidth,borderTop,borderSide);
    WINDOW *chat2 = newwin(chatHeight-2, chatWidth-2, borderTop+1,borderSide+1);
    WINDOW *input = newwin(yMax - chatHeight - borderTop*2, chatWidth, chatHeight + borderTop, borderSide);
    WINDOW *input2 = newwin(yMax - chatHeight - borderTop*2 -2, chatWidth-2, chatHeight + borderTop + 1, borderSide + 1);
    box(chat,0,0);
    box(input, 0,0);
    refresh();
    wrefresh(chat);
    wrefresh(input);
    keypad(input2, 1);

    char messageHistory[chatHeight2][1024];
    for(int j = 0; j < chatHeight2; j++)
        memset(messageHistory[j], 0, 1024);

    

    char message[1024];

    
    int c, i, row, x, y;
    row = x = y = 0;
    
    while(1){
        memset(message, 0, 1024);
        i = 0;
        wmove(input2, 0,0);
        while((c = wgetch(input2)) != '\n'){
            message[i++] = c;
            mvwprintw(input2, y, 0, "%s ", message);
            werase(input2);     //hacky way to get utf8 characters to work in the input box
            wrefresh(input2);
            mvwprintw(input2, y, 0, "%s", message);
            x++;
        }

        if(row < chatHeight2){
            for(i = 0; i < x; i++)
                messageHistory[row + i/(chatWidth-2)][i%(chatWidth-2)] = message[i];
            for(i = 0; i < chatHeight2; i++)
                mvwprintw(chat2, i, 0, "%s\n", messageHistory[i]);
            row += x/(chatWidth-1)+1;
        } else{
            int messageRowLength = x/(chatWidth-1)+1;
            for(int j = 0; j < messageRowLength; j++){
                for(i = 0; i < chatHeight2-1; i++){
                    strcpy(messageHistory[i], messageHistory[i+1]);
                    mvwprintw(chat2, i, 0, "%s\n", messageHistory[i]);
                }
                memset(messageHistory[row-1], 0 , 1024);
            }
            
            for(i = 0; i < x; i++)
                messageHistory[row-1 - messageRowLength+1 + i/(chatWidth-2)][i%(chatWidth-2)] = message[i];
            
            for(i = row-1 - messageRowLength; i <= row-1; i++)
                mvwprintw(chat2, i, 0, "%s\n", messageHistory[i]);
            
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