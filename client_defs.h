#ifndef SERVER_C_CLIENT_DEFS_H
#define SERVER_C_CLIENT_DEFS_H

#define PLAYER_PAIR 1
#define PATH_PAIR 2
#define WALL_PAIR 3
#define CAMPSITE_PAIR 4
#define COIN_PAIR 5
#define BEAST_PAIR 6
#define BUSHES_PAIR 7
#define DROPPED_PAIR 8

#define PORT 8989


int create_connection(int* client_socket, char* join, int* connected);
void start_ncurses();
void end_game(int client_socket, int connected);
void show_informations(int a, int w, char* coordinates, char* deaths, char* coins_found, char* coins_brought, int nr);
void show_legend(int a, int w);
void write_map();
void recv_check(long bytes, int* connected);

#endif //SERVER_C_CLIENT_DEFS_H
