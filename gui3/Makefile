TARGET: siktacka-client siktacka-server

siktacka-client: siktacka-client.cpp
	g++ siktacka-client.cpp err.c player.cpp helper.cpp parser.cpp -o siktacka-client -O2 -Wall -std=c++11
	
siktacka-server: siktacka-server.cpp
	g++ siktacka-server.cpp err.c helper.cpp server.cpp -o siktacka-server -O2 -Wall -std=c++11

clean: 
	rm -f *.o siktacka-client siktacka-server
