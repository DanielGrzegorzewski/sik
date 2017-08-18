#ifndef _SERVER_H_
#define _SERVER_H_

#include <vector>

class Datagram
{
    public:
        uint64_t session_id;
        int8_t turn_direction;
        uint32_t next_expected_event_no;
        std::string player_name;

        Datagram(unsigned char *buffer, size_t len);
};

class Server
{
    private:
        std::vector<Datagram> datagrams;
        int map_width;
        int map_height;
        uint16_t server_port;
        int game_speed;
        int turn_speed;
        int seed;

    public:
        int sock;
        struct sockaddr_in server_address;
        bool get_random_first_call;

        Server(int argc, char *argv[]);
        void make_socket();
        void receive_datagram_from_client(unsigned char *datagram, int len, struct sockaddr_in &srvr_address, ssize_t &rcv_len);
        void send_datagram_to_client(struct sockaddr_in *client_address, unsigned char *datagram, int len);
        void push_datagram(Datagram datagram);
        uint64_t get_random();
};

#endif