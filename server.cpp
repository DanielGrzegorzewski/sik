#include <netdb.h>
#include <iostream>
#include <cstring>

#include "err.h"
#include "server.h"
#include "constans.h"

void Server::make_socket()
{
    this->sock = socket(AF_INET, SOCK_DGRAM, 0); // creating IPv4 UDP socket
    if (this->sock < 0)
        syserr("socket");

    this->server_address.sin_family = AF_INET; // IPv4
    this->server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    this->server_address.sin_port = htons(this->server_port);

    if (bind(this->sock, (struct sockaddr *) &(this->server_address),
            (socklen_t) sizeof(server_address)) < 0)
        syserr("bind");
}

Server::Server(int argc, char *argv[])
{
    this->server_port = atoi(DEFAULT_SERVER_PORT);
}