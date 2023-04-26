#ifndef PROJEKT_1_SO2_DEFS_H
#define PROJEKT_1_SO2_DEFS_H

#include <stdlib.h>
#include <pthread.h>

#define PORT 8989

#define MAX_PLAYERS 4
#define MAX_BEASTS 10

#define START_COINS_AMOUNT 10
#define START_BEAST_AMOUNT 2

#define TOUR_TIME 500000

#define PLAYER_PAIR 1
#define PATH_PAIR 2
#define WALL_PAIR 3
#define CAMPSITE_PAIR 4
#define COIN_PAIR 5
#define BEAST_PAIR 6
#define BUSHES_PAIR 7
#define DROPPED_PAIR 8

#define DISABLE_INSTANT_MOVE 1

enum status { UP=0, DOWN, LEFT, RIGHT, NO_MOVE, NO_PLAYER, NO_BEAST};
enum error { NO_SPACE, ALLOCATION_FAILED, NO_FILE, INCORRECT_INPUT, STATUS_OK };
enum action {COIN, CAMPSITE, BUSH, DROPPED_CHEST, PLAYER_COLLISION, NOTHING};

struct point_t{
    int x;
    int y;
};

struct player_t{
    struct point_t location;

    int coins;
    int score;
    int deaths;
    int bush;

    enum status status;
    char arr[37];

    int socket_id;
    pthread_t thread;
};

struct beast_t{
    struct point_t location;
    struct point_t target_loc;

    int bush;
    enum status status;
    char ch;
    int collision_flag;

    int chase_flag;

    pthread_t thread;
};

struct board_t{
    struct point_t size;
    char** board;
    int dropped[100][100];

    struct player_t player[MAX_PLAYERS];
    struct beast_t beast[MAX_BEASTS];
};

void init_player(struct player_t* player, int x, int y);
struct board_t* init_board(enum error* code);

void draw_player_board(struct board_t* board, struct player_t* player);

void free_board(char** board);
char** read_board(enum error* code, int* x, int* y);

void display_board_server(struct board_t* board);

int collision(struct point_t cell, struct board_t* board);
void free_coordinates(struct board_t* board, enum error* code, int* x, int* y);
void add_new_coin(struct board_t* board, char ch);

void move_player(struct board_t* board, struct player_t* player);
int is_coin(struct point_t pkt, struct board_t* board);
int is_campsite(struct point_t pkt, struct board_t* board);
int is_bush(struct point_t pkt, struct board_t* board);
int is_dropped(struct point_t pkt, struct board_t* board);
int is_player(struct point_t pkt, struct board_t* board);

enum action action(int* flag, struct board_t* board, struct point_t temp);
void coin_manager(struct player_t* player, struct board_t* board, struct point_t temp, int flag);
void campsite_manager(struct player_t* player);
void bush_manager(struct player_t* player, struct board_t* board, struct point_t temp);
void dropped_chest_manager(struct board_t* board, struct point_t temp, struct player_t* player, int* bush_flag);
void player_collision_manager(struct player_t* player, struct board_t* board, struct point_t temp, int flag);

void update_player_move(struct player_t* player, struct board_t* board, enum status status);
void set_stock_player(struct player_t* player);

void show_legend(int a, int w);
int byte_check(long bytes_received, struct board_t* board, struct player_t* player);
void display_player_data(struct player_t* player, int a, int b);
void send_data_for_player(int socket, struct player_t* player);
void end_game(int server_socket, struct board_t* board);
void display_server_stats(int a, long round, int server_socket, struct board_t* board);
int start_socket(int* server_socket);
void start_ncurses();
struct board_t* create_basic_board(int server_socket);
void add_beast(struct board_t* board);
void move_beast(struct beast_t* beast);
int beast_collisions(struct point_t cell, struct board_t* board, enum action* action);
void kill_player_threads(struct board_t* board);
void beast_catch(struct board_t* board, char* id);
void spawn_coins_and_beasts(struct board_t* board, void* (*bFun)(void*));
void scan_for_player(struct board_t* board, struct beast_t* beast);
int beast_scan_status(int x, int y, struct board_t* board, struct beast_t* beast);


void* beast(void* arg);
void* game_loop(void* arg);
void* keyboard_handler(void* arg);
void* handle_client_connection(void* arg);
void* wait_for_client(void* arg);

#endif //PROJEKT_1_SO2_DEFS_H
