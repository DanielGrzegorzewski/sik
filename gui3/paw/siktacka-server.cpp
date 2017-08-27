#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string>
#include <vector>
#include <memory>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <set>
#include <cmath>
#include <cassert>
#include <functional>
#include "siktacka-helper.h"
#include "siktacka-consts.h"
#include "siktacka-parser.h"
#include "err.h"

static uint32_t WIDTH = DEFAULT_WIDTH;
static uint32_t HEIGHT = DEFAULT_HEIGHT;
static uint32_t PORT = DEFAULT_PORT;
static uint32_t SPEED = DEFAULT_SPEED;
static uint32_t TURN = DEFAULT_TURN;
static uint32_t SEED = DEFUALT_SEED;

static std::set<std::pair<int32_t, int32_t>> BOARD;
static uint32_t GAME_ID = INITIAL_GAME_ID;

void parse_arguments(int argc, char **argv) {
    parse_server_arguments(argc, argv, WIDTH, HEIGHT, PORT, SPEED, TURN, SEED);
}

uint32_t random_gen() {
    uint64_t tmp_seed = (uint64_t) SEED;
    tmp_seed *= RAND_GEN_MULTIPLY;
    tmp_seed %= RAND_GEN_MODULO;
    SEED = (uint32_t) tmp_seed;
    return SEED;
}

int init_sock() {
    struct sockaddr_in6 server_address;
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr(ERROR_MESS_SOCK);

    addr_server_udp_init(server_address, PORT);
    if (bind(sock, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
        syserr(ERROR_MESS_BIND);

    fcntl(sock, F_SETFL, O_NONBLOCK);
    return sock;
}

ClientMessage deserialize_datagram(char* buffer_iter, ssize_t len) {
    uint64_t session_id;
    memcpy(&session_id, buffer_iter, 8);
    buffer_iter += 8;
    session_id = read_uint64(session_id);

    int8_t turn_direction;
    memcpy(&turn_direction, buffer_iter, 1);
    buffer_iter += 1;
    turn_direction = read_int8(turn_direction);

    uint32_t next_expected_event_no;
    memcpy(&next_expected_event_no, buffer_iter, 4);
    buffer_iter += 4;
    next_expected_event_no = read_uint32(next_expected_event_no);

    char player_name[MAX_PLAYER_NAME_LENGTH];
    memset(player_name, 0, MAX_PLAYER_NAME_LENGTH);
    memcpy(&player_name, buffer_iter, len - 13);
    buffer_iter += 13;

    std::string player_name_str(player_name);
    ClientMessage parsed_message = ClientMessage(session_id, next_expected_event_no, 
                                                 player_name_str);
    parsed_message.set_turn_direction(turn_direction);

    return parsed_message;
}

void send_event(int sock, struct sockaddr_in6 &addr,
                socklen_t addr_len, std::shared_ptr<Event> &event) {
    ServerMessage server_message = ServerMessage(GAME_ID);
    server_message.try_add_event(event);
    sendto(sock, server_message.serialize().c_str(), server_message.get_len(), 
           0, (struct sockaddr*) &addr, addr_len);
}

void send_event_to_all(int sock, std::vector<Player> &players,
                       std::shared_ptr<Event> &event) {
    for (Player &player : players)
        send_event(sock, player.get_address(), player.get_addr_len(), event);
}

void send_events(int sock, struct sockaddr_in6 &addr,
                 socklen_t addr_len, std::vector<std::shared_ptr<Event>> &events) {
    if (events.empty())
        return;

    ServerMessage server_message = ServerMessage(GAME_ID);
    for (std::shared_ptr<Event> &event : events) {
        if (server_message.try_add_event(event) == FAILED)
            break;
    }

    sendto(sock, server_message.serialize().c_str(), server_message.get_len(), 
           0, (struct sockaddr*) &addr, addr_len);
}

void send_events_to_all(int sock, std::vector<Player> &players,
                        std::vector<std::shared_ptr<Event>> &events) {
    for (Player &player : players)
        send_events(sock, player.get_address(), player.get_addr_len(), events);
}

bool compare_sockaddr(struct sockaddr_in6 &first, struct sockaddr_in6 &second) {
    if (memcmp(&first, &second, sizeof(first)) == 0)
        return true;
    else
        return false;
}

int validate_player(struct sockaddr_in6 &client_address, ClientMessage &datagram,
                     std::vector<Player> &players) {
    int player_number = 0;
    for (Player &player : players) {
        if (!player.get_connected())
            continue;
        if (compare_sockaddr(client_address, player.get_address())) {
            if (datagram.get_session_id() > player.get_session_id())
                player.set_session_id(datagram.get_session_id());
            if (datagram.get_session_id() == player.get_session_id()) {
                player.set_next_expected(datagram.get_next_expected());
                player.set_turn_direction(datagram.get_turn_direction());
                player.set_activity();
                return player_number;
            }
            if (datagram.get_session_id() < player.get_session_id())
                return IGNORE_DATAGRAM;
        }
        ++player_number;
    }
    return ADD_NEW_PLAYER;
}

void add_new_player(ClientMessage &datagram, std::vector<Player> &players,
                    sockaddr_in6 &address) {
    std::string player_name = std::string(datagram.get_player_name());

    std::cout << "NEW PLAYER: " << player_name << "\n";

    Player new_player = Player(player_name, datagram.get_session_id(),
                               datagram.get_turn_direction(),
                               datagram.get_next_expected(),
                               address);
    players.push_back(new_player);
}

void receive_activities(int server_sock, std::vector<Player> &players,
                      std::vector<std::shared_ptr<Event>> &events) {
    int flags;
    struct sockaddr_in6 client_address;
    char buffer[1000];
    socklen_t rcva_len;
    ssize_t len;

    do {
        rcva_len = (socklen_t) sizeof(client_address);
        flags = 0;
        memset(buffer, 0, sizeof(buffer));
        len = recvfrom(server_sock, buffer, sizeof(buffer), flags,
                       (struct sockaddr *) &client_address, &rcva_len);

        if (len <= 0)
            break;
        else {
            std::string buffer_str = std::string();
            for (int i = 0; i < len; ++i)
                buffer_str += buffer[i];

            if (parse_client_datagram(buffer_str) == FAILED)
                continue;

            char* buffer_iter = buffer;
            ClientMessage datagram = deserialize_datagram(buffer_iter, len);

            int player_index = validate_player(client_address, datagram, players);
            if (player_index == IGNORE_DATAGRAM)
                continue;
            if (player_index == ADD_NEW_PLAYER)
                add_new_player(datagram, players, client_address);

            if (events.size() > datagram.get_next_expected()) {
                std::vector<std::shared_ptr<Event>> events_to_send(
                        events.begin() + datagram.get_next_expected(), events.end());
                send_events(server_sock, client_address, rcva_len, events_to_send);
            }
        }
    } while (len > 0);
}

bool check_game_start(std::vector<Player> &players) {
    int active = 0, inactive = 0;
    for (Player &player : players) {
        player.set_in_play();
        if (player.get_in_play())
            ++active;
        else
            ++inactive;
    }

    if (active >= MIN_PLAYERS_NUMBER && inactive == 0)
        return true;
    else
        return false;
}

void player_eliminated(int sock, Player &player, std::vector<Player> &players,
                       std::vector<std::shared_ptr<Event>> &events) {

    std::cout << "PLAYER ELIMINATED\n";

    player.set_out_of_play();
    std::shared_ptr<EventPlayerEliminated> event_player_eliminated = 
            std::make_shared<EventPlayerEliminated>(events.size(), 
                                                    player.get_player_number());
    events.push_back(event_player_eliminated);

    send_event_to_all(sock, players, events[events.size() - 1]);
}

int check_pixel(double double_x, double double_y) {
    int x = (int) floor(double_x);
    int y = (int) floor(double_y);
    std::pair<int, int> new_pixel = std::make_pair(x, y);

    if (BOARD.find(new_pixel) != BOARD.end())
        return FAILED;
    if ((uint32_t) x >= WIDTH || x < 0)
        return FAILED;
    if ((uint32_t) y >= HEIGHT || y < 0)
        return FAILED;

    return SUCCESS;
}

void pixel(int sock, Player &player, std::vector<Player> &players,
           std::vector<std::shared_ptr<Event>> &events) {

    std::cout << "PIXEL\n";

    int x = (int) floor(player.get_x());
    int y = (int) floor(player.get_y());
    std::pair<int, int> new_pixel = std::make_pair(x, y);
    BOARD.insert(new_pixel);

    std::shared_ptr<EventPixel> event_pixel = std::make_shared<EventPixel>(
            events.size(), player.get_player_number(), x, y);
    events.push_back(event_pixel);

    send_event_to_all(sock, players, events[events.size() - 1]);
}

double get_new_x() {
    return (random_gen() % WIDTH + ADD_TO_RAND);
}

double get_new_y() {
    return (random_gen() % HEIGHT + ADD_TO_RAND);
}

int32_t get_new_direction() {
    return (random_gen() % DEGREES);
}

void init_players_positions(int sock, std::vector<Player> &players,
                            std::vector<std::shared_ptr<Event>> &events) {
    for (Player &player : players) {
        if (player.get_in_play()) {
            player.set_x(get_new_x());
            player.set_y(get_new_y());
            player.set_direction(get_new_direction());
            if (check_pixel(player.get_x(), player.get_y()) == SUCCESS)
                pixel(sock, player, players, events);
            else {
                std::cout << "ELIMINATED --- INIT PLAYERS POSITION\n";
                player_eliminated(sock, player, players, events);
            }
        }
    }
}

void start_game(int sock, std::vector<Player> &players,
                std::vector<std::shared_ptr<Event>> &events) {
    GAME_ID = random_gen();
    init_players(players);
    init_events(events);

    std::vector<std::string> player_names = get_player_names(players);
    std::shared_ptr<EventNewGame> event_new_game = std::make_shared<EventNewGame>(
            events.size(), WIDTH, HEIGHT, player_names);
    events.push_back(event_new_game);

    send_event_to_all(sock, players, events[events.size() - 1]);
    init_players_positions(sock, players, events);
}

bool game_continue(std::vector<Player> &players) {
    int counter = 0;
    for (Player &player : players) {
        if (player.get_in_play()) {
            ++counter;
            if (counter == 2)
                return true;
        }
    }
    return false;
}

int make_move(Player &player) {
    int act_x = (int) floor(player.get_x());
    int act_y = (int) floor(player.get_y());
    int32_t act_direction = player.get_direction();
    int8_t act_turn = player.get_turn_direction();

    player.set_direction(act_direction + act_turn * TURN);
    player.set_x(player.get_x() + cos(player.get_direction() * M_PI / STRAIGHT_ANGLE));
    player.set_y(player.get_y() + sin(player.get_direction() * M_PI / STRAIGHT_ANGLE));

    if (act_x == (int) floor(player.get_x()) && act_y == (int) floor(player.get_y()))
        return NOT_THIS_TIME;

    return MOVED;
}

void next_game_turn(int sock, std::vector<Player> &players,
                std::vector<std::shared_ptr<Event>> &events) {
    for (Player &player : players) {
        if (player.get_in_play()) {
            if (make_move(player) == NOT_THIS_TIME)
                continue;
            if (check_pixel(player.get_x(), player.get_y()) == SUCCESS)
                pixel(sock, player, players, events);
            else {
                std::cout << "ELIMINATED --- NEXT GAME TURN\n";
                player_eliminated(sock, player, players, events);
            }
        }
    }
}

void end_game(int sock, std::vector<Player> &players,
              std::vector<std::shared_ptr<Event>> &events) {
    std::shared_ptr<EventGameOver> event_game_over = std::make_shared<EventGameOver>(
            events.size());
    events.push_back(event_game_over);

    for (Player &player : players)
        player.set_out_of_play();

    BOARD.clear();
    events.clear();
    send_event_to_all(sock, players, events[events.size() - 1]);
}

void disconnect_inactive_clients(std::vector<Player> &players) {
    uint64_t actual_time = get_actual_time();
    for (int i = players.size() - 1; i >= 0; --i) {
        if (!players[i].check_activity(actual_time))
            players.erase(players.begin() + i);
    }
}

int main(int argc, char **argv) {

    parse_arguments(argc, argv);
    uint64_t ROUNDS_INTERVALS = 1000000 / SPEED;

    int server_sock = init_sock();
    std::vector<Player> players;
    std::vector<std::shared_ptr<Event>> events;
    
    while (true) {
        receive_activities(server_sock, players, events);
        disconnect_inactive_clients(players);

        if (check_game_start(players)) {
            start_game(server_sock, players, events);
            uint64_t game_turn = 0;
            uint64_t next_turn = get_actual_time() + ROUNDS_INTERVALS;

            while (game_continue(players)) {
                ++game_turn;

                while (next_turn > get_actual_time())
                    receive_activities(server_sock, players, events);

                disconnect_inactive_clients(players);
                next_game_turn(server_sock, players, events);
                next_turn += ROUNDS_INTERVALS;
            }

            end_game(server_sock, players, events);
        } 
    }

    return 0;
}
