#include <netdb.h>
#include <iostream>

#include "server.h"

#define BUFFER_SIZE   100

unsigned char buffer[BUFFER_SIZE];
struct sockaddr_in client_address;
ssize_t len;

int main(int argc, char *argv[]) {

    Server server(argc, argv);
    server.make_socket();

    for (;;) {
        do {
            server.receive_datagram_from_client(buffer, (size_t)sizeof(buffer), client_address, len);
            //(void) printf("read from socket: %zd bytes: %.*s\n", len,
            //        (int) len, buffer);
                
            server.send_datagram_to_client(&client_address, buffer, len);
                
            Datagram datagram(buffer, len);
            server.push_datagram(datagram);
            std::cout<<"session_id: "<<datagram.session_id<<"\nturn_direction: "<<static_cast<int16_t>(datagram.turn_direction)<<"\nnext_expected_event_no: "<<datagram.next_expected_event_no<<"\nplayer_name: "<<datagram.player_name<<"\n";

        } while (len > 0);
        (void) printf("finished exchange\n");
    }

    return 0;
}