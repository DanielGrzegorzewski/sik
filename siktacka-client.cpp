#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <iostream>
#include <cstring>
#include <vector>

#include "err.h"
#include "constans.h"
#include "player.h"
#include "helper.h"

int main(int argc, char *argv[]) 
{
    unsigned char buffer[BUFFER_SIZE];
    Player player(argc, argv);
    player.make_socket();

    unsigned long long timestamp = atoll(argv[3]);
    unsigned char message[9];

    std::string ts = make_message_from_n_byte(timestamp, 8);
    for (int i = 0; i < 8; ++i)
        message[i] = ts[i];
    message[8] = '\0';
    send_datagram(&player, message, 9);

    while (true) {
        receive_datagram(&player, buffer, (size_t)sizeof(buffer));
        (void) printf("%llu\n", read_time(buffer));
    }

    if (close(player.sock) == -1) { 
        syserr("close");
    }
    return 0;
}