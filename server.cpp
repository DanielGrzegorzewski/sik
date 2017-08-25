#include <netdb.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <algorithm>
#include <cmath>
#include <fcntl.h>

#include "err.h"
#include "server.h"
#include "constans.h"
#include "helper.h"

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
    this->correct_datagram = parse_datagram(buffer, len, this->session_id, this->turn_direction, this->next_expected_event_no, this->player_name);
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

std::string Event::to_string(uint32_t event_number)
{
    std::string result = "";
    result += make_message_from_n_byte(event_number, 4);
    result += make_message_from_n_byte(this->event_type, 1);
    result += this->event_data;
    uint32_t crc = calculate_crc32(result);
    result = make_message_from_n_byte(result.size(), 4) + result;
    result += make_message_from_n_byte(crc, 4);
    return result;
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

bool cmp(Client &client1, Client &client2)
{
    return client1.client_name < client2.client_name;
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
        if (this->server->clients[i].alive)
            ++active_clients;
    return active_clients <= 0;
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

    fcntl(sock, F_SETFL, O_NONBLOCK);
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

ssize_t Server::receive_datagram_from_client(unsigned char *datagram, int len, struct sockaddr_in &srvr_address, ssize_t &rcv_len)
{
    int flags = 0;
    socklen_t rcva_len;

    memset(datagram, 0, sizeof(datagram));
    rcva_len = (socklen_t) sizeof(srvr_address);
    rcv_len = recvfrom(this->sock, datagram, len, flags,
        (struct sockaddr *) &srvr_address, &rcva_len);
    return rcv_len;
    //if (rcv_len < 0)
    //    syserr("error on datagram from client socket");
}

void Server::send_datagram_to_client(struct sockaddr_in *client_address, unsigned char *datagram, int len)
{
    int sflags = 0;
    socklen_t snda_len;
    ssize_t snd_len;

    snda_len = (socklen_t) sizeof(*client_address);
    std::cout<<"len = "<<len<<"\n";
    snd_len = sendto(this->sock, datagram, (size_t) len, sflags,
            (struct sockaddr *) client_address, snda_len);
    std::cout<<"snd_len "<<snd_len<<", len = "<<len<<"\n";
    if (snd_len != len)
        syserr("error on sending datagram to client socket");
}

void Server::read_datagrams()
{
    unsigned char buffer[SERVER_BUFFER_SIZE];
    struct sockaddr_in client_address;
    ssize_t len = 0;

    while (len > -1) {
        len = this->receive_datagram_from_client(buffer, (size_t)sizeof(buffer), client_address, len);
        Datagram datagram(buffer, len);
        if (!datagram.correct_datagram)
            continue;
        int ind = this->find_index_of_client(client_address, datagram.session_id);
        if (ind == -1) {
            Client client(this, client_address, datagram.session_id, datagram.player_name, datagram.turn_direction, datagram.next_expected_event_no);
            this->add_client(client);
            ind = this->find_index_of_client(client_address, datagram.session_id);
        }
        else {
            this->clients[ind].turn_direction = datagram.turn_direction;
            this->clients[ind].next_expected_event_no = datagram.next_expected_event_no;
        }
        this->send_events_to_client(ind);
    }
}

int Server::client_index_from_alives(int ind)
{
    int result = 0;
    for (int i = 0; i < ind; ++i)
        if (this->clients[i].alive)
            ++result;
    return result;
}

bool Server::check_collision(int x, int y)
{
    if (x < 0 || y < 0 || x >= this->map_width || y >= this->map_height)
        return true;
    if (this->game->occupied_pixels.find(std::make_pair(x, y)) != this->game->occupied_pixels.end())
        return true;
    return false;
}

void Server::send_event(int event_id) 
{
    unsigned char message[MESSAGE_FROM_SERVER_MAX_SIZE];
    std::string game_id_str = make_message_from_n_byte(this->game->game_id, 4);
    int cur_ptr, client_id;
    for (client_id = 0; client_id < this->clients.size(); ++client_id) {
        for (cur_ptr = 0; cur_ptr < 4; ++cur_ptr)
            message[cur_ptr] = game_id_str[cur_ptr];
        std::string event_str = this->events[event_id].to_string(event_id);
        for (int j = 0; j < event_str.size(); ++j)
            message[cur_ptr++] = event_str[j];
        std::cout<<"Wysylam event o len = "<<cur_ptr<<"\n";
        this->send_datagram_to_client(&this->clients[client_id].client_address, message, cur_ptr);
    }
}

void Server::process_client(int ind)
{
    this->clients[ind].direction += this->clients[ind].direction * this->clients[ind].turn_direction;
    this->clients[ind].direction = ((this->clients[ind].direction%360) + 360)%360;
    int last_x = (int)this->clients[ind].head_x;
    int last_y = (int)this->clients[ind].head_y;
    this->clients[ind].head_x += sin((-1)*this->clients[ind].head_x/(2*PI));
    this->clients[ind].head_y += cos((-1)*this->clients[ind].head_y/(2*PI));
    int new_x = (int)this->clients[ind].head_x;
    int new_y = (int)this->clients[ind].head_y;
    
    if (new_x == last_x && new_y == last_y)
        return;
    if (check_collision(new_x, new_y)) {
        this->clients[ind].alive = false;
        Event event(2);
        event.create_event_player_eliminated(client_index_from_alives(ind));
        this->events.push_back(event);
        send_event(this->events.size()-1);
        return;
    }
    
    this->game->occupied_pixels.insert(std::make_pair(new_x, new_y));
    Event event(1);
    event.create_event_pixel(client_index_from_alives(ind), new_x, new_y);
    this->events.push_back(event);
    send_event(this->events.size()-1);
}

void Server::process_clients()
{
    this->last_time = this->get_time();
    for (size_t i = 0; i < this->clients.size(); ++i)
        if (this->clients[i].alive)
            process_client(i);
}

void Server::send_events_to_client(int ind)
{
    int from = this->clients[ind].next_expected_event_no;
    int to = this->events.size();
    for (int i = from; i < to; ++i) {
        unsigned char message[MESSAGE_FROM_SERVER_MAX_SIZE];
        std::string game_id_str = make_message_from_n_byte(this->game->game_id, 4);
        int cur_ptr;
        for (cur_ptr = 0; cur_ptr < 4; ++cur_ptr)
            message[cur_ptr] = game_id_str[cur_ptr];
        while (i < to && cur_ptr + this->events[i].to_string(i).size() < MESSAGE_FROM_SERVER_MAX_SIZE) {
            std::string event_str = this->events[i].to_string(i);
            for (int j = 0; j < event_str.size(); ++j)
                message[cur_ptr++] = event_str[j];
            ++i;
        }
        --i;
        std::cout<<"Wysylam do "<<ind<<" wiadomosc:\n";
        for (int j = 0; j < cur_ptr; ++j)
            std::cout<<message[j];
        std::cout<<"\n";
        this->send_datagram_to_client(&this->clients[ind].client_address, message, cur_ptr);
        std::cout<<"poszlo\n";
    }
}

void Server::add_client(Client client)
{
    this->clients.push_back(client);
    sort(this->clients.begin(), this->clients.end(), cmp);
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
    std::vector<std::string> players_name;

    this->game_is_active = true;
    this->game = new Game(this);
    
    for (size_t i = 0; i < this->clients.size(); ++i)
        if (this->clients[i].client_name.size() > 0) {
            this->clients[i].alive = true;
            players_name.push_back(this->clients[i].client_name);
        }

    Event event(0);
    event.create_event_new_game(this->map_width, this->map_height, players_name);
    this->events.push_back(event);
    send_event(this->events.size()-1);
}

bool Server::time_to_next_round_elapsed()
{
    return this->game_is_active && ((this->get_time() - this->last_time) > this->time_period);
}

void Server::close_socket()
{
    if (close(this->sock) == -1) 
        syserr("close");
}

void Server::game_over()
{
    delete this->game;
    this->game_is_active = false;
    Event event(3);
    this->events.push_back(event);
    send_event(this->events.size()-1);
}