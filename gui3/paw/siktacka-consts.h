#ifndef SIKTACKA_CONSTS__H
#define SIKTACKA_CONSTS__H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include "err.h"


/* DEFINE CONSTANTS VALUES FOR SIKTACKA-CLIENT */

const std::string DEFAULT_PLAYER_NAME = "";
const std::string DEFAULT_GAME_SERVER_HOST = "";
const std::string DEFAULT_GAME_SERVER_PORT = "12345";
const std::string DEFAULT_UI_SERVER_HOST = "localhost";
const std::string DEFAULT_UI_SERVER_PORT = "12346";

const uint32_t MAX_PLAYER_NAME_LENGTH = 64;
const uint32_t MIN_ACSII_NAME_VALUE = 33;
const uint32_t MAX_ACSII_NAME_VALUE = 126;
const uint32_t MAX_UDP_DATAGRAM_SIZE = 512;
const int64_t SENDING_INTERVALS = 20000;
const int32_t MAX_PORT_VALUE = 65535;
const int32_t GUI_BUFFER_SIZE = 1000;
const int32_t ERROR_CODE = 1;

const std::string LEFT_KEY_DOWN = "LEFT_KEY_DOWN";
const std::string LEFT_KEY_UP = "LEFT_KEY_UP";
const std::string RIGHT_KEY_DOWN = "RIGHT_KEY_DOWN";
const std::string RIGHT_KEY_UP = "RIGHT_KEY_UP";

const std::string NEW_GAME_PREFIX = "NEW_GAME ";
const std::string PIXEL_PREFIX = "PIXEL ";
const std::string PLAYER_ELIMINATED_PREFIX = "PLAYER_ELIMINATED ";


/* DEFINE CONSTANTS VALUES FOR SIKTACKA-SERVER */

const uint32_t DEFAULT_HEIGHT = 600;
const uint32_t DEFAULT_WIDTH = 800;
const uint32_t DEFAULT_PORT = 12345;
const uint32_t DEFAULT_SPEED = 50;
const uint32_t DEFAULT_TURN = 6;
const uint32_t DEFUALT_SEED = 0;

const uint32_t RAND_GEN_MULTIPLY = 279470273;
const uint32_t RAND_GEN_MODULO = 4294967291;

const uint8_t EVENT_TYPE_NEW_GAME = 0;
const uint8_t EVENT_TYPE_PIXEL = 1;
const uint8_t EVENT_TYPE_PLAYER_ELIMINATED = 2;
const uint8_t EVENT_TYPE_GAME_OVER = 3;

const uint64_t DISCONNECTING_TIME = 2000000;
const int MIN_PLAYERS_NUMBER = 2;
const int MIN_CLIENT_DATAGRAM_SIZE = 13;
const int MIN_SERVER_EVENT_SIZE = 13;
const int IGNORE_DATAGRAM = -2;
const int ADD_NEW_PLAYER = -3;
const int DEGREES = 360;
const double ADD_TO_RAND = 0.5;
const double STRAIGHT_ANGLE = 180.0;
const int MOVED = 1;
const int NOT_THIS_TIME = -1;


/* DEFINE OTHER CONSTANTS VALUES */

const uint32_t INITIAL_GAME_ID = 0;
const bool INITIAL_GAME_STARTED = false;
const uint32_t INITIAL_NEXT_EXPECTED = 0;

const int SUCCESS = 0;
const int FAILED = -1;


/* DEFINE ERROR MESSAGES FOR SIKTACKA-CLIENT */

const char *ERROR_MESS_WRONG_CLIENT_USAGE = "Usage: ./siktacka-client player_name\
 game_server_host[:port] [ui_server_host[:port]]";
const char *ERROR_MESS_INCORRECT_NAME_LENGTH = "Incorrect length of player name!";
const char *ERROR_MESS_INCORRECT_NAME_ASCII = "Incorrect ASCII code in player name!";
const char *ERROR_MESS_INCORRECT_SERVER_PORT = "Incorrect server port!";


/* DEFINE ERROR MESSAGES FOR SIKTACKA-SERVER */

const char *ERROR_MESS_WRONG_SERVER_USAGE = "Usage: ./siktacka-server [-W n] [-H n]\
 [-p n] [-s n] [-t n] [-r n]";
const char *ERROR_MESS_INCORRECT_WIDTH = "Incorrect width!";
const char *ERROR_MESS_INCORRECT_HEIGHT = "Incorrect height!";
const char *ERROR_MESS_INCORRECT_PORT = "Incorrect port!";
const char *ERROR_MESS_INCORRECT_SPEED = "Incorrect speed!";
const char *ERROR_MESS_INCORRECT_TURN = "Incorrect turn!";
const char *ERROR_MESS_INCORRECT_SEED = "Incorrect seed!";


/* DEFINE OTHER ERROR MESSAGES */

const char *ERROR_MESS_GETADDRINFO = "getaddrinfo failed!";
const char *ERROR_MESS_SOCK = "sock failed!";
const char *ERROR_MESS_BIND = "bind failed!";
const char *ERROR_MESS_CONNECT = "connect failed!";
const char *ERROR_MESS_WRITE = "write failed!";
const char *ERROR_MESS_SETSOCKOPT = "setsockopt failed!";

const char *ERROR_MESS_INCORRECT_DATAGRAM = "Incorrect datagram!";
const char *ERROR_MESS_INCORRECT_CRC = "Incorrect CRC32!";
const char *ERROR_MESS_INCORRECT_LEN = "Incorrect len in datagram!";


#endif //SIKTACKA_CONSTS__H
