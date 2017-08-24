TARGET: echo-server echo-client nums-server nums-client

CC	= cc
CFLAGS	= -Wall -O2
LFLAGS	= -Wall

echo-server.o echo-client.o nums-server.o nums-client.o err.o: err.h

echo-server: echo-server.o err.o
	$(CC) $(LFLAGS) $^ -o $@

echo-client: echo-client.o err.o
	$(CC) $(LFLAGS) $^ -o $@

nums-server: nums-server.o err.o
	$(CC) $(LFLAGS) $^ -o $@

nums-client: nums-client.o err.o
	$(CC) $(LFLAGS) $^ -o $@

.PHONY: clean TARGET
clean:
	rm -f echo-server echo-client nums-server nums-client *.o *~ *.bak
