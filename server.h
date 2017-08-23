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

        Datagram(unsigned char *buffer, size_t len);
};

/*
– NEW_GAME
      event_type: 0
      event_data:
         maxx (4 bajty, szerokość planszy w pikselach, liczba bez znaku)
         maxy (4 bajty, wysokość planszy w pikselach, liczba bez znaku)
         Następnie lista nazw graczy zawierająca dla każdego z graczy:
           player_name (jak w punkcie „4. Komunikaty od klienta do serwera”)
           znak '\0'

    – PIXEL
      event_type: 1
      event_data:
         player_number (1 bajt)
         x (4 bajty, odcięta, liczba bez znaku)
         y (4 bajty, rzędna, liczba bez znaku)

    – PLAYER_ELIMINATED
      event_type: 2
      event_data:
         player_number (1 bajt)

    – GAME_OVER
      event_type: 3
      event_data: brak
*/

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
};

/*
numer partii (game_id), wysyłany w każdym wychodzącym datagramie,
   – bieżące współrzędne głowy węża każdego z graczy oraz kierunek jego ruchu
     (jako liczby zmiennoprzecinkowe o co najmniej podwójnej precyzji),
   – zdarzenia wygenerowane od początku gry (patrz punkt „6. Rekordy opisujące
     zdarzenia” oraz dalej),
   – zajęte piksele planszy.
   */

class Server;

class Client
{
    public:
        Server *server;
        uint64_t session_id;
        struct sockaddr_in client_address;
        double head_x;
        double head_y;
        double direction;
        std::string client_name;
        bool alive;
        int8_t turn_direction;
        uint32_t next_expected_event_no;

        Client(Server *server, struct sockaddr_in, uint64_t session_id, std::string client_name, int8_t turn_direction, uint32_t next_expected_event_no);
};

bool cmp(Client &client1, Client &client2);

class Game
{
    private:
        uint32_t game_id;
        std::vector<Event> events;
        std::set<std::pair<uint32_t, uint32_t> > occupied_pixels;

    public:
        Server *server;
        bool is_game_active;

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
        struct sockaddr_in server_address;
        bool get_random_first_call;
        bool game_is_active;
        uint64_t time_period;
        std::vector<Client> clients;
        Game *game;

        Server(int argc, char *argv[]);
        void make_socket();
        ssize_t receive_datagram_from_client(unsigned char *datagram, int len, struct sockaddr_in &srvr_address, ssize_t &rcv_len);
        void send_datagram_to_client(struct sockaddr_in *client_address, unsigned char *datagram, int len);
        void read_datagrams();
        void process_client();
        void process_clients();
        void send_events_to_clients();
        uint64_t get_random();
        uint64_t get_time();
        void add_client(Client client);
        int find_index_of_client(struct sockaddr_in client_address, uint64_t session_id);
        bool can_start_new_game();
        void start_new_game();
};


#endif