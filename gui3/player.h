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
        uint32_t game_id;
        struct sockaddr_in6 client_address;
        std::vector<std::string> events_from_server;
        std::vector<std::string> players_name;
        bool left_push;
        bool right_push;
        bool active_game;
        uint32_t map_width;
        uint32_t map_height;
        bool did_push;

        Player(int argc, char *argv[]);
        void make_socket();
        void make_gui_socket();
        std::string make_datagram();
        void send_player_datagram();
        void receive_from_server();
        void receive_from_gui();
        void send_to_gui();
        bool process_event(std::string event, uint32_t game_id);
        uint64_t get_time();
        bool time_to_next_round_elapsed();
        void close_socket();
        void close_gui_socket();
};

#endif