#ifndef _SERVER_H_
#define _SERVER_H_

#include <vector>

#include "helper.h"

class Server
{
    private:
        std::vector<Datagram> datagrams;
        uint16_t server_port;

    public:
        int sock;
        struct sockaddr_in server_address;

        Server(int argc, char *argv[]);
        void make_socket();
};

#endif