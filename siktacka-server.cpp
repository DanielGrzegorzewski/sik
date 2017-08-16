#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include "err.h"
#include "helper.h"

#define BUFFER_SIZE   100
#define PORT_NUM     12345

int main(int argc, char *argv[]) {
    int sock;
    int flags, sflags;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;

    unsigned char buffer[BUFFER_SIZE];
    socklen_t snda_len, rcva_len;
    ssize_t len, snd_len;

    sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
    if (sock < 0)
        syserr("socket");
    // after socket() call; we should close(sock) on any execution path;
    // since all execution paths exit immediately, sock would be closed when program terminates

    server_address.sin_family = AF_INET; // IPv4
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // listening on all interfaces
    server_address.sin_port = htons(PORT_NUM); // default port for receiving is PORT_NUM

    // bind the socket to a concrete address
    if (bind(sock, (struct sockaddr *) &server_address,
            (socklen_t) sizeof(server_address)) < 0)
        syserr("bind");

    snda_len = (socklen_t) sizeof(client_address);
    for (;;) {
        do {
            rcva_len = (socklen_t) sizeof(client_address);
            flags = 0; // we do not request anything special
            len = recvfrom(sock, buffer, sizeof(buffer), flags,
                    (struct sockaddr *) &client_address, &rcva_len);
            if (len < 0)
                syserr("error on datagram from client socket");
            else {
                (void) printf("read from socket: %zd bytes: %.*s\n", len,
                        (int) len, buffer);
                sflags = 0;
                buffer[22] = '\0';

                //snd_len = sendto(sock, buffer, (size_t) len, sflags,
                //        (struct sockaddr *) &client_address, snda_len);
                //if (snd_len != len)
                //    syserr("error on sending datagram to client socket");
                //bool parse_datagram(unsigned char *datagram, uint64_t &session_id, int8_t &turn_direction, uint32_t &next_expected_event_no, std::string &player_name)
                uint64_t session_id;
                int8_t turn_direction;
                uint32_t next_expected_event_no;
                std::string player_name;
                parse_datagram(buffer, len, session_id, turn_direction, next_expected_event_no, player_name);
                std::cout<<"session_id: "<<session_id<<"\nturn_direction: "<<static_cast<int16_t>(turn_direction)<<"\nnext_expected_event_no: "<<next_expected_event_no<<"\nplayer_name: "<<player_name<<"\n";
            }
        } while (len > 0);
        (void) printf("finished exchange\n");
    }

    return 0;
}