#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <vector>

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
        int sock_gui;
        uint64_t last_time;
        struct sockaddr_in server_address;
        std::vector<std::string> events_from_server;

        Player(int argc, char *argv[]);
        void make_socket();
        void make_gui_socket();
        std::string make_datagram();
        void send_player_datagram();
        void receive_from_server();
        void receive_from_gui();
        void send_to_gui();
        void process_event(std::string event);
        uint64_t get_time();
        bool time_to_next_round_elapsed();
        void close_socket();
        void close_gui_socket();
};

#endif