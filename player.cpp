#include <sys/time.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <unistd.h>

#include "player.h"
#include "parser.h"
#include "helper.h"
#include "err.h"

uint64_t get_current_timestamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_sec)*1000000+tv.tv_usec;
}

Player::Player(int argc, char **argv)
{
	parse_client_arguments(argc, argv, this->player_name, this->game_server_host, this->server_port, this->ui_server_host, this->ui_port);
    this->session_id = get_current_timestamp();
    this->turn_direction = 0;
    this->next_expected_event_no = 0;
    this->last_time = this->get_time();
}

std::string Player::make_datagram()
{
	std::string datagram = "";
	datagram += make_message_from_n_byte(this->session_id, 8);
	datagram += make_message_from_n_byte(this->turn_direction, 1);
	datagram += make_message_from_n_byte(this->next_expected_event_no, 4);
	datagram += this->player_name;
	return datagram;
}

void Player::send_player_datagram()
{
    std::string datagram_str = this->make_datagram();
    size_t datagram_length = datagram_str.size();
    unsigned char *datagram = new unsigned char[datagram_length];
    for (size_t i = 0; i < datagram_length; ++i)
        datagram[i] = datagram_str[i];
    send_datagram(this, datagram, datagram_length);
    delete[] datagram;
    this->last_time = this->get_time();
}

void Player::make_socket()
{
    struct addrinfo *addr_result;
    struct addrinfo addr_hints;
    (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; // IPv4
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;

    if (getaddrinfo(this->game_server_host.c_str(), NULL, &addr_hints, &addr_result) != 0)
        syserr("getaddrinfo");

    this->server_address.sin_family = AF_INET;
    this->server_address.sin_addr.s_addr =
        ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr;
    this->server_address.sin_port = htons(this->server_port);

    freeaddrinfo(addr_result);

    this->sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (this->sock < 0)
        syserr("socket");
}

void Player::make_gui_socket()
{
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;
    int err;

    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET; // IPv4
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;
    if (getaddrinfo(this->ui_server_host.c_str(), std::to_string(this->ui_port).c_str(), &addr_hints, &addr_result) != 0)
        syserr("getaddrinfo");

    sock_gui = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (sock_gui < 0)
        syserr("socket");

    if (connect(sock_gui, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
        syserr("connect");

    freeaddrinfo(addr_result);
}

uint64_t Player::get_time()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return (uint64_t)tp.tv_sec*1000 + tp.tv_usec/1000;
}

void Player::receive_from_server()
{
    unsigned char buffer[555];
    ssize_t len = receive_datagram(this, buffer, (size_t)(sizeof(buffer) - 1));
    if (len < 0)
        syserr("receive from server");
    while (len > 0) {
        std::string event = "";
        for (int i = 0; i < len; ++i)
            event += buffer[i];
        this->events_from_server.push_back(event);
        len = receive_datagram(this, buffer, (size_t)(sizeof(buffer) - 1));
        if (len < 0)
            syserr("receive from server");
    }
}

void Player::receive_from_gui()
{
    unsigned char buffer[555];
    ssize_t rcv_len = read(this->sock_gui, buffer, sizeof(buffer) - 1);
    if (rcv_len < 0)
        syserr("receive from gui");
    while (rcv_len > 0) {
        std::string message = "";
        for (int i = 0; i < rcv_len; ++i)
            message += buffer[i];
        if (message == "LEFT_KEY_DOWN") {

        }
        else if (message == "LEFT_KEY_UP") {

        }
        else if (message == "RIGHT_KEY_DOWN") {

        }
        else if (message == "RIGHT_KEY_UP") {

        }
        rcv_len = read(this->sock_gui, buffer, sizeof(buffer) - 1);
        if (rcv_len < 0)
            syserr("receive from gui");
    }
}

void Player::send_to_gui()
{
    for (size_t i = 0; i < this->events_from_server.size(); ++i) {
        std::string event = this->events_from_server[i];
        uint32_t game_id = 
    }
    this->events_from_server.clear();
}

bool Player::time_to_next_round_elapsed()
{
    return (this->get_time() - this->last_time) > 20;
}

void Player::close_socket()
{
    if (close(this->sock) == -1) 
        syserr("close socket");
}

void Player::close_gui_socket()
{
    if (close(this->sock_gui) == -1)
        syserr("close gui socket");
}