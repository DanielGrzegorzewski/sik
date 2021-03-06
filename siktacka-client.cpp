#include <netdb.h>
#include <unistd.h>
#include <iostream>

#include "player.h"
#include "helper.h"
#include "err.h"

// TODO bledy w komunikatach
// TODO powywalac zmienne publiczne, dac gettery i settery

int main(int argc, char *argv[]) 
{
    Player player(argc, argv);
    player.make_socket();
    player.make_gui_socket();
    player.send_player_datagram();

    while (true) {
        std::cout<<"1\n";
        player.receive_from_server();
        std::cout<<"2\n";
        player.receive_from_gui();
        std::cout<<"3\n";
        player.send_to_gui();
        std::cout<<"4\n";

        if (player.time_to_next_round_elapsed()) {
            std::cout<<"5";
            player.send_player_datagram();
            std::cout<<"6\n";
        }
    }

    player.close_socket();
    player.close_gui_socket();
    return 0;
}