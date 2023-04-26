#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PLAYER_PAIR 1
#define PATH_PAIR 2
#define WALL_PAIR 3
#define CAMPSITE_PAIR 4
#define COIN_PAIR 5
#define BEAST_PAIR 6
#define BUSHES_PAIR 7
#define DROPPED_PAIR 8


int connected = 1;
int client_socket;
char join[2];

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
                sscanf(arr+i, "%d", &connected);
                break;

            case '%':
                attron(COLOR_PAIR(BUSHES_PAIR));
                mvaddch(j, k, '#' );
                attroff(COLOR_PAIR(BUSHES_PAIR));
                break;

            case 'D':
                attron(COLOR_PAIR(DROPPED_PAIR));
                mvaddch(i,j, ch );
                attroff(COLOR_PAIR(DROPPED_PAIR));
                break;

            default:
                break;
        }
    }
}

void recv_check(long bytes)
{
    if (bytes <= 0)
    {
        connected = 0;
        clear();
        endwin();
        printf("\n\n\tConnection lost!\n\n\n");
        close(client_socket);
        exit(1);
    }
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

void show_informations(int a, int w, char* coordinates, char* deaths, char* coins_found, char* coins_brought)
{
    char tab[5];
    int c = w + 25;
    int b = w+3;

    mvprintw(a, w, "Informations:");

    mvprintw(a+2, b, "Curr Y/X");
    mvprintw(a+2, c, coordinates);

    mvprintw(a+3, b, "Campsite Y/X");
    mvprintw(a+3, c, "9/40");

    mvprintw(a+4, b, "Deaths");
    mvprintw(a+4, c, deaths);

    mvprintw(a+5, b, "Coins found");
    mvprintw(a+5, c, coins_found);

    mvprintw(a+6, b, "Coins brought");
    mvprintw(a+6, c, coins_brought);

    sprintf(tab, "%d", connected);
    mvprintw(a+7, b, "Your number");
    mvprintw(a+7, c, tab);
}

void* render_map(void* arg)
{
    char arr[100];
    char coordinates[20];
    char deaths[20];
    char coins_brought[20];
    char coins_found[20];

    long bytes_received;

    while (connected)
    {
/*        memset(arr, 0, 37);
        memset(coordinates, 0, 20);
        memset(deaths, 0, 20);
        memset(coins_brought, 0, 20);
        memset(coins_found, 0, 20);*/

        bytes_received = recv(client_socket, arr, 37, 0);
        recv_check(bytes_received);

        bytes_received = recv(client_socket, coordinates, 20, 0);
        recv_check(bytes_received);

        bytes_received = recv(client_socket, deaths, 20, 0);
        recv_check(bytes_received);

        bytes_received = recv(client_socket, coins_brought, 20, 0);
        recv_check(bytes_received);

        bytes_received = recv(client_socket, coins_found, 20, 0);
        recv_check(bytes_received);

        clear();
        write_map(arr);
        show_informations(1, 10, coordinates, deaths, coins_found, coins_brought);
        show_legend(10, 10);
        refresh();
    }

    pthread_exit(NULL);
}

int main ()
{
    struct sockaddr_in servaddr;

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        printf("\n\n\tFailed to create socket!\n\n\n");
        return 1;
    }

    bzero(&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(8989);

    if (connect(client_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) != 0)
    {
        printf("\n\n\tCan't connect to the server\n\n\n");
        return 1;
    }

    memset(join, 0, 2);

    *join = 'J';
    send(client_socket, join, 2, 0);

    recv(client_socket, join, 2, 0);

    if(*join == 'F')
    {
        printf("\n\n\tServer is full!\n\n\n");
        close(client_socket);
        return 1;
    }

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
    init_pair(BEAST_PAIR, COLOR_RED, COLOR_WHITE);
    init_pair(BUSHES_PAIR, COLOR_BLACK, COLOR_WHITE);
    init_pair(DROPPED_PAIR, COLOR_GREEN, COLOR_YELLOW);

    pthread_t client_render;
    pthread_create(&client_render, NULL, render_map, NULL);


    while (connected)
    {
        int key = getch();

        switch (key)
        {
            case 'Q':
            case 'q':
                *join = 'Q';
                send(client_socket, join, 2, 0);
                connected = 0;
                break;

            case KEY_UP:
                *join = 'W';
                send(client_socket, join, 2, 0);
                break;

            case KEY_DOWN:
                *join = 'S';
                send(client_socket, join, 2, 0);
                break;

            case KEY_RIGHT:
                *join = 'D';
                send(client_socket, join, 2, 0);
                break;

            case KEY_LEFT:
                *join = 'A';
                send(client_socket, join, 2, 0);
                break;

            default:
                continue;
        }
    }


    clear();
    refresh();
    endwin();
    close(client_socket);
    printf("\n\n\tYou have left the game!\n\n\n");
    return 0;
}
