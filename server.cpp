#include <netdb.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

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
    this->map_width = DEFAULT_MAP_WIDTH;
    this->map_height = DEFAULT_MAP_HEIGHT;
    this->server_port = atoi(DEFAULT_SERVER_PORT);
    this->game_speed = DEFAULT_GAME_SPEED;
    this->turn_speed = DEFAULT_TURN_SPEED;
    this->seed = time(NULL);
    this->get_random_first_call = true;

    int opt;
    // TODO dodaj parsowanie, sprawdzanie danych
    while ((opt = getopt(argc, argv, "WHpstr:")) != -1) {
        switch (opt) {
            case 'W':
                this->map_width = atoi(optarg);
                break;
            case 'H':
                this->map_height = atoi(optarg);
                break;
            case 'p':
                this->server_port = atoi(optarg);
                break;
            case 's':
                this->game_speed = atoi(optarg);
                break;
            case 't':
                this->turn_speed = atoi(optarg);
                break;
            case 'r':
                this->seed = atoi(optarg);
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                        argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

uint64_t Server::get_random()
{
    static int r = 0;
    if (this->get_random_first_call) {
        this->get_random_first_call = false;
        return r = this->seed;
    }
    return r = ((uint64_t)r * 279470273) % 4294967291;
}

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

void Server::receive_datagram_from_client(unsigned char *datagram, int len, struct sockaddr_in &srvr_address, ssize_t &rcv_len)
{
    int flags = 0;
    socklen_t rcva_len;

    memset(datagram, 0, sizeof(datagram));
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

    snda_len = (socklen_t) sizeof(client_address);
    snd_len = sendto(this->sock, datagram, (size_t) len, sflags,
            (struct sockaddr *) client_address, snda_len);
    if (snd_len != len)
        syserr("error on sending datagram to client socket");
}

void Server::push_datagram(Datagram datagram)
{
    this->datagrams.push_back(datagram);
}