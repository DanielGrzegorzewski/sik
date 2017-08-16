#include <iostream>

#include "parser.h"
#include "constans.h"
#include "err.h"

bool check_player_name(std::string name)
{
	size_t i;
	if (name.length() < 1 || name.length() > 64)
		return false;
	for (i = 0; i < name.size(); ++i)
		if (name[i] < 33 || name[i] > 126)
			return false;
	return true;
}

bool check_port(std::string &port)
{
	size_t i;
	if (port.size() > 6 || port.size() < 1)
		return false;
	for (i = 0; i < port.size(); ++i)
		if (port[i] < '0' || port[i] > '9')
			return false;
	int value = atoi(port.c_str());
	if (value < MIN_PORT_VALUE || value > MAX_PORT_VALUE)
		return false;
	return true;
}

void set_server_port(std::string &game_server_host, uint16_t &server_port)
{
	server_port = atoi(DEFAULT_SERVER_PORT);
	size_t i;
	size_t len = game_server_host.size();
	for (i = 0; i < len; ++i)
		if (game_server_host[i] == ':') {
			if (i == len-1)
				fatal(WRONG_CLIENT_ARGS);
			std::string port_str = game_server_host.substr(i+1);
			if (!check_port(port_str))
				fatal(WRONG_PORT);
			server_port = atoi(game_server_host.substr(i+1).c_str());
			game_server_host = game_server_host.substr(0, i);
			return;
		}
}

void set_ui_port(std::string &ui_server_host, uint16_t &ui_port)
{
	ui_port = atoi(DEFAULT_UI_PORT);
	size_t i;
	size_t len = ui_server_host.size();
	for (i = 0; i < len; ++i)
		if (ui_server_host[i] == ':') {
			if (i == len-1)
				fatal(WRONG_CLIENT_ARGS);
			std::string port_str = ui_server_host.substr(i+1);
			if (!check_port(port_str))
				fatal(WRONG_PORT);
			ui_port = atoi(ui_server_host.substr(i+1).c_str());
			ui_server_host = ui_server_host.substr(0, i);
			return;
		}
}

bool parse_client_arguments(int argc, char *argv[], std::string &player_name, std::string &game_server_host, 
							uint16_t &server_port, std::string &ui_server_host, uint16_t &ui_port)
{
	//if (argc != 3 && argc != 4)
	//	fatal(WRONG_CLIENT_ARGS);
	player_name = argv[1];
	if (!check_player_name(player_name))
		fatal(WRONG_PLAYER_NAME);
	game_server_host = argv[2];
	set_server_port(game_server_host, server_port);
	ui_server_host = DEFAULT_UI_NAME;
	if (argc == 4)
		ui_server_host = argv[3];
	set_ui_port(ui_server_host, ui_port);
}