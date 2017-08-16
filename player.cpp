#include <sys/time.h>
#include <netdb.h>
#include <iostream>
#include <cstring>

#include "player.h"
#include "parser.h"
#include "helper.h"
#include "err.h"

uint64_t get_current_timestamp()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((uint64_t)tv.tv_usec)*1000000+tv.tv_usec;
}

Player::Player(int argc, char **argv)
{
	parse_client_arguments(argc, argv, this->player_name, this->game_server_host, this->server_port, this->ui_server_host, this->ui_port);
    this->session_id = get_current_timestamp();
    this->turn_direction = 0;
    this->next_expected_event_no = 0;
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
    send_datagram(this, datagram, datagram_length);
    delete[] datagram;
}

void Player::make_socket()
{
    struct addrinfo *addr_result;
    (void) memset(&(this->addr_hints), 0, sizeof(struct addrinfo));
    this->addr_hints.ai_family = AF_INET; // IPv4
    this->addr_hints.ai_socktype = SOCK_DGRAM;
    this->addr_hints.ai_protocol = IPPROTO_UDP;
    this->addr_hints.ai_flags = 0;
    this->addr_hints.ai_addrlen = 0;
    this->addr_hints.ai_addr = NULL;
    this->addr_hints.ai_canonname = NULL;
    this->addr_hints.ai_next = NULL;

    if (getaddrinfo(this->game_server_host.c_str(), NULL, &this->addr_hints, &addr_result) != 0)
        syserr("getaddrinfo");

    this->my_address.sin_family = AF_INET;
    this->my_address.sin_addr.s_addr =
        ((struct sockaddr_in*) (addr_result->ai_addr))->sin_addr.s_addr;
    this->my_address.sin_port = htons(this->server_port);

    freeaddrinfo(addr_result);

    this->sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (this->sock < 0)
        syserr("socket");
}