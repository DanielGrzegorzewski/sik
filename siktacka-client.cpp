#include <netdb.h>
#include <unistd.h>
#include <iostream>

#include "err.h"
#include "player.h"
#include "helper.h"

int BUFFER_SIZE = 100;

int main(int argc, char *argv[]) 
{
    unsigned char buffer[BUFFER_SIZE];
    Player player(argc, argv);
    player.make_socket();
    player.send_player_datagram();

    while (true) {
        receive_datagram(&player, buffer, (size_t)sizeof(buffer));
        (void) printf("dostalem\n");
    }

    if (close(player.sock) == -1) 
        syserr("close");

    return 0;
}