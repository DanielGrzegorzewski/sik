#ifndef SIKTACKA_PARSER__H
#define SIKTACKA_PARSER__H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "siktacka-helper.h"
#include "siktacka-consts.h"
#include "err.h"


void parse_player_name(char *player_name, std::string &PLAYER_NAME) {
    PLAYER_NAME = player_name;
    if (PLAYER_NAME.length() > MAX_PLAYER_NAME_LENGTH)
        fatal(ERROR_MESS_INCORRECT_NAME_LENGTH);
    for (size_t i = 0; i < PLAYER_NAME.length(); ++i)
        if ((uint32_t) PLAYER_NAME[i] < MIN_ACSII_NAME_VALUE ||
                (uint32_t) PLAYER_NAME[i] > MAX_ACSII_NAME_VALUE)
            fatal(ERROR_MESS_INCORRECT_NAME_ASCII);
}

void parse_port_value(std::string &PORT) {
    if (PORT.length() > 6 || (uint64_t) stoull(PORT) == 0 ||
        (uint64_t) stoull(PORT) > MAX_PORT_VALUE)
        fatal(ERROR_MESS_INCORRECT_SERVER_PORT);
}

void parse_server_data(char *server_data, std::string &HOST, std::string &PORT) {
    std::string server_str(server_data);
    bool parse_port = false;
    size_t colon_index = -1;
    for (size_t i = 0; i < server_str.length(); ++i) {
        if (server_str[i] == ':') {
            parse_port = true;
            colon_index = i;
            break;
        }
    }

    if (parse_port) {
        HOST = server_str.substr(0, colon_index);
        PORT = server_str.substr(colon_index + 1, server_str.length());
        parse_port_value(PORT);
    } else {
        HOST = server_str;
    }
}

void parse_client_arguments(int argc, char **argv, std::string &PLAYER_NAME,
	                        std::string &GAME_SERVER_HOST, std::string &GAME_SERVER_PORT,
	                        std::string &UI_SERVER_HOST, std::string &UI_SERVER_PORT) {

    if (argc != 3 && argc != 4)
        syserr(ERROR_MESS_WRONG_CLIENT_USAGE);

    parse_player_name(argv[1], PLAYER_NAME);
    parse_server_data(argv[2], GAME_SERVER_HOST, GAME_SERVER_PORT);
    if (argc == 4)
        parse_server_data(argv[3], UI_SERVER_HOST, UI_SERVER_PORT);
}

void parse_server_arguments(int argc, char **argv, uint32_t &WIDTH, uint32_t &HEIGHT,
	                        uint32_t &PORT, uint32_t &SPEED, uint32_t &TURN, uint32_t &SEED) {
    SEED = (unsigned long) time(NULL);

    int option = 0;
    while ((option = getopt(argc, argv, "W:H:p:s:t:r:")) != -1) {
        switch (option) {
            case 'W':
                if ((WIDTH = strtoul(optarg, NULL, 10)) == 0)
                    fatal(ERROR_MESS_INCORRECT_WIDTH);
                break;
            case 'H':
                if ((HEIGHT = strtoul(optarg, NULL, 10)) == 0)
                    fatal(ERROR_MESS_INCORRECT_HEIGHT);
                break;
            case 'p':
                if ((PORT = strtoul(optarg, NULL, 10)) == 0)
                    fatal(ERROR_MESS_INCORRECT_PORT);
                break;
            case 's':
                if ((SPEED = strtoul(optarg, NULL, 10)) == 0)
                    fatal(ERROR_MESS_INCORRECT_SPEED);
                break;
            case 't':
                if ((TURN = strtoul(optarg, NULL, 10)) == 0)
                    fatal(ERROR_MESS_INCORRECT_TURN);
                break;
            case 'r':
                if ((SEED = strtoul(optarg, NULL, 10)) == 0)
                    fatal(ERROR_MESS_INCORRECT_SEED);
                break;
            default:
                fatal(ERROR_MESS_WRONG_SERVER_USAGE);
        }
    }
}

int parse_client_datagram(std::string &datagram) {
    if (datagram.length() < MIN_CLIENT_DATAGRAM_SIZE ||
        datagram.length() > MIN_CLIENT_DATAGRAM_SIZE + MAX_PLAYER_NAME_LENGTH) {
        std::cerr << ERROR_MESS_INCORRECT_DATAGRAM << "\n";
        return FAILED;
    }

    std::string player_name = datagram.substr(13, MAX_PLAYER_NAME_LENGTH);
    for (size_t i = 0; i < player_name.length(); ++i) {
        if (player_name[i] != '\0') {
            if ((uint32_t) player_name[i] < MIN_ACSII_NAME_VALUE ||
                (uint32_t) player_name[i] > MAX_ACSII_NAME_VALUE) {
                std::cerr << ERROR_MESS_INCORRECT_DATAGRAM << "\n";
                return FAILED;
            }
        }
    }

    return SUCCESS;
}


#endif //SIKTACKA_PARSER__H
