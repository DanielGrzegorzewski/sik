TARGET: siktacka-client siktacka-server gui1

SRC=cmd.c err.c net.c read_line.c

CC = gcc

CFLAGS=-std=c99 -Wall -Wunused -DDEBUG

siktacka-client: siktacka-client.cpp err.c
	g++ siktacka-client.cpp err.c -o siktacka-client -std=c++11
	
siktacka-server: siktacka-server.cpp err.c
	g++ siktacka-server.cpp err.c -o siktacka-server -std=c++11
	
gui1: gui1.c $(SRC) gui.h 
	$(CC) $(CFLAGS) gui1.c $(SRC) -o gui1 `pkg-config gtk+-2.0 --cflags --libs`

clean: 
	rm -f *.o gui1 siktacka-client siktacka-server
