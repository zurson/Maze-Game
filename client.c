#include "client_defs.h"
#include <ncurses.h>

#include <pthread.h>
#include <string.h>
#include <sys/socket.h>


int connected = 1;
int client_socket;
char join[2];
pthread_t client_render, keyboard_handler;

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
        bytes_received = recv(client_socket, arr, 37, 0);
        recv_check(bytes_received, &connected);

        bytes_received = recv(client_socket, coordinates, 20, 0);
        recv_check(bytes_received, &connected);

        bytes_received = recv(client_socket, deaths, 20, 0);
        recv_check(bytes_received, &connected);

        bytes_received = recv(client_socket, coins_brought, 20, 0);
        recv_check(bytes_received, &connected);

        bytes_received = recv(client_socket, coins_found, 20, 0);
        recv_check(bytes_received, &connected);

        if(!connected || connected == -1)
        {
            pthread_cancel(keyboard_handler);
            end_game(client_socket, connected);
        }

        clear();
        write_map(arr);
        show_informations(1, 10, coordinates, deaths, coins_found, coins_brought, connected);
        show_legend(10, 10);
        refresh();
    }

    pthread_exit(NULL);
}

void* client_keyboard(void* arg)
{
    while (connected)
    {
        int key = getch();

        switch (key)
        {
            case 'Q':
            case 'q':
                *join = 'Q';
                send(client_socket, join, 2, 0);
                pthread_cancel(client_render);
                connected = 0;
                pthread_exit(NULL);

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

    pthread_exit(NULL);
}

int main ()
{
    if(create_connection(&client_socket, join, &connected))
        return 1;

    start_ncurses();

    pthread_create(&client_render, NULL, render_map, NULL);
    pthread_create(&keyboard_handler, NULL, client_keyboard, NULL);

    pthread_join(keyboard_handler, NULL);

    pthread_cancel(client_render);
    end_game(client_socket, connected);
    return 0;
}
