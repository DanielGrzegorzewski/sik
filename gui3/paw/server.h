#ifndef _SERVER_H_
#define _SERVER_H_

#include <vector>
#include <set>

class Datagram
{
    public:
        uint64_t session_id;
        int8_t turn_direction;
        uint32_t next_expected_event_no;
        std::string player_name;
        bool correct_datagram;

        Datagram(unsigned char *buffer, size_t len);
};

class Event
{
    private:
        uint32_t event_type;
        std::string event_data;

    public:
        Event(uint32_t event_type);
        void create_event_new_game(uint32_t maxx, uint32_t maxy, std::vector<std::string> &players_name);
        void create_event_pixel(uint8_t player_number, uint32_t x, uint32_t y);
        void create_event_player_eliminated(uint8_t player_number);
        std::string to_string(uint32_t event_number);
};

class Server;

class Client
{
    public:
        Server *server;
        uint64_t session_id;
        struct sockaddr_in6 client_address;
        double head_x;
        double head_y;
        int direction;
        std::string client_name;
        bool alive;
        int8_t turn_direction;
        uint32_t next_expected_event_no;

        Client(Server *server, struct sockaddr_in6, uint64_t session_id, std::string client_name, int8_t turn_direction, uint32_t next_expected_event_no);
};

bool cmp(Client &client1, Client &client2);

class Game
{
    private:
        std::vector<Event> events;

    public:
        uint32_t game_id;
        Server *server;
        bool is_game_active;
        std::set<std::pair<uint32_t, uint32_t> > occupied_pixels;

        Game(Server *server);
        bool check_is_game_over();
};

class Server
{
    private:
        std::vector<Datagram> datagrams;
        uint16_t server_port;
        uint32_t game_speed;
        uint32_t turn_speed;
        uint32_t seed;

    public:
        uint32_t map_width;
        uint32_t map_height;
        int sock;
        uint64_t last_time;
        struct sockaddr_in6 server_address;
        bool get_random_first_call;
        bool game_is_active;
        uint64_t time_period;
        std::vector<Client> clients;
        std::vector<Event> events;
        Game *game;

        Server(int argc, char *argv[]);
        void make_socket();
        ssize_t receive_datagram_from_client(unsigned char *datagram, int len, struct sockaddr_in6 &srvr_address, ssize_t &rcv_len);
        void send_datagram_to_client(struct sockaddr_in6 *client_address, unsigned char *datagram, int len);
        void read_datagrams();
        void send_event(int event_id);
        void process_client(int ind);
        void process_clients();
        void send_events_to_client(int ind);
        uint64_t get_random();
        uint64_t get_time();
        bool check_collision(int x, int y);
        void add_client(Client client);
        int client_index_from_alives(int ind);
        int find_index_of_client(struct sockaddr_in6 client_address, uint64_t session_id);
        bool can_start_new_game();
        void start_new_game();
        bool time_to_next_round_elapsed();
        void close_socket();
        void game_over();
};


#endif