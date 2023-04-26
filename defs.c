#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void init_player(struct player_t* player, int x, int y)
{
    player->location.x = x;
    player->location.y = y;
    player->status = NO_PLAYER;
    player->coins = 0;
    player->score = 0;
    player->deaths = 0;
    player->bush = 0;
    player->socket_id = 0;
}

void init_beast(struct beast_t* beast, int x, int y)
{
    beast->location.x = x;
    beast->location.y = y;
    beast->status = NO_BEAST;
    beast->bush = 0;
    beast->ch = ' ';
    beast->collision_flag = 0;
    beast->chase_flag = 0;
}

struct board_t* init_board(enum error* code)
{
    if(!code)
        return NULL;

    struct board_t* board;

    board = calloc(1, sizeof(struct board_t));
    if(!board)
    {
        *code = ALLOCATION_FAILED;
        return NULL;
    }

    char** tab = read_board(code, &board->size.x, &board->size.y);
    if(!tab)
        return NULL;
    board->board = tab;

    for(int i=0; i<MAX_PLAYERS; i++)
        init_player(&board->player[i], 0, 0);

    for(int i=0; i<MAX_BEASTS; i++)
        init_beast(&board->beast[i], 0, 0);

    return board;
}

void free_board(char** board)
{
    if(!board)
        return;

    for(int i=0; *(board + i); i++)
        free(*(board+i));
    free(board);
}

char** read_board(enum error* code, int* x, int* y)
{
    FILE* stream = fopen("board2.txt", "r");
    if(!stream)
    {
        *code = NO_FILE;
        return NULL;
    }

    char** board = calloc(1, sizeof(char*));
    if(!board)
    {
        *code = ALLOCATION_FAILED;
        fclose(stream);
        return NULL;
    }

    rewind(stream);

    int width;
    int height = 1;

    while(!feof(stream))
    {
        width=1;
        int ch = fgetc(stream);

        while(ch != '\n' && ch != EOF)
        {
            char* wsk = realloc(*(board + height - 1), (width+1) * sizeof(char));
            if(!wsk)
            {
                printf("Failed to allocate memory\n");
                free_board(board);
                fclose(stream);
                return NULL;
            }

            *(board + height - 1) = wsk;

            *(*(board + height - 1) + width - 1) = ch;
            *(*(board + height - 1) + width) = 0;
            ch = fgetc(stream);
            width++;
        }

        height++;
        char** wsk = realloc(board, height * sizeof(char*));
        if(!wsk)
        {
            printf("Failed to allocate memory\n");
            free_board(board);
            fclose(stream);
            return NULL;
        }
        board = wsk;
        *(board + height - 1) = NULL;
    }

    *y = width-1;
    *x = height-1;

    fclose(stream);
    return board;
}

void display_board_server(struct board_t* board)
{
    if(!board)
        return;

    for(int i=0; *(board->board+i); i++)
    {
        for(int j=0; *(*(board->board+i)+j); j++)
        {
            char ch = (*(*(board->board+i)+j));

            switch(ch)
            {
                case '#':
                    attron(COLOR_PAIR(WALL_PAIR));
                    mvaddch(i,j, ACS_CKBOARD );
                    attroff(COLOR_PAIR(WALL_PAIR));
                    break;

                case ' ':
                    attron(COLOR_PAIR(PATH_PAIR));
                    mvaddch(i,j, ch );
                    attroff(COLOR_PAIR(PATH_PAIR));
                    break;

                case 'A':
                    attron(COLOR_PAIR(CAMPSITE_PAIR));
                    mvaddch(i,j, ch );
                    attroff(COLOR_PAIR(CAMPSITE_PAIR));
                    break;

                case 'T':
                case 't':
                case 'c':
                    attron(COLOR_PAIR(COIN_PAIR));
                    mvaddch(i,j, ch );
                    attroff(COLOR_PAIR(COIN_PAIR));
                    break;

                case '1':
                case '2':
                case '3':
                case '4':
                    attron(COLOR_PAIR(PLAYER_PAIR));
                    mvaddch(i, j, ch);
                    attroff(COLOR_PAIR(PLAYER_PAIR));
                    break;

                case '%':
                    attron(COLOR_PAIR(BUSHES_PAIR));
                    mvaddch(i,j, '#' );
                    attroff(COLOR_PAIR(BUSHES_PAIR));
                    break;

                case 'D':
                    attron(COLOR_PAIR(DROPPED_PAIR));
                    mvaddch(i,j, ch );
                    attroff(COLOR_PAIR(DROPPED_PAIR));
                    break;

                case '*':
                    attron(COLOR_PAIR(BEAST_PAIR));
                    mvaddch(i,j, ch );
                    attroff(COLOR_PAIR(BEAST_PAIR));
                    break;

                default:
                    break;
            }
        }
    }
}

void draw_player_board(struct board_t* board, struct player_t* player)
{
    if(player->status == NO_PLAYER)
        return;

    int x = player->location.x;
    int y = player->location.y;

    int new_y1, new_x1, new_x2, new_y2;

    new_x1 = x-2;
    while(new_x1 < 0)
        new_x1++;

    new_y1 = y+3;
    while(new_y1 > board->size.y)
        new_y1--;
    // ------------------------------
    new_x2 = x+3;
    while(new_x2 > board->size.x)
        new_x2--;

    new_y2 = y-2;
    while(new_y2 < 0)
        new_y2++;

    int index=0;

    for(int i=new_x1; i<new_x2; i++)
    {
        for(int j=new_y2; j<new_y1; j++)
        {
            player->arr[index] = board->board[i][j];
            index++;
        }
        player->arr[index] = 'x';
        index++;
    }
    player->arr[index] = 0;
}

void display_server_stats(int a, long round, int server_socket, struct board_t* board)
{
    mvprintw(1, a, "Server's PID:");
    mvprintw(1, a+15, "%d", server_socket);

    mvprintw(2, a, "Campsite Y/X:");
    mvprintw(2, a+15, "8/35");

    mvprintw(3, a, "Round");
    mvprintw(3, a+15, "%ld", round);

    mvprintw(5, a, "Parameter:");
    mvprintw(6, a, "PID");
    mvprintw(7, a, "Type");
    mvprintw(8, a, "Curr Y/X");
    mvprintw(9, a, "Deaths");
    mvprintw(10, a, "Carried");
    mvprintw(11, a, "Brought");


    mvprintw(5, a+15, "Player 1");
    mvprintw(5, a+25, "Player 2");
    mvprintw(5, a+35, "Player 3");
    mvprintw(5, a+45, "Player 4");
    int c = a+15;
    for(int i=0; i<MAX_PLAYERS; i++)
    {
        display_player_data(&board->player[i], c, 6);
        c+=10;
    }

    show_legend(13, a);
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



void coin_manager(struct player_t* player, struct board_t* board, struct point_t temp, int flag)
{
    *(*(board->board + temp.x) + temp.y) = ' ';

    switch(flag)
    {
        case 'c':
            player->coins += 1;
            add_new_coin(board, 'c');
            break;

        case 't':
            player->coins += 10;
            add_new_coin(board, 't');
            break;

        case 'T':
            player->coins += 50;
            add_new_coin(board, 'T');
            break;

        default:
            break;
    }
}

void campsite_manager(struct player_t* player)
{
    player->score += player->coins;
    player->coins = 0;
}

void bush_manager(struct player_t* player, struct board_t* board, struct point_t temp)
{
    char ch = *(*(board->board + player->location.x) + player->location.y);

    if(player->bush)
        *(*(board->board+player->location.x)+player->location.y) = '%';
    else
        *(*(board->board+player->location.x)+player->location.y) = ' ';

    player->bush = 2;

    *(*(board->board + temp.x) + temp.y) = ch;

    player->location.x = temp.x;
    player->location.y = temp.y;

    if(DISABLE_INSTANT_MOVE)
        player->status = NO_MOVE;
}

void dropped_chest_manager(struct board_t* board, struct point_t temp, struct player_t* player, int* bush_flag)
{
    *(*(board->board + temp.x) + temp.y) = ' ';
    player->coins += abs(*(*(board->dropped + temp.x) + temp.y));

    if(*(*(board->dropped + temp.x) + temp.y) < 0)
        *bush_flag = 1;

    *(*(board->dropped + temp.x) + temp.y) = 0;
}

void player_collision_manager(struct player_t* player, struct board_t* board, struct point_t temp, int flag)
{
    int dropped = player->coins + board->player[flag-1].coins;
    int x1, x2, y1, y2;
    enum error code;
    char nr_1;
    char nr_2[5];
    int bush=1;

    nr_1 = *(*(board->board + player->location.x) + player->location.y);
    sprintf(nr_2, "%d", flag);

    free_coordinates(board, &code, &x1, &y1);
    *(*(board->board + x1) + y1) = nr_1;
    free_coordinates(board, &code, &x2, &y2);
    *(*(board->board + x2) + y2) = *nr_2;

    if(player->bush)
        *(*(board->board + player->location.x) + player->location.y) = '%';
    else
        *(*(board->board + player->location.x) + player->location.y) = ' ';

    if(board->player[flag-1].bush)
        *(*(board->board + board->player[flag-1].location.x) + board->player[flag-1].location.y) = '%';
    else
        *(*(board->board + board->player[flag-1].location.x) + board->player[flag-1].location.y) = ' ';

    if(board->player[flag-1].bush)
        bush = -1;

    player->coins = 0;
    board->player[flag-1].coins = 0;

    board->player[flag-1].bush = 0;
    player->bush = 0;

    if(dropped)
    {
        *(*(board->board + temp.x) + temp.y) = 'D';
        *(*(board->dropped + temp.x) + temp.y) = dropped * bush;
    }

    player->location.x = x1;
    player->location.y = y1;
    board->player[flag-1].location.x = x2;
    board->player[flag-1].location.y = y2;

    if(DISABLE_INSTANT_MOVE)
    {
        player->status = NO_MOVE;
        board->player[flag-1].status = NO_MOVE;
    }
}



void move_player(struct board_t* board, struct player_t* player)
{
    int flag;
    int bush_flag=0;
    struct point_t temp;
    const char player_id = *(*(board->board + player->location.x) + player->location.y);

    if(player->bush == 2)
    {
        player->bush--;
        return;
    }

    temp.x = player->location.x;
    temp.y = player->location.y;

    switch(player->status)
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

    enum action result = action(&flag, board, temp);

    switch(result)
    {
        case COIN:
            coin_manager(player, board, temp, flag);
            break;

        case CAMPSITE:
            campsite_manager(player);
            break;

        case BUSH:
            bush_manager(player, board, temp);
            break;

        case DROPPED_CHEST:
            dropped_chest_manager(board, temp, player, &bush_flag);
            break;

        case PLAYER_COLLISION:
            player_collision_manager(player, board, temp, flag);
            return;

        default:
            break;
    }

    if(collision(temp, board))
        return;

    if(player->bush)
    {
        *(*(board->board+player->location.x)+player->location.y) = '%';
        player->bush = 0;
    }
    else
        *(*(board->board+player->location.x)+player->location.y) = ' ';


    player->location.x = temp.x;
    player->location.y = temp.y;

    *(*(board->board + temp.x) + temp.y) = player_id;

    if(bush_flag)
        player->bush = 2;

    if(DISABLE_INSTANT_MOVE)
        player->status = NO_MOVE;
}

void update_player_move(struct player_t* player, struct board_t* board, enum status status)
{
    if(!player || player->status == NO_PLAYER)
        return;

    player->status = status;
}

int is_coin(struct point_t pkt, struct board_t* board)
{
    if( *(*(board->board + pkt.x) + pkt.y) == 'T' ||  *(*(board->board + pkt.x) + pkt.y) == 't' || *(*(board->board + pkt.x) + pkt.y) == 'c')
        return *(*(board->board + pkt.x) + pkt.y);
    return 0;
}

int is_campsite(struct point_t pkt, struct board_t* board)
{
    if( *(*(board->board + pkt.x) + pkt.y) == 'A')
        return 1;
    return 0;
}

int is_bush(struct point_t pkt, struct board_t* board)
{
    if( *(*(board->board + pkt.x) + pkt.y) == '%')
        return 1;
    return 0;
}

int is_dropped(struct point_t pkt, struct board_t* board)
{
    if( *(*(board->board + pkt.x) + pkt.y) == 'D')
        return 1;
    return 0;
}

int is_player(struct point_t pkt, struct board_t* board)
{
    if( *(*(board->board + pkt.x) + pkt.y) == '1')
        return 1;
    else if( *(*(board->board + pkt.x) + pkt.y) == '2')
        return 2;
    else if( *(*(board->board + pkt.x) + pkt.y) == '3')
        return 3;
    else if( *(*(board->board + pkt.x) + pkt.y) == '4')
        return 4;
    return 0;
}

enum action action(int* flag, struct board_t* board, struct point_t temp)
{
    *flag = is_coin(temp, board);
    if(*flag)
        return COIN;

    if(is_bush(temp, board))
        return BUSH;

    if (is_campsite(temp, board))
        return CAMPSITE;

    if(is_dropped(temp, board))
        return DROPPED_CHEST;

    *flag = is_player(temp, board);
    if (*flag)
        return PLAYER_COLLISION;
    else
        return NOTHING;
}



int beast_collisions(struct point_t cell, struct board_t* board, enum action* action)
{
    switch (board->board[cell.x][cell.y])
    {
        case '1':
        case '2':
        case '3':
        case '4':
            *action = PLAYER_COLLISION;
            return 0;

        case '*':
        case 'A':
        case '#':
            *action = NOTHING;
            return 1;

        default:
            *action = NOTHING;
            return 0;
    }
}

void add_beast(struct board_t* board)
{
    int i;
    int flag = -1;
    enum error code;
    int x, y;

    srand(time(NULL));

    for(i=0; i<MAX_BEASTS; i++)
    {
        if(board->beast[i].status == NO_BEAST)
        {
            flag = i;
            break;
        }
    }

    free_coordinates(board, &code, &x, &y);

    if(flag == -1 || code == NO_SPACE)
        return;

    board->beast[i].status = rand() % 4;
    board->beast[i].location.x = x;
    board->beast[i].location.y = y;
    board->board[x][y] = '*';
}

void beast_catch(struct board_t* board, char* id)
{
    int player_id;
    sscanf(id, "%d", &player_id);

    struct player_t* player = &board->player[player_id-1];

    int dropped = player->coins;
    int x, y;
    enum error code;
    int bush=1;

    struct point_t temp;
    temp.x = player->location.x;
    temp.y = player->location.y;

    free_coordinates(board, &code, &x, &y);

    player->location.x = x;
    player->location.y = y;

    *(*(board->board + x) + y) = *id;

    if(player->bush)
    {
        *(*(board->board + temp.x) + temp.y) = '%';
        bush = -1;
    }
    else
        *(*(board->board + temp.x) + temp.y) = ' ';


    if(dropped)
    {
        *(*(board->board + temp.x) + temp.y) = 'D';
        *(*(board->dropped + temp.x) + temp.y) = dropped * bush;
    }

    player->coins = 0;
    player->bush = 0;
    player->deaths++;

    if(DISABLE_INSTANT_MOVE)
        player->status = NO_MOVE;
}

void scan_for_player(struct board_t* board, struct beast_t* beast)
{
    int x, y, flag;
    x = beast->location.x;
    y = beast->location.y;


    for(int i=x; i<board->size.x; i++)
    {
        flag = beast_scan_status(i, y, board, beast);

        if(flag == 1)
            return;
        else if(flag == 2)
            break;
    }

    for(int i=x; i>0; i--)
    {
        flag = beast_scan_status(i, y, board, beast);

        if(flag == 1)
            return;
        else if(flag == 2)
            break;
    }

    for(int i=y; i<board->size.y; i++)
    {
        flag = beast_scan_status(x, i, board, beast);

        if(flag == 1)
            return;
        else if(flag == 2)
            break;
    }

    for(int i=y; i>0; i--)
    {
        flag = beast_scan_status(x, i, board, beast);

        if(flag == 1)
            return;
        else if(flag == 2)
            break;
    }

    beast->chase_flag = 0;
}

int beast_scan_status(int x, int y, struct board_t* board, struct beast_t* beast)
{
    struct point_t cell;
    enum action action;

    cell.x = x;
    cell.y = y;

    if(board->board[cell.x][cell.y]  == '#')
        return 2;

    beast_collisions(cell, board, &action);
    if(action == PLAYER_COLLISION)
    {
        beast->chase_flag = 1;
        beast->target_loc.x = cell.x;
        beast->target_loc.y = cell.y;
        return 1;
    }

    return 0;
}



void set_stock_player(struct player_t* player)
{
    if(!player)
        return;

    player->coins = 0;
    player->score = 0;
    player->deaths = 0;
    player->bush = 0;
    player->status = NO_PLAYER;

    if(player->socket_id)
        close(player->socket_id);
    player->socket_id = 0;
}

int collision(struct point_t cell, struct board_t* board)
{
    if(board->board[cell.x][cell.y] != ' ')
        return 1;

    return 0;
}

void free_coordinates(struct board_t* board, enum error* code, int* x, int* y)
{
    int i=0;
    struct point_t helper;

    srand(time(NULL));

    helper.x = rand() % board->size.x;
    helper.y = rand() % board->size.y;

    while( collision(helper, board) )
    {
        helper.x = rand() % board->size.x;
        helper.y = rand() % board->size.y;

        i++;
        if(i>10000)
        {
            *code = NO_SPACE;
            return;
        }
    }

    *x = helper.x;
    *y = helper.y;

    *code = STATUS_OK;
}

void add_new_coin(struct board_t* board, char ch)
{
    int x;
    int y;
    enum error code;

    free_coordinates(board, &code, &x ,&y);
    if(code == NO_SPACE)
        return;


    switch(ch)
    {
        case 'T':
            *(*(board->board+x)+y) = 'T';
            break;
        case 't':
            *(*(board->board+x)+y) = 't';
            break;
        case 'c':
            *(*(board->board+x)+y) = 'c';
            break;
        default:
            break;
    }
}

int byte_check(long bytes_received, struct board_t* board, struct player_t* player)
{
    if (bytes_received <= 0)
    {
        if(player->bush)
            *(*(board->board+player->location.x)+player->location.y) = '%';
        else
            *(*(board->board+player->location.x)+player->location.y) = ' ';

        set_stock_player(player);

        return 1;
    }

    return 0;
}

void display_player_data(struct player_t* player, int a, int b)
{
    if(player->status != NO_PLAYER)
    {
        mvprintw(b, a, "%d", player->socket_id);
        mvprintw(b+1, a, "HUMAN");
        mvprintw(b+2, a, "%d/%d", player->location.x, player->location.y);
        mvprintw(b+3, a, "%d", player->deaths);
        mvprintw(b+4, a, "%d", player->coins);
        mvprintw(b+5, a, "%d", player->score);
        return;
    }

    mvprintw(b, a, "-");
    mvprintw(b+1, a, "-");
    mvprintw(b+2, a, "--/--");
    mvprintw(b+3, a, "-");
    mvprintw(b+4, a, "-");
    mvprintw(b+5, a, "-");
}

void send_data_for_player(int socket, struct player_t* player)
{
    char int_str[20];
    char temp[20];

    memset(int_str, 0, 20);
    sprintf(int_str, "%d", player->location.x);
    strcpy(temp, int_str);
    strcat(temp, "/");
    sprintf(int_str, "%d", player->location.y);
    strcat(temp, int_str);
    send(socket, temp, 20, 0);

    memset(int_str, 0, 20);
    sprintf(int_str, "%d", player->deaths);
    send(socket, int_str, 20, 0);

    memset(int_str, 0, 20);
    sprintf(int_str, "%d", player->score);
    send(socket, int_str, 20, 0);

    memset(int_str, 0, 20);
    sprintf(int_str, "%d", player->coins);
    send(socket, int_str, 20, 0);
}

void end_game(int server_socket, struct board_t* board)
{
    endwin();
    system("clear");
    printf("\n\n\tThe game has ended! (Server closed)\n\n\n");

    free_board(board->board);
    close(server_socket);
    shutdown(server_socket, SHUT_RDWR);
}

void kill_player_threads(struct board_t* board)
{
    for(int i=0; i<MAX_PLAYERS; i++)
    {
        if(board->player[i].status != NO_PLAYER)
            pthread_cancel(board->player[i].thread);
    }
}

int start_socket(int* server_socket)
{
    struct sockaddr_in serverAddr;

    *server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if(*server_socket < 0)
    {
        printf("Failed to create socket!\n");
        return 1;
    }

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);

    int reuse_port = 1;
    setsockopt(*server_socket, SOL_SOCKET,SO_REUSEADDR, &reuse_port, sizeof(int));

    if(bind(*server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("Failed to bind socket!\n");
        close(*server_socket);
        return 1;
    }

    if ((listen(*server_socket, 5)) != 0)
    {
        printf("Listen failed\n");
        close(*server_socket);
        return 1;
    }

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

struct board_t* create_basic_board(int server_socket)
{
    enum error error_code;
    struct board_t* board = init_board(&error_code);

    if(error_code == ALLOCATION_FAILED || error_code == INCORRECT_INPUT || error_code == NO_FILE)
    {
        printf("Failed to initialize board\n");
        close(server_socket);
        return NULL;
    }

    return board;
}

void spawn_coins_and_beasts(struct board_t* board, void* (*bFun)(void*))
{
    for(int i=0; i<START_COINS_AMOUNT; i++)
    {
        add_new_coin(board, 'c');
        add_new_coin(board, 't');
        add_new_coin(board, 'T');
    }

    for(int i=0; i<START_BEAST_AMOUNT; i++)
        add_beast(board);
}