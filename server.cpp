#include <netdb.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>

#include "err.h"
#include "server.h"
#include "constans.h"
#include "helper.h"

uint64_t read_8_byte_number(unsigned char *buffer)
{
    uint64_t result = 0;
    for (size_t i = 0; i < 8; ++i)
        result += buffer[i]*(1LL<<(8*(7-i)));
    return result;
}

int8_t read_1_byte_number(unsigned char *buffer)
{
    int8_t result = buffer[0];
    return result;
}

uint32_t read_4_byte_number(unsigned char *buffer)
{
    uint32_t result = 0;
    for (size_t i = 0; i < 4; ++i)
        result += buffer[i]*(1LL<<(8*(3-i)));
    return result;
}

std::string get_string_from_datagram(unsigned char *datagram, uint8_t from, uint8_t to)
{
    std::string result_string = "";
    for (int i = from; i <= to; ++i)
        result_string += datagram[i];
    return result_string;
}

// datagram size 100, zal: >= 13, < 100, sprawdzic dla == 100 (>=100 tez) czy if bedzie dzialal (Wyslac dlugie imie)
bool parse_datagram(unsigned char *datagram, uint32_t len, uint64_t &session_id, int8_t &turn_direction, uint32_t &next_expected_event_no, std::string &player_name)
{
    unsigned char session_id_str[8];
    for (size_t i = 0; i < 8; ++i)
        session_id_str[i] = datagram[i];

    unsigned char turn_direction_str[1];
    turn_direction_str[0] = datagram[8];

    unsigned char next_expected_event_no_str[4];
    for (size_t i = 9; i < 13; ++i)
        next_expected_event_no_str[i-9] = datagram[i];

    session_id = read_8_byte_number(session_id_str);
    turn_direction = read_1_byte_number(turn_direction_str);
    next_expected_event_no = read_4_byte_number(next_expected_event_no_str);
    player_name = get_string_from_datagram(datagram, 13, len-1);

    if (player_name.size() > 64)
        return false;

    for (size_t i = 0; i < player_name.size(); ++i)
        if (player_name[i] < 33 || player_name[i] > 126)
            return false;

    return true;
}

Datagram::Datagram(unsigned char *buffer, size_t len)
{
    parse_datagram(buffer, len, this->session_id, this->turn_direction, this->next_expected_event_no, this->player_name);
}

Event::Event(uint32_t event_type)
{
    this->event_type = event_type;
    this->event_data = "";
}

void Event::create_event_new_game(uint32_t maxx, uint32_t maxy, std::vector<std::string> &players_name)
{
    this->event_data += make_message_from_n_byte(maxx, 4);
    this->event_data += make_message_from_n_byte(maxy, 4);
    for (std::string str: players_name)
        this->event_data += str + '\0';
}

void Event::create_event_pixel(uint8_t player_number, uint32_t x, uint32_t y)
{
    this->event_data += make_message_from_n_byte(player_number, 1);
    this->event_data += make_message_from_n_byte(x, 4);
    this->event_data += make_message_from_n_byte(y, 4);
}

void Event::create_event_player_eliminated(uint8_t player_number)
{
    this->event_data += make_message_from_n_byte(player_number, 1);
}

Client::Client(Server *server, struct sockaddr_in client_address, uint64_t session_id, std::string client_name, int8_t turn_direction, uint32_t next_expected_event_no)
{
    this->server = server;
    this->client_address = client_address;
    this->session_id = session_id;
    this->client_name = client_name;
    this->alive = false;
    this->head_x = ((server->get_random())%server->map_width) + 0.5;
    this->head_y = ((server->get_random())%server->map_height) + 0.5;
    this->direction = (server->get_random())%360;
    this->turn_direction = turn_direction;
    this->next_expected_event_no = next_expected_event_no;
}

Game::Game(Server *server)
{
    this->game_id = server->get_random();
    this->server = server;
    this->is_game_active = false;
}

bool Game::check_is_game_over()
{
    uint32_t active_clients = 0;
    for (size_t i = 0; i < server->clients.size(); ++i)
        if (server->clients[i].alive)
            ++active_clients;
    return active_clients <= 1;
}

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

// Konwertuje ciag znakow na liczbe jezeli owa jest liczba
// mniejsza od 2^20, w.p.p zwraca 2^30
uint64_t get_int(char *buffer)
{
    uint64_t result = 1<<30;
    int len = strlen(buffer);
    if (len >= 7)
        return result;
    for (int i = 0; i < len; ++i)
        if (buffer[i] < '0' || buffer[i] > '9')
            return result;
    result = (uint64_t)atoi(buffer);
    return result;
}

bool check_port(uint32_t port)
{
    if (port < MIN_PORT_VALUE || port > MAX_PORT_VALUE)
        return false;
    return true;
}

Server::Server(int argc, char *argv[])
{
    this->map_width = DEFAULT_MAP_WIDTH;
    this->map_height = DEFAULT_MAP_HEIGHT;
    this->server_port = atoi(DEFAULT_SERVER_PORT);
    this->game_speed = DEFAULT_GAME_SPEED;
    this->turn_speed = DEFAULT_TURN_SPEED;
    this->seed = time(NULL);
    this->get_random_first_call = true;
    this->game_is_active = false;
    this->last_time = this->get_time();

    int opt;
    while ((opt = getopt(argc, argv, "W:H:p:s:t:r:")) != -1) {
        uint32_t num = get_int(optarg);
        if (num == (1<<30))
            syserr("Wrong call parameter(s)");
        switch (opt) {
            case 'W':
                this->map_width = num;
                break;
            case 'H':
                this->map_height = num;
                break;
            case 'p':
                if (!check_port(num))
                    syserr("Wrong port parameter");
                this->server_port = num;
                break;
            case 's':
                this->game_speed = num;
                break;
            case 't':
                this->turn_speed = num;
                break;
            case 'r':
                this->seed = num;
                break;
            default: /* '?' */
                syserr("Usage: %s [-W n] [-H n] [-p n] [-s n] [-t n] [-r n]", argv[0]);
        }
    }
    this->time_period = 1000/(this->game_speed);
}

uint64_t Server::get_random()
{
    static uint32_t r = 0;
    if (this->get_random_first_call) {
        this->get_random_first_call = false;
        return r = this->seed;
    }
    return r = ((uint64_t)r * 279470273) % 4294967291;
}

void Server::receive_datagram_from_client(unsigned char *datagram, int len, struct sockaddr_in &srvr_address, ssize_t &rcv_len)
{
    int flags = 0;
    socklen_t rcva_len;

    memset(datagram, 0, sizeof(datagram));
    rcva_len = (socklen_t) sizeof(srvr_address);
    rcv_len = recvfrom(this->sock, datagram, len, flags,
        (struct sockaddr *) &srvr_address, &rcva_len);
    if (rcv_len < 0)
        syserr("error on datagram from client socket");
}

void Server::send_datagram_to_client(struct sockaddr_in *client_address, unsigned char *datagram, int len)
{
    int sflags = 0;
    socklen_t snda_len;
    ssize_t snd_len;

    snda_len = (socklen_t) sizeof(*client_address);
    snd_len = sendto(this->sock, datagram, (size_t) len, sflags,
            (struct sockaddr *) client_address, snda_len);
    if (snd_len != len)
        syserr("error on sending datagram to client socket");
}

bool Server::read_datagrams()
{

}

void Server::process_clients()
{

}

void Server::send_events_to_clients()
{

}

uint64_t Server::get_time()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (uint64_t)tp.tv_sec*1000 + tp.tv_usec/1000;
}

int Server::find_index_of_client(struct sockaddr_in client_address, uint64_t session_id)
{
    for (size_t i = 0; i < (this->clients.size()); ++i) {
        Client client = this->clients[i];
        if (client.session_id == session_id && memcmp(&client.client_address, &client_address, sizeof(client_address)) == 0)
            return i;
    }
    return -1;
}

bool Server::can_start_new_game()
{
    size_t ready_clients = 0;
    for (size_t i = 0; i < this->clients.size(); ++i)
        if (this->clients[i].client_name.size() > 0 && this->clients[i].turn_direction != 0)
            ++ready_clients;
    return ready_clients >= 2;
}

void Server::start_new_game()
{
    for (size_t i = 0; i < this->clients.size(); ++i)
        if (this->clients[i].client_name.size() > 0 && this->clients[i].turn_direction != 0)
            this->clients[i].alive = true;
}