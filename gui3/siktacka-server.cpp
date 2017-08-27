#include <netdb.h>
#include <unistd.h>
#include <iostream>

#include "server.h"
#include "err.h"

int main(int argc, char *argv[]) {

    Server server(argc, argv);
    server.make_socket();

    while (true) {
        server.read_datagrams();
        if (!server.game_is_active && server.can_start_new_game())
            server.start_new_game();
        if (server.time_to_next_round_elapsed()) {
            //std::cout<<"zaraz process_clients\n";
            server.process_clients();
            //std::cout<<"po processie\n";
            if (server.game->check_is_game_over())
                server.game_over();
        }
    }

    server.close_socket();
    return 0;
}