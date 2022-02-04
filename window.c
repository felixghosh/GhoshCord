#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

int main(){
    setlocale(LC_CTYPE, "");
    initscr();
    cbreak();

    int yMax, xMax;
    getmaxyx(stdscr, yMax, xMax);

    WINDOW *chat = newwin((int)(yMax*0.75),xMax-4,1,2);
    WINDOW *chat2 = newwin((int)(yMax*0.75)-2,xMax-6,2,3);
    WINDOW *input = newwin(yMax - (int)(yMax*0.75) - 2, xMax-4, (int)(yMax*0.75)+ 1, 2);
    WINDOW *input2 = newwin(yMax - (int)(yMax*0.75) - 4, xMax-6, (int)(yMax*0.75)+ 2, 3);
    box(chat,0,0);
    box(input, 0,0);
    refresh();
    wrefresh(chat);
    wrefresh(input);

    mvwprintw(chat2, 0, 0, "Felix: Hejsan!");
    wrefresh(chat2);

    

    char message[1024];
    
    int c, i, row;
    row = 1;
    
    
    while(1){
        memset(message, 0, 1024);
        i = 0;
        wmove(input2, 0,0);
        while((c = wgetch(input2)) != '\n')
            message[i++] = c;
        mvwprintw(chat2, row++, 0, "%s\n", message);
        wrefresh(chat2);
        werase(input2);
        //box(input2, 0,0);
        wrefresh(input2);
    }
    getch();
    endwin();
    return 0;
}