#include <sys/time.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

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
    this->left_push = false;
    this->right_push = false;
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

    fcntl(this->sock, F_SETFL, O_NONBLOCK);
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

    fcntl(this->sock_gui, F_SETFL, O_NONBLOCK);
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
    //std::cout<<"11\n";
    ssize_t len = receive_datagram(this, buffer, sizeof(buffer)-1);
    //std::cout<<"22\n";
    while (len > 0) {
        //std::cout<<"k\n";
        std::string event = "";
        std::cout<<"Dostalem z serva o size = "<<len<<"\n";
        for (int i = 0; i < len; ++i)
            event += buffer[i];
        this->events_from_server.push_back(event);
        //std::cout<<"Probuje pobrac\n";
        len = receive_datagram(this, buffer, sizeof(buffer)-1);
        //std::cout<<"Pobrane\n";
    }
}

void Player::receive_from_gui()
{
    unsigned char buffer[555];
    ssize_t rcv_len = read(this->sock_gui, buffer, sizeof(buffer) - 1);
    while (rcv_len > 0) {
        std::string message = "";
        for (int i = 0; i < rcv_len-1; ++i)
            message += buffer[i];
        //std::cout<<"Dostalem wiadomosc: "<<message<<", len = "<<message.size()<<", czy rowne LEFT_KEY_UP: "<<(bool)(message=="LEFT_KEY_UP")<<"\n";
        if (message == "LEFT_KEY_DOWN") {
            //syserr("wchodzi\n");
            this->left_push = true;
        }
        else if (message == "LEFT_KEY_UP")
            this->left_push = false;
        else if (message == "RIGHT_KEY_DOWN")
            this->right_push = true;
        else if (message == "RIGHT_KEY_UP")
            this->right_push = false;
        if (this->left_push && !this->right_push)
            this->turn_direction = -1;
        else if (!this->left_push && this->right_push)
            this->turn_direction = 1;
        else
            this->turn_direction = 0;
        rcv_len = read(this->sock_gui, buffer, sizeof(buffer) - 1);
    }
}

std::string get_event(std::string events, int ind)
{
    unsigned char event_len_str[4];
    for (int i = 0; i < 4; ++i)
        event_len_str[i] = events[ind+i];
    uint32_t len = read_4_byte_number(event_len_str);
    std::cout<<"wcz len = "<<len<<"\n";
    std::string event = "";
    for (int i = 0; i < 4+len+4; ++i)
        event += events[ind+i];
    return event;
}

uint32_t get_4_byte_number(std::string str, int from)
{
    unsigned char number_str[4];
    for (size_t i = 0; i < 4; ++i)
        number_str[i] = str[from+i];
    return read_4_byte_number(number_str);
}

int8_t get_1_byte_number(std::string str, int from)
{
    unsigned char number_str[1];
    number_str[0] = str[from];
    return read_1_byte_number(number_str);
}

bool Player::process_event(std::string event)
{
    std::cout<<"caly ev = "<<event.size()<<"\n";
    uint32_t len = get_4_byte_number(event, 0);
    uint32_t event_no = get_4_byte_number(event, 4);
    int8_t event_type = get_1_byte_number(event, 8);
    uint32_t crc = get_4_byte_number(event, 4+len);
    std::cout<<"event len = "<<len<<"\n";
    std::string crc_str = "";
    for (int i = 4; i < 4+len; ++i)
        crc_str += event[i];
    if (crc != calculate_crc32(crc_str) || event_no != this->next_expected_event_no)
        return false;

    if (event_type == 0) {
        uint32_t map_width = get_4_byte_number(event, 9);
        uint32_t map_height = get_4_byte_number(event, 13);
        std::string message = "NEW_GAME " + std::to_string(map_width) + " " + std::to_string(map_height);
        int ind = 17;
        while (ind < 4+len) {
            message += " ";
            std::string name = "";
            while (ind < 4+len && event[ind] != '\0')
                name += event[ind++];
            this->players_name.push_back(name);
            message += name;
            ++ind;
        }
        unsigned char buffer[message.size()+1];
        for (int i = 0; i < message.size(); ++i)
            buffer[i] = message[i];
        buffer[message.size()] = '\0';
        ++this->next_expected_event_no;
        //std::cout<<"Wysylam: \n"<<message<<"\n";
        message += (char)10;
        write(this->sock_gui, message.c_str(), message.size());
        return true;
    }
    else if (event_type == 1) {
        int8_t player_number = get_1_byte_number(event, 9);
        uint32_t player_x = get_4_byte_number(event, 10);
        uint32_t player_y = get_4_byte_number(event, 14);
        std::string message = "PIXEL ";
        message += std::to_string(player_x);
        message += " " + std::to_string(player_y);
        message += " " + this->players_name[player_number];
        ++this->next_expected_event_no;
        //std::cout<<"Wysylam: \n"<<message<<"\n";
        message += (char)10;
        write(this->sock_gui, message.c_str(), message.size());
        return true;
    }
    else if (event_type == 2) {
        int8_t player_number = get_1_byte_number(event, 9);
        std::string message = "PLAYER_ELIMINATED ";
        message += this->players_name[player_number];
        ++this->next_expected_event_no;
        //std::cout<<"Wysylam: \n"<<message<<"\n";
        message += (char)10;
        write(this->sock_gui, message.c_str(), message.size());
        return true;
    }
    else if (event_type == 3) {
        std::string message = "GAME OVER";
        std::cout<<"Wysylam: \n"<<message<<"\n";
        ++this->next_expected_event_no;
        message += (char)10;
        write(this->sock_gui, message.c_str(), message.size());
        return true;
    }
    else {
        std::cout<<"Wysylam: cosniedziala\n";
        return false;
    }

    return true;
}

void Player::send_to_gui()
{
    for (size_t i = 0; i < this->events_from_server.size(); ++i) {
        std::cout<<"noelo\n";
        std::string events = this->events_from_server[i];
        unsigned char game_id_str[4];
        for (int j = 0; j < 4; ++j)
            game_id_str[j] = events[j];
        uint32_t game_id = read_4_byte_number(game_id_str);
        std::cout<<"event size = "<<events.size()<<", game_id = "<<game_id<<"\n";
        for (size_t j = 4; j < events.size(); ++j) {
            std::string event = get_event(events, j);
            std::cout<<"siz = "<<event.size()<<"\n";
            bool event_res = process_event(event);
            if (!event_res)
                break;
            j += event.size()-1;
        }
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