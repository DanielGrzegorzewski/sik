#ifndef _PARSER_H_
#define _PARSER_H_

bool check_player_name(std::string name);

bool check_port(std::string &port);

void set_server_port(std::string &game_server_host, uint16_t &server_port);

void set_ui_port(std::string &ui_server_host, uint16_t &ui_port);

bool parse_client_arguments(int argc, char *argv[], std::string &player_name, std::string &game_server_host, 
							uint16_t &server_port, std::string &ui_server_host, uint16_t &ui_port);

#endif
