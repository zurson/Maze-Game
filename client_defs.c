#include "client_defs.h"
#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int create_connection(int* client_socket, char* join, int* connected)
{
    struct sockaddr_in servaddr;

    *client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (*client_socket == -1)
    {
        printf("\n\n\tFailed to create socket!\n\n\n");
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(PORT);

    if (connect(*client_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("\n\n\tCan't connect to the server\n\n\n");
        return 1;
    }

    memset(join, 0, 2);
    recv(*client_socket, join, 2, 0);

    if(*join == 'F')
    {
        printf("\n\n\tServer is full!\n\n\n");
        close(*client_socket);
        return 1;
    }

    *connected = *join;

    return 0;
}

void start_ncurses()
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    start_color();

    init_pair(PLAYER_PAIR, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(PATH_PAIR, COLOR_WHITE, COLOR_WHITE);
    init_pair(WALL_PAIR, COLOR_BLUE, COLOR_BLUE);
    init_pair(CAMPSITE_PAIR, COLOR_YELLOW, COLOR_GREEN);
    init_pair(COIN_PAIR, COLOR_BLACK, COLOR_YELLOW);
    init_pair(BEAST_PAIR, COLOR_WHITE, COLOR_RED);
    init_pair(BUSHES_PAIR, COLOR_BLACK, COLOR_WHITE);
    init_pair(DROPPED_PAIR, COLOR_GREEN, COLOR_YELLOW);
}

void end_game(int client_socket, int connected)
{
    endwin();
    system("clear");
    close(client_socket);

    if(!connected)
        printf("\n\n\tYou have left the game!\n\n\n");
    else
        printf("\n\n\tConnection lost!\n\n\n");
}

void show_informations(int a, int w, char* coordinates, char* deaths, char* coins_found, char* coins_brought, int nr)
{
    char tab[2] = {'\0', '\0'};
    int c = w + 25;
    int b = w+3;

    mvprintw(a, w, "Informations:");

    mvprintw(a+2, b, "Curr Y/X");
    mvprintw(a+2, c, coordinates);

    mvprintw(a+3, b, "Campsite Y/X");
    mvprintw(a+3, c, "8/35");

    mvprintw(a+4, b, "Deaths");
    mvprintw(a+4, c, deaths);

    mvprintw(a+5, b, "Coins found");
    mvprintw(a+5, c, coins_found);

    mvprintw(a+6, b, "Coins brought");
    mvprintw(a+6, c, coins_brought);

    *tab = nr;
    mvprintw(a+7, b, "Your number");
    mvprintw(a+7, c, tab);
}

void show_legend(int a, int w)
{
    int c = w + 10;
    int b = w+3;

    mvprintw(a, w, "Legend:");

    attron(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(a+2, b, "1234");
    attroff(COLOR_PAIR(PLAYER_PAIR));
    mvprintw(a+2, c, "- players");

    attron(COLOR_PAIR(WALL_PAIR));
    mvaddch(a+3, b, ACS_CKBOARD);
    attroff(COLOR_PAIR(WALL_PAIR));
    mvprintw(a+3, c, "- wall");

    mvaddch(a+4, b, '#');
    mvprintw(a+4, c, "- bushes (slow down)");

    attron(COLOR_PAIR(BEAST_PAIR));
    mvaddch(a+5, b, '*');
    attroff(COLOR_PAIR(BEAST_PAIR));
    mvprintw(a+5, c, "- enemy");

    attron(COLOR_PAIR(COIN_PAIR));
    mvaddch(a+6, b, 'c');
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(a+6, c, "- one coin");

    attron(COLOR_PAIR(COIN_PAIR));
    mvaddch(a+7, b, 't');
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(a+7, c, "- treasure (10 coins)");

    attron(COLOR_PAIR(COIN_PAIR));
    mvaddch(a+8, b, 'T');
    attroff(COLOR_PAIR(COIN_PAIR));
    mvprintw(a+8, c, "- large treasure (50 coins)");

    attron(COLOR_PAIR(CAMPSITE_PAIR));
    mvaddch(a+9, b, 'A');
    attroff(COLOR_PAIR(CAMPSITE_PAIR));
    mvprintw(a+9, c, "- campsite");
}

void write_map(char* arr)
{
    int j=1;
    int k=1;

    for(int i=0; *(arr+i); i++, k++)
    {
        char ch = *(arr+i);

        if(ch == 'x')
        {
            j++;
            k=0;
            continue;
        }

        switch(ch)
        {
            case '#':
                attron(COLOR_PAIR(WALL_PAIR));
                mvaddch(j, k, ACS_CKBOARD );
                attroff(COLOR_PAIR(WALL_PAIR));
                break;

            case ' ':
                attron(COLOR_PAIR(PATH_PAIR));
                mvaddch(j, k, ch );
                attroff(COLOR_PAIR(PATH_PAIR));
                break;

            case 'A':
                attron(COLOR_PAIR(CAMPSITE_PAIR));
                mvaddch(j, k, ch );
                attroff(COLOR_PAIR(CAMPSITE_PAIR));
                break;

            case 'T':
            case 't':
            case 'c':
                attron(COLOR_PAIR(COIN_PAIR));
                mvaddch(j, k, ch );
                attroff(COLOR_PAIR(COIN_PAIR));
                break;

            case '1':
            case '2':
            case '3':
            case '4':
                attron(COLOR_PAIR(PLAYER_PAIR));
                mvaddch(j, k, ch);
                attroff(COLOR_PAIR(PLAYER_PAIR));
                break;

            case '%':
                attron(COLOR_PAIR(BUSHES_PAIR));
                mvaddch(j, k, '#' );
                attroff(COLOR_PAIR(BUSHES_PAIR));
                break;

            case 'D':
                attron(COLOR_PAIR(DROPPED_PAIR));
                mvaddch(j,k, ch );
                attroff(COLOR_PAIR(DROPPED_PAIR));
                break;

            case '*':
                attron(COLOR_PAIR(BEAST_PAIR));
                mvaddch(j,k, ch );
                attroff(COLOR_PAIR(BEAST_PAIR));
                break;

            default:
                break;
        }
    }
}

void recv_check(long bytes, int* connected)
{
    if (bytes <= 0)
        *connected = -1;
}