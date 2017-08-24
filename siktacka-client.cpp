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
        player.receive_from_server();
        player.receive_from_gui();
        player.send_to_gui();

        if (player.time_to_next_round_elapsed())
            player.send_player_datagram();
    }

    player.close_socket();
    player.close_gui_socket();
    return 0;
}