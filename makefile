executable_server = server
executable_client = client

all: Client Server

Client: client.c
	gcc -g -Wall -pedantic -lncurses -lpthread client.c client_defs.c -o $(executable_client)

Server: server.c defs.c defs.h
	gcc -g -Wall -pedantic -lncurses -lpthread server.c defs.c -o $(executable_server)

.PHONY: clean
clean:
	rm $(executable_server)
	rm $(executable_client)