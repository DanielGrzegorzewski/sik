#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>

#include "err.h"
#include "server.h"
#include "helper.h"

#define BUFFER_SIZE   100
#define PORT_NUM     12345

int main(int argc, char *argv[]) {

    Server server(argc, argv);
    server.make_socket();

    int flags, sflags;
    struct sockaddr_in client_address;

    unsigned char buffer[BUFFER_SIZE];
    socklen_t snda_len, rcva_len;
    ssize_t len, snd_len;

    for (;;) {
        do {
            rcva_len = (socklen_t) sizeof(client_address);
            flags = 0; // we do not request anything special
            len = recvfrom(server.sock, buffer, sizeof(buffer), flags,
                    (struct sockaddr *) &client_address, &rcva_len);
            if (len < 0)
                syserr("error on datagram from client socket");
            else {
                (void) printf("read from socket: %zd bytes: %.*s\n", len,
                        (int) len, buffer);
                
                //snda_len = (socklen_t) sizeof(client_address);
                //sflags = 0;
                //snd_len = sendto(sock, buffer, (size_t) len, sflags,
                //        (struct sockaddr *) &client_address, snda_len);
                //if (snd_len != len)
                //    syserr("error on sending datagram to client socket");
                
                Datagram datagram(buffer, len);
                std::cout<<"session_id: "<<datagram.session_id<<"\nturn_direction: "<<static_cast<int16_t>(datagram.turn_direction)<<"\nnext_expected_event_no: "<<datagram.next_expected_event_no<<"\nplayer_name: "<<datagram.player_name<<"\n";
            }
        } while (len > 0);
        (void) printf("finished exchange\n");
    }

    return 0;
}