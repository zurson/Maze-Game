#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include "defs.h"
#include <time.h>

#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int server_status = 1;
int server_socket;
struct board_t* board;

pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
pthread_t game_thread, new_connection_thread;

void move_beast(struct beast_t* beast)
{
    struct point_t temp;

    if(beast->bush)
    {
        beast->bush--;
        return;
    }

    temp.x = beast->location.x;
    temp.y = beast->location.y;

    switch(beast->status)
    {
        case UP:
            temp.x--;
            break;

        case DOWN:
            temp.x++;
            break;

        case LEFT:
            temp.y--;
            break;

        case RIGHT:
            temp.y++;
            break;

        default:
            return;
    }

    enum action action;

    pthread_mutex_lock(&locker);

    beast->collision_flag = beast_collisions(temp, board, &action);
    if(beast->collision_flag && action == NOTHING)
    {
        pthread_mutex_unlock(&locker);
        return;
    }

    if(action == PLAYER_COLLISION)
    {
        char id[2];
        *id = *(*(board->board + temp.x) + temp.y);
        beast_catch(board, id);
    }

    if(board->board[temp.x][temp.y] == '%')
        beast->bush = 1;

    char step = *(*(board->board + temp.x) + temp.y);
    *(*(board->board+beast->location.x)+beast->location.y) = beast->ch;
    beast->ch = step;

    beast->location.x = temp.x;
    beast->location.y = temp.y;

    *(*(board->board + temp.x) + temp.y) = '*';
    pthread_mutex_unlock(&locker);
}

void* beast(void* arg)
{
    struct beast_t* beast = (struct beast_t*)arg;

    srand(time(NULL));

    scan_for_player(board, beast);

    if (beast->chase_flag == 0) {
        if (beast->collision_flag)
            beast->status = rand() % 4;

        move_beast(beast);

        pthread_exit(NULL);
    } else {
        if (beast->location.x < beast->target_loc.x) {
            beast->status = DOWN;

            move_beast(beast);

            pthread_exit(NULL);
        }
        if (beast->location.x > beast->target_loc.x) {
            beast->status = UP;

            move_beast(beast);

            pthread_exit(NULL);
        }

        if (beast->location.y < beast->target_loc.y) {
            beast->status = RIGHT;

            move_beast(beast);

            pthread_exit(NULL);
        }
        if (beast->location.y > beast->target_loc.y) {
            beast->status = LEFT;

            move_beast(beast);

            pthread_exit(NULL);
        }
    }

    pthread_exit(NULL);
}


void* game_loop(void* arg)
{
    long round = 1;
    int index;

    while (server_status)
    {
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            struct player_t* player = &board->player[i];

            if (player->status == NO_PLAYER || player->status == NO_MOVE)
                continue;

            move_player(board, player);
        }

        index = 0;
        for(int i=0; i < MAX_BEASTS; i++)
        {
            if(board->beast[i].status != NO_BEAST)
            {
                pthread_create(&board->beast[i].thread, NULL, beast, &board->beast[i]);
                index++;
            }
        }

        for(int i=0; i<index; i++)
            pthread_join(board->beast[i].thread, NULL);

        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            struct player_t* player = &board->player[i];

            if (player->status == NO_PLAYER)
                continue;

            draw_player_board(board, player);
            send(player->socket_id, player->arr, 37, 0);
            send_data_for_player(player->socket_id, player);
        }

        clear();
        display_board_server(board);
        display_server_stats(75, round, server_socket, board);
        refresh();

        usleep(TOUR_TIME);
        round++;
    }

    return NULL;
}

void* keyboard_handler(void* arg)
{
    while (server_status)
    {
        unsigned char key = getch();

        switch (key)
        {
            case 'Q':
            case 'q':
                server_status = 0;
                pthread_exit(NULL);

            case 'c':
                add_new_coin(board, 'c');
                break;

            case 't':
                add_new_coin(board, 't');
                break;

            case 'T':
                add_new_coin(board, 'T');
                break;

            case 'B':
            case 'b':
                add_beast(board);
                break;

            default:
                break;
        }
    }

    pthread_exit(NULL);
}

void* handle_client_connection(void* arg)
{
    struct player_t *player = (struct player_t *) arg;

    char buff[2], ch;
    long bytes_received;

    while (server_status)
    {
        bytes_received = recv(player->socket_id, buff, 2, 0);

        if (byte_check(bytes_received, board, player))
            pthread_exit(NULL);

        ch = *buff;

        switch (ch)
        {
            case 'W':
                update_player_move(player, board, UP);
                break;

            case 'S':
                update_player_move(player, board, DOWN);
                break;

            case 'A':
                update_player_move(player, board, LEFT);
                break;

            case 'D':
                update_player_move(player, board, RIGHT);
                break;

            case 'Q':
            case 'q':
                if (player->bush)
                    *(*(board->board + player->location.x) + player->location.y) = '%';
                else
                    *(*(board->board + player->location.x) + player->location.y) = ' ';

                set_stock_player(player);
                break;

            default:
                break;
        }
    }

    pthread_exit(NULL);
}

void* wait_for_client(void* arg)
{
    char tab[5] = {'1', '2', '3', '4', 0};
    char buff[2];
    int i, flag;
    enum error code;
    int x, y;

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);

    while(server_status)
    {
        flag = -1;

        int client_socket = accept(server_socket, (struct sockaddr*)&client, &client_len);
        if (client_socket == -1)
            continue;

        for(i=0; i<MAX_PLAYERS; i++)
        {
            if(board->player[i].status == NO_PLAYER)
            {
                flag = i;
                break;
            }
        }

        free_coordinates(board, &code, &x, &y);

        if(flag == -1 || code == NO_SPACE)
        {
            buff[0] = 'F';
            send(client_socket, buff, 2, 0);
            continue;
        }

        board->player[i].socket_id = client_socket;
        board->player[i].location.x = x;
        board->player[i].location.y = y;
        board->player[i].status = NO_MOVE;
        board->board[x][y] = tab[i];

        buff[0] = ((i+1) + '0');
        send(client_socket, buff, 2, 0);

        if (server_status)
            pthread_create(&board->player[i].thread, NULL, handle_client_connection, &board->player[i]);
        else
            break;
    }

    pthread_exit(NULL);
}

int main()
{
    pthread_t keyboard_thread;

    if(start_socket(&server_socket))
        return 1;

    board = create_basic_board(server_socket);
    if(!board)
        return 1;

    spawn_coins_and_beasts(board, beast);

    start_ncurses();

    pthread_create(&game_thread, NULL, game_loop, NULL);
    pthread_create(&keyboard_thread, NULL, keyboard_handler, NULL);
    pthread_create(&new_connection_thread, NULL, wait_for_client, NULL);

    pthread_join(keyboard_thread, NULL);

    pthread_cancel(new_connection_thread);
    pthread_cancel(game_thread);
    kill_player_threads(board);

    end_game(server_socket, board);
    pthread_mutex_destroy(&locker);

    return 0;
}
