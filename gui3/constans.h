#ifndef _CONSTANS_H_
#define _CONSTANS_H_

const char *LOCALHOST = "127.0.0.1";
const char *DEFAULT_SERVER_PORT = "12345";
const char *DEFAULT_UI_PORT = "12346";
const char *DEFAULT_UI_NAME = "localhost";
const char *WRONG_CLIENT_ARGS = "Usage: ./siktacka-client player_name game_server_host[:port] [ui_server_host[:port]]";
const char *WRONG_PLAYER_NAME = "Wrong player name. It must contain 1-64 characters, each between 33-126 ascii code";
const char *WRONG_PORT = "Wrong port. It has to be between 1 and 65535";

const int SENDING_INTERVAL = 20;
const int BUFFER_SIZE = 80000;
const int MIN_PORT_VALUE = 1;
const int MAX_PORT_VALUE = 65535;
const int DEFAULT_MAP_WIDTH = 800;
const int DEFAULT_MAP_HEIGHT = 600;
const int DEFAULT_GAME_SPEED = 50;
const int DEFAULT_TURN_SPEED = 6;
const int SERVER_BUFFER_SIZE = 100;
const int MESSAGE_FROM_SERVER_MAX_SIZE = 512;
const int MORE_THAN_MAX_DATAGRAM_SIZE = 555;
const int MIN_PLAYER_DATAGRAM_SIZE = 13;
const int MAX_PLAYER_DATAGRAM_SIZE = 77;
const int MAX_PLAYER_NAME_LENGTH = 64;
const int MIN_PLAYER_NAME_ASCII = 33;
const int MAX_PLAYER_NAME_ASCII = 126;

const double PI = 3.14159265359;

#endif