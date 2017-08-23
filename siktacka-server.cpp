#include <netdb.h>
#include <iostream>

#include "server.h"

#define BUFFER_SIZE   100

unsigned char buffer[BUFFER_SIZE];
ssize_t len;

int main(int argc, char *argv[]) {

    Server server(argc, argv);
    server.make_socket();

    while (true) {
        server.read_datagrams();

        if (!server.game_is_active && server.can_start_new_game()) {
            server.game_is_active = true;
            server.game = new Game(&server);
            server.start_new_game();
        }

        if (server.game_is_active && server.get_time() - server.last_time > server.time_period) {
            server.last_time = server.get_time();
            server.process_clients();
            if (server.game->check_is_game_over()) {
                delete server.game;
                server.game_is_active = false;
                Event event(3);
                server.events.push_back(event);
            }
            server.send_events_to_clients();
        }
    }

    return 0;
}