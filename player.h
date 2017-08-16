#ifndef _PLAYER_H_
#define _PLAYER_H_

class Player
{
    private:
        uint64_t session_id;
        int8_t turn_direction;
        uint32_t next_expected_event_no;
        std::string player_name;
        std::string game_server_host;
        uint16_t server_port;
        std::string ui_server_host;
        uint16_t ui_port;

    public:
        int sock;
        struct addrinfo addr_hints;
        struct sockaddr_in my_address;

        Player(int argc, char *argv[]);
        void make_socket();
        std::string make_datagram();
        void send_player_datagram();
};

#endif