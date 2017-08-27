#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sstream>
#include <fcntl.h>
#include "siktacka-helper.h"
#include "siktacka-consts.h"
#include "siktacka-parser.h"
#include "err.h"

static std::string PLAYER_NAME = DEFAULT_PLAYER_NAME;
static std::string GAME_SERVER_HOST = DEFAULT_GAME_SERVER_HOST;
static std::string GAME_SERVER_PORT = DEFAULT_GAME_SERVER_PORT;
static std::string UI_SERVER_HOST = DEFAULT_UI_SERVER_HOST;
static std::string UI_SERVER_PORT = DEFAULT_UI_SERVER_PORT;

static uint32_t GAME_ID = INITIAL_GAME_ID;
static bool GAME_STARTED = INITIAL_GAME_STARTED;
static uint32_t NEXT_EXPECTED = INITIAL_NEXT_EXPECTED;
static std::vector<std::string> PLAYERS;

static uint32_t WIDTH;
static uint32_t HEIGHT;

void parse_arguments(int argc, char **argv) {
    parse_client_arguments(argc, argv, PLAYER_NAME, GAME_SERVER_HOST, GAME_SERVER_PORT,
                           UI_SERVER_HOST, UI_SERVER_PORT);
}

int connect_with_server(struct sockaddr_in6 &client_address) {
    int sock;
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;
    struct sockaddr_in6 server_address;

    addr_hints_client_udp_init(addr_hints);

    int err = getaddrinfo(GAME_SERVER_HOST.c_str(), GAME_SERVER_PORT.c_str(),
                          &addr_hints, &addr_result);
    check_get_addr_info(err);

    sock = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sock < 0)
        syserr(ERROR_MESS_SOCK);

    addr_client_udp_init(client_address, GAME_SERVER_PORT);
    if (bind(sock, (struct sockaddr*) &client_address, sizeof(client_address)) < 0)
        syserr(ERROR_MESS_BIND);
    memcpy(&server_address, addr_result->ai_addr, addr_result->ai_addrlen);
    if (connect(sock, (struct sockaddr*) &server_address, sizeof(server_address)) < 0)
        syserr(ERROR_MESS_CONNECT);

    freeaddrinfo(addr_result);
    fcntl(sock, F_SETFL, O_NONBLOCK);
    return sock;
}

int connect_with_gui() {
    int gui_sock;
    struct addrinfo addr_hints;
    struct addrinfo *addr_result;

    addr_hints_client_tcp_init(addr_hints);
    int err = getaddrinfo(UI_SERVER_HOST.c_str(), UI_SERVER_PORT.c_str(),
                          &addr_hints, &addr_result);
    check_get_addr_info(err);

    gui_sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (gui_sock < 0)
        syserr(ERROR_MESS_SOCK);
    if (connect(gui_sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
        syserr(ERROR_MESS_CONNECT);

    // https://stackoverflow.com/questions/17842406/
    //how-would-one-disable-nagles-algorithm-in-linux
    int flag = 1;
    if (setsockopt(gui_sock, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(int)))
        syserr(ERROR_MESS_SETSOCKOPT);

    freeaddrinfo(addr_result);
    fcntl(gui_sock, F_SETFL, O_NONBLOCK);
    return gui_sock;
}

void read_data_from_gui(int gui_sock, bool &LEFT_KEY, bool &RIGHT_KEY) {
    char buffer[GUI_BUFFER_SIZE];
    (void) memset(buffer, 0, sizeof(buffer));
    size_t len = (size_t) sizeof(buffer) - 1;
    ssize_t rcv_len = read(gui_sock, buffer, len);
    if (rcv_len < 0)
        return;

    // https://stackoverflow.com/questions/18318980/
    // taking-input-of-a-string-word-by-word
    std::string buffer_str(buffer);
    std::string word;
    std::istringstream iss(buffer_str);
    while(iss >> word) {
        if (word == LEFT_KEY_DOWN) {
            LEFT_KEY = true;
        } else if (word == LEFT_KEY_UP) {
            LEFT_KEY = false;
        } else if (word == RIGHT_KEY_DOWN) {
            RIGHT_KEY = true;
        } else if (word == RIGHT_KEY_UP) {
            RIGHT_KEY = false;
        }
    }
}

int8_t get_direction(int gui_sock, bool &LEFT_KEY, bool &RIGHT_KEY) {
    read_data_from_gui(gui_sock, LEFT_KEY, RIGHT_KEY);
    if (LEFT_KEY && !RIGHT_KEY)
        return (int8_t) (-1);
    else if (RIGHT_KEY && !LEFT_KEY)
        return (int8_t) 1;
    else
        return (int8_t) 0;
}

int check_game_id(uint32_t game_id) {
    if (!GAME_STARTED) {
        GAME_STARTED = true;
        GAME_ID = game_id;
        return SUCCESS;
    } else if (GAME_ID == game_id)
        return SUCCESS;
    else
        return FAILED;
}

void send_to_gui(int gui_sock, std::string &gui_message) {
    gui_message += char(10);
    send(gui_sock, gui_message.c_str(), gui_message.length(), 0);
    std::cout << gui_message << "\n";
}

void send_new_game(int gui_sock, std::vector<std::string> &player_names) {
    std::string gui_message = NEW_GAME_PREFIX;
    gui_message += std::to_string(WIDTH);
    gui_message += " ";
    gui_message += std::to_string(HEIGHT);
    for (std::string &player_name : player_names)
        gui_message += (" " + player_name);

    send_to_gui(gui_sock, gui_message);
}

void send_pixel(int gui_sock, uint8_t player_number, uint32_t x, uint32_t y) {
    std::string gui_message = PIXEL_PREFIX;
    gui_message += std::to_string(x);
    gui_message += " ";
    gui_message += std::to_string(y);
    gui_message += " ";
    gui_message += PLAYERS[player_number];

    send_to_gui(gui_sock, gui_message);
}

void send_player_eliminated(int gui_sock, uint8_t player_number) {
    std::string gui_message = PLAYER_ELIMINATED_PREFIX;
    gui_message += PLAYERS[player_number];

    send_to_gui(gui_sock, gui_message);
}

 
void deserialize_event_new_game(char* buffer, uint32_t len, int gui_sock) {
    if (len <= 2 * sizeof(uint32_t))
        fatal(ERROR_MESS_INCORRECT_LEN);

    uint32_t coded_width;
    memcpy(&coded_width, buffer, sizeof(uint32_t));
    WIDTH = read_uint32(coded_width);
    buffer += sizeof(uint32_t);

    uint32_t coded_height;
    memcpy(&coded_height, buffer, sizeof(uint32_t));
    HEIGHT = read_uint32(coded_height);
    buffer += sizeof(uint32_t);
    len -= 2 * sizeof(uint32_t);

    std::vector<std::string> player_names;
    std::string player_name;
    for (uint32_t i = 0; i < len; ++i) {
        if (buffer[i] != '\0')
            player_name += buffer[i];
        else {
            std::string copy = player_name;
            player_names.push_back(copy);
            player_name.clear();
        }
    }

    for (std::string name : player_names) {
        if (name.length() > MAX_PLAYER_NAME_LENGTH)
            fatal(ERROR_MESS_INCORRECT_NAME_LENGTH);
        for (size_t i = 0; i < name.length(); ++i)
            if ((uint32_t) name[i] < MIN_ACSII_NAME_VALUE ||
                    (uint32_t) name[i] > MAX_ACSII_NAME_VALUE)
                fatal(ERROR_MESS_INCORRECT_NAME_ASCII);
    }

    for (std::string name : player_names)
        PLAYERS.push_back(name);

    send_new_game(gui_sock, player_names);
}

void deserialize_event_pixel(char* buffer, uint32_t len, int gui_sock) {
    if (len != sizeof(uint8_t) + 2 * sizeof(uint32_t))
        fatal(ERROR_MESS_INCORRECT_LEN);

    uint8_t coded_player_number;
    memcpy(&coded_player_number, buffer, sizeof(uint8_t));
    uint8_t player_number = read_uint8(coded_player_number);
    buffer += sizeof(uint8_t);

    uint32_t coded_x;
    memcpy(&coded_x, buffer, sizeof(uint32_t));
    uint32_t x = read_uint32(coded_x);
    buffer += sizeof(uint32_t);

    uint32_t coded_y;
    memcpy(&coded_y, buffer, sizeof(uint32_t));
    uint32_t y = read_uint32(coded_y);
    buffer += sizeof(uint32_t);

    send_pixel(gui_sock, player_number, x, y);
}

void deserialize_event_player_eliminated(char* buffer, uint32_t len, int gui_sock) {
    if (len != sizeof(uint8_t))
        fatal(ERROR_MESS_INCORRECT_LEN);

    uint8_t coded_player_number;
    memcpy(&coded_player_number, buffer, sizeof(uint8_t));
    uint8_t player_number = read_uint8(coded_player_number);
    buffer += sizeof(uint8_t);

    send_player_eliminated(gui_sock, player_number);
}

void deserialize_event_game_over(char* buffer, uint32_t len) {
    GAME_STARTED = false;
    PLAYERS.clear();
}

void deserialize_events(char* buffer, ssize_t buffer_len, int gui_sock) {
    if (buffer_len < MIN_SERVER_EVENT_SIZE) {
        std::cerr << ERROR_MESS_INCORRECT_DATAGRAM << "\n";
        return;
    }

    uint32_t coded_len;
    memcpy(&coded_len, buffer, sizeof(uint32_t));
    uint32_t len = read_uint32(coded_len);
    buffer += sizeof(uint32_t);

    uint32_t coded_crc32;
    memcpy(&coded_crc32, (buffer + len), sizeof(uint32_t));
    uint32_t crc32 = read_uint32(coded_crc32);
    std::string data_to_check = std::string(buffer - 4, buffer + len);
    if (crc32 != get_crc32(data_to_check)) {
        std::cerr << ERROR_MESS_INCORRECT_CRC << "\n";
        return;
    }

    uint32_t coded_event_no;
    memcpy(&coded_event_no, buffer, sizeof(uint32_t));
    uint32_t event_no = read_uint32(coded_event_no);
    buffer += sizeof(uint32_t);

    uint8_t coded_event_type;
    memcpy(&coded_event_type, buffer, sizeof(uint8_t));
    uint8_t event_type = read_uint8(coded_event_type);
    buffer += sizeof(uint8_t);

    uint32_t datagram_size = len - sizeof(uint32_t) - sizeof(uint8_t);
    switch (event_type) {
        case EVENT_TYPE_NEW_GAME:
            deserialize_event_new_game(buffer, datagram_size, gui_sock);
            break;

        case EVENT_TYPE_PIXEL:
            deserialize_event_pixel(buffer, datagram_size, gui_sock);
            break;

        case EVENT_TYPE_PLAYER_ELIMINATED:
            deserialize_event_player_eliminated(buffer, datagram_size, gui_sock);
            break;

        case EVENT_TYPE_GAME_OVER:
            deserialize_event_game_over(buffer, datagram_size);
            break;

        default:
             break;
    }

    buffer_len -= 2 * sizeof(uint32_t);
    buffer_len -= len;
    buffer += sizeof(uint32_t);
    if (NEXT_EXPECTED == event_no)
        ++NEXT_EXPECTED;
}

int main(int argc, char **argv) {

    parse_arguments(argc, argv);
    char buffer[MAX_UDP_DATAGRAM_SIZE];
    struct sockaddr_in6 client_address;
    int server_sock = connect_with_server(client_address);
    int gui_sock = connect_with_gui();

    static bool LEFT_KEY;
    static bool RIGHT_KEY;

    uint64_t session_id = get_actual_time();
    uint64_t time_to_send = session_id;

    while (true) {
        ClientMessage client_message = ClientMessage(session_id, NEXT_EXPECTED, 
                                                     PLAYER_NAME);
        int8_t act_direction = get_direction(gui_sock, LEFT_KEY, RIGHT_KEY);
        client_message.set_turn_direction(act_direction);
        std::string client_serialized_message = client_message.serialize();
        const char* datagram = client_serialized_message.c_str();

        if ((uint32_t) write(server_sock, datagram, client_serialized_message.length()) 
                != client_serialized_message.length())
            syserr(ERROR_MESS_WRITE);

        time_to_send = get_actual_time() + SENDING_INTERVALS;
        while (time_to_send > get_actual_time()) {

            while (true) {
                (void) memset(buffer, 0, sizeof(buffer));
                ssize_t len = read(server_sock, buffer, sizeof(buffer));
                if (len <= 0)
                    break;

                uint32_t coded_game_id;
                memcpy(&coded_game_id, buffer, 4);
                uint32_t game_id = read_uint32(coded_game_id);
                
                if (check_game_id(game_id) == SUCCESS) {
                    deserialize_events(buffer + 4, len - 4, gui_sock);
                }
            }
        }
    }

    return 0;
}
