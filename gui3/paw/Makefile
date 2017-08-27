TARGET: siktacka-server siktacka-client

SRC=err.c
CC = g++
CFLAGS=-std=c++14 -Wall -O2



siktacka-server: siktacka-server.cpp $(SRC)
	$(CC) $(CFLAGS) siktacka-server.cpp $(SRC) -o siktacka-server

siktacka-client: siktacka-client.cpp $(SRC)
	$(CC) $(CFLAGS) siktacka-client.cpp $(SRC) -o siktacka-client

clean:
	rm -f *.o siktacka-server siktacka-client