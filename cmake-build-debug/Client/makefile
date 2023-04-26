# List your *.h files (if you do not have them in your project then leave the variable "headers" empty):
headers = client_defs.h

# List your *.c files:
sources = client.c client_defs.c

# Specify name of your program:
executable = client

$(executable): $(headers) $(sources)
	gcc -g -Wall -pedantic -lncurses -lpthread $(sources) -o $(executable)

.PHONY: clean
clean:
	rm $(executable)

.PHONY: check
check: $(executable)
	valgrind --leak-check=full --track-origins=yes ./$(executable)