#include <netdb.h>
#include <iostream>

#include "server.h"

#define BUFFER_SIZE   100

unsigned char buffer[BUFFER_SIZE];
struct sockaddr_in client_address;
ssize_t len;

// eventy od serva <=> trwa gra, koniec gry czyscimy graczy

int main(int argc, char *argv[]) {

    Server server(argc, argv);
    server.make_socket();

    while (true) {
        server.read_datagrams();

        if (!server.game_is_active && server.can_start_new_game()) {
            server.game_is_active = true;
            server.game = new Game(&server);
        }

        if (server.game_is_active && server.get_time() - server.last_time > server.time_period) {
            server.last_time = server.get_time();
            server.process_clients();
            server.send_events_to_clients();
            if (server.game->check_is_game_over()) {
                delete server.game;
                server.game_is_active = false;
            }
        }

        /*do {
            server.receive_datagram_from_client(buffer, (size_t)sizeof(buffer), client_address, len);                
            server.send_datagram_to_client(&client_address, buffer, len);
                
            Datagram datagram(buffer, len);
            server.push_datagram(datagram);
            std::cout<<"session_id: "<<datagram.session_id<<"\nturn_direction: "<<static_cast<int16_t>(datagram.turn_direction)<<"\nnext_expected_event_no: "<<datagram.next_expected_event_no<<"\nplayer_name: "<<datagram.player_name<<"\n";

        } while (len > 0);*/
    }

    return 0;
}