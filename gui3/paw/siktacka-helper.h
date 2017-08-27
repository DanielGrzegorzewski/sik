#ifndef SIKTACKA_HELPER__H
#define SIKTACKA_HELPER__H

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <memory>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <boost/crc.hpp>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include "siktacka-consts.h"
#include "err.h"


/* SOME USEFUL FUNCTIONS */

int8_t read_int8(int8_t value) {
    return value;
}

uint8_t read_uint8(uint8_t value) {
    return value;
}

uint32_t read_uint32(uint32_t value) {
    return be32toh(value);
}

uint64_t read_uint64(uint64_t value) {
    return be64toh(value);
}

std::string serialize_string_8(uint8_t value) {
    std::string serialized;
    serialized += (char) (value & 0xFF);
    return serialized;
}

std::string serialize_string_32(uint32_t value) {
    value = htobe32(value);
    std::string serialized;
    for (int i = 0; i < 4; ++i)
        serialized += (char) ((value >> 8 * i) & 0xFF);
    return serialized;
}

std::string serialize_string_64(uint64_t value) {
    value = htobe64(value);
    std::string serialized;
    for (int i = 0; i < 8; ++i)
        serialized += (char) ((value >> 8 * i) & 0xFF);
    return serialized;
}

int8_t deserialize_string_8(std::string serialized) {
    int8_t value;
    const char* serialized_char = serialized.c_str();
    memcpy(&value, serialized_char, sizeof(int8_t));
    return value;
}

uint8_t deserialize_string_8u(std::string serialized) {
    uint8_t value;
    const char* serialized_char = serialized.c_str();
    memcpy(&value, serialized_char, sizeof(uint8_t));
    return value;
}

uint32_t deserialize_string_32(std::string serialized) {
    uint32_t value;
    const char* serialized_char = serialized.c_str();
    memcpy(&value, serialized_char, sizeof(uint32_t));
    return be32toh(value);
}

uint64_t deserialize_string_64(std::string serialized) {
    uint64_t value;
    const char* serialized_char = serialized.c_str();
    memcpy(&value, serialized_char, sizeof(uint64_t));
    return be64toh(value);
}

std::string serialize_player_name(char* player_name) {
    std::string serialized = "";
    for (uint32_t i = 0; i < MAX_PLAYER_NAME_LENGTH; ++i) {
        if (player_name[i] == '\0')
            break;
        serialized += player_name[i];
    }
    return serialized;
}

std::string deserialize_player_name(std::string &player_name) {
    std::string deserialized = "";
    for (uint32_t i = 0; i < MAX_PLAYER_NAME_LENGTH; ++i) {
        if (player_name[i] == '\0')
            break;
        deserialized += player_name[i];
    }
    return deserialized;
}

// https://stackoverflow.com/questions/3756323/
// getting-the-current-time-in-milliseconds
uint64_t get_actual_time() {
    timeval time_val;
    gettimeofday(&time_val, NULL);
    int64_t actual_time = time_val.tv_sec * 1000000 + time_val.tv_usec;
    return actual_time;
}

// https://stackoverflow.com/questions/2573726/how-to-use-boostcrc
uint32_t get_crc32(std::string &string_to_check) {
    boost::crc_32_type result;
    result.process_bytes(string_to_check.data(), string_to_check.length());
    return result.checksum();
}


/* DEFINE PLAYER CLASS */

class Player {
    private:
        std::string name;
        uint8_t player_number;
        uint64_t session_id;
        int8_t turn_direction;
        uint32_t next_expected_event_no;
        struct sockaddr_in6 address;
        socklen_t addr_len;

        bool in_play;
        bool connected;
        double x;
        double y;
        int32_t direction;
        uint64_t last_activity;

    public:
        Player(std::string &player_name, uint64_t session_id, int8_t turn_direction,
               uint32_t next_expected_event_no, struct sockaddr_in6 &address) :
                name(player_name),
                session_id(session_id),
                turn_direction(turn_direction),
                next_expected_event_no(next_expected_event_no) {
            this->set_address(address);
            this->set_connected(true);
            this->set_activity(); 
            this->in_play = false;
        }

        std::string get_name() { return name; }
        uint8_t get_player_number() { return player_number; }
        uint64_t get_session_id() { return session_id; }
        int8_t get_turn_direction() { return turn_direction; }
        struct sockaddr_in6 &get_address() { return address; }
        socklen_t get_addr_len() { return addr_len; }
        bool get_connected() { return connected; }
        bool get_in_play() { return in_play; }
        double get_x() { return x; }
        double get_y() { return y; }
        int32_t get_direction() { return direction; }

        void set_name(std::string &name) { this->name = name; }
        void set_player_number(uint8_t player_number) {
            this->player_number = player_number;
        }
        void set_session_id(uint64_t session_id) {
            this->session_id = session_id;
        }
        void set_next_expected(uint32_t next_expected_event_no) {
            this->next_expected_event_no = next_expected_event_no;
        }
        void set_turn_direction(int8_t turn_direction) {
            this->turn_direction = turn_direction;
        }
        void set_address(sockaddr_in6 &address) {
            this->addr_len = (size_t) sizeof(address);
            memcpy(&this->address, &address, addr_len);
        }
        void set_in_play() {
            if (name != "" && turn_direction != 0)
                this->in_play = true;
        }
        void set_out_of_play() {
            this->in_play = false;
        }
        void set_connected(bool connected) {
            this->connected = connected;
        }
        void set_activity() {
            this->last_activity = get_actual_time();
        }
        bool check_activity(uint64_t actual_time) {
            if (this->last_activity + DISCONNECTING_TIME < actual_time)
                this->connected = false;
            return get_connected();
        }
        void set_x(double x) { this->x = x; }
        void set_y(double y) { this->y = y; }
        void set_direction(int32_t direction) {
            this->direction = direction;
        }
};


/* DEFINE EVENT RECORDS CLASS */

class Event {
    private:
        uint32_t len;
        uint32_t event_no;
        uint8_t event_type;

    public:
        Event(uint32_t len, uint32_t event_no, uint8_t event_type) :
                len(5 + len),
                event_no(event_no),
                event_type(event_type) {}

        virtual std::string serialize_event_data() {
            return std::string();
        }

        virtual std::string serialize() {
            std::string serialized = serialize_string_32(len);
            serialized += serialize_string_32(event_no);
            serialized += serialize_string_8(event_type);
            serialized += serialize_event_data();
            uint32_t crc32 = get_crc32(serialized);
            serialized += serialize_string_32(crc32);
            return serialized;
        }

        uint32_t get_len() { return len; }
        uint32_t get_event_no() { return event_no; }
        uint8_t get_event_type() { return event_type; }
};

class EventNewGame : public Event {
    private:
        struct EventNewGameData {
            uint32_t maxx;
            uint32_t maxy;
            std::vector<std::string> players;

            EventNewGameData(uint32_t maxx, uint32_t maxy,
                             std::vector<std::string> &player_names) :
                    maxx(maxx),
                    maxy(maxy) {
                for (std::string &player_name : player_names)
                    this->players.push_back(player_name);
            }
        } event_data;

    public:
        EventNewGame(uint32_t event_no, uint32_t maxx, uint32_t maxy,
                     std::vector<std::string> &player_names) :
                Event(this->get_promise_length(player_names), event_no, EVENT_TYPE_NEW_GAME),
                event_data(maxx, maxy, player_names) {}

        uint32_t get_data_length() {
            uint32_t data_length = 8;
            for (std::string &player_name : event_data.players)
                data_length += player_name.length() + 1;
            return data_length;
        }

        uint32_t get_promise_length(std::vector<std::string> &player_names) {
            uint32_t promise_length = 8;
            for (std::string &player_name : player_names)
                promise_length += player_name.length() + 1;
            return promise_length;
        }

        std::string serialize_event_data() {

            std::string serialized = serialize_string_32(event_data.maxx);
            serialized += serialize_string_32(event_data.maxy);
            for (std::string &player_name : event_data.players) {
                serialized += player_name;
                serialized += '\0';
            }
            return serialized;
        }
};

class EventPixel : public Event {
    private:
        struct EventPixelData {
            uint8_t player_number;
            uint32_t x;
            uint32_t y;

            EventPixelData(uint8_t player_number, uint32_t x, uint32_t y) :
                    player_number(player_number),
                    x(x), 
                    y(y) {}
        } event_data;

    public:
        EventPixel(uint32_t event_no, uint8_t player_number, uint32_t x, uint32_t y) :
                Event(this->get_data_length(), event_no, EVENT_TYPE_PIXEL),
                event_data(player_number, x, y) {}

        uint32_t get_data_length() { return 9; }

        std::string serialize_event_data() {
            std::string serialized = serialize_string_8(event_data.player_number);
            serialized += serialize_string_32(event_data.x);
            serialized += serialize_string_32(event_data.y);
            return serialized;
        }
};

class EventPlayerEliminated : public Event {
    private:
        struct EventPlayerEliminatedData {
            uint8_t player_number;

            EventPlayerEliminatedData(uint8_t player_number) :
                    player_number(player_number) {}
        } event_data;

    public:
        EventPlayerEliminated(uint32_t event_no, uint8_t player_number) :
                Event(this->get_data_length(), event_no, EVENT_TYPE_PLAYER_ELIMINATED),
                event_data(player_number) {}

        uint32_t get_data_length() { return 1; }

        std::string serialize_event_data() {
            std::string serialized = serialize_string_8(event_data.player_number);
            return serialized;
        }
};

class EventGameOver : public Event {
    private:
        struct EventGameOverData {
            EventGameOverData() {}
        } event_data;
    public:
        EventGameOver(uint32_t event_no) :
                Event(this->get_data_length(), event_no, EVENT_TYPE_GAME_OVER),
                event_data() {}

        uint32_t get_data_length() { return 0; }

        std::string serialize_event_data() {
            std::string serialized = "";
            return serialized;
        }
};


/* DEFINE MESSAGES CLASSES */

class ClientMessage {
    private:
        uint64_t session_id;
        int8_t turn_direction;
        uint32_t next_expected_event_no;
        char player_name[MAX_PLAYER_NAME_LENGTH];

    public:
        ClientMessage(uint64_t session_id, uint32_t next_expected_event_no,
                      std::string &player_name_str) :
                session_id(session_id),
                turn_direction((int8_t) 0),
                next_expected_event_no(next_expected_event_no) {
            memset(player_name, 0, MAX_PLAYER_NAME_LENGTH);
            for (uint32_t i = 0; i < player_name_str.length(); ++i)
                player_name[i] = player_name_str[i];
        }

        uint64_t get_session_id() { return session_id; }
        int8_t get_turn_direction() { return turn_direction; }
        uint32_t get_next_expected() { return next_expected_event_no; }
        char* get_player_name() { return player_name; }

        void set_turn_direction(int8_t turn_direction) {
            this->turn_direction = turn_direction;
        }

        std::string serialize() {
            std::string serialized = serialize_string_64(session_id);
            serialized += (char) turn_direction;
            serialized += serialize_string_32(next_expected_event_no);
            serialized += serialize_player_name(player_name);
            return serialized;
        }
};

class ServerMessage {
    private:
        uint32_t game_id;
        std::vector<std::shared_ptr<Event>> events;
        int32_t len;

    public:
        ServerMessage(uint32_t game_id) :
                game_id(game_id),
                len(3 * sizeof(uint32_t)) {
            events = std::vector<std::shared_ptr<Event>>();
        }

        int32_t get_len() { return len; }

        void add_event(std::shared_ptr<Event> event) {
            events.push_back(event);
            len += event->get_len();
        }

        int try_add_event(std::shared_ptr<Event> event) {
            if (len + event->get_len() <= MAX_UDP_DATAGRAM_SIZE) {
                add_event(event);
                return SUCCESS;
            } else {
                return FAILED;
            }
        }

        std::string serialize() {
            std::string serialized = serialize_string_32(game_id);
            for (std::shared_ptr<Event> event : events)
                serialized += event->serialize();
            return serialized;
        }
};


/* SOME USEFUL FUNCTIONS */

void check_get_addr_info(int err) {
    if (err) {
        if (err == EAI_SYSTEM)
            syserr(ERROR_MESS_GETADDRINFO);
        else
            fatal(ERROR_MESS_GETADDRINFO);
    }
}

void addr_hints_client_udp_init(struct addrinfo &addr_hints) {
    (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = NULL;
    addr_hints.ai_canonname = NULL;
    addr_hints.ai_next = NULL;
}

void addr_client_udp_init(struct sockaddr_in6 &client_address,
                          std::string &server_port) {
    client_address.sin6_family = AF_INET6;
    client_address.sin6_port = htons(0);
    client_address.sin6_flowinfo = 0;
    client_address.sin6_addr = in6addr_any;
    client_address.sin6_scope_id = 0;
}

void addr_hints_client_tcp_init(struct addrinfo &addr_hints) {
    (void) memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;   
}

void addr_server_udp_init(struct sockaddr_in6 &server_address, uint32_t &port) {
    server_address.sin6_family = AF_INET6;
    server_address.sin6_port = htons((uint16_t) port);
    server_address.sin6_flowinfo = 0;
    server_address.sin6_addr = in6addr_any;
    server_address.sin6_scope_id = 0;
}

// https://stackoverflow.com/questions/1380463/sorting-a-vector-of-custom-objects
struct player_comparator {
    inline bool operator() (Player &player_1, Player &player_2) {
        return (player_1.get_name() < player_2.get_name());
    }
};

void init_players(std::vector<Player> &players) {
    std::sort(players.begin(), players.end(), player_comparator());
    uint8_t number = 0; 
    for (Player &player : players) {
        if (player.get_in_play()) {
            player.set_player_number(number);
            ++number;
        }
    }
}

void init_events(std::vector<std::shared_ptr<Event>> &events) {
    events.clear();
}

std::vector<std::string> get_player_names(std::vector<Player> &players) {
    std::vector<std::string> player_names;
    for (Player &player : players) {
        if (player.get_in_play())
            player_names.push_back(player.get_name());
    }
    return player_names;
}

#endif //SIKTACKA_HELPER__H
