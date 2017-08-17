TARGET: siktacka-client siktacka-server

SRC=cmd.c err.c net.c read_line.c

CC = gcc

CFLAGS=-std=c99 -Wall -Wunused -DDEBUG

siktacka-client: siktacka-client.cpp
	g++ siktacka-client.cpp err.c player.cpp helper.cpp parser.cpp -o siktacka-client -std=c++11
	
siktacka-server: siktacka-server.cpp
	g++ siktacka-server.cpp err.c helper.cpp server.cpp -o siktacka-server -std=c++11

clean: 
	rm -f *.o gui1 siktacka-client siktacka-server
