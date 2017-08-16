#ifndef _HELPER_H_
#define _HELPER_H_

#include "player.h"

void send_datagram(Player *player, unsigned char *datagram, int len);

void receive_datagram(Player *player, unsigned char *datagram, int len);

unsigned char make_char(unsigned long long my_time, int from, int to);

unsigned long long read_time(unsigned  char* buffer);

std::string make_message_from_n_byte(uint64_t timestamp, uint8_t n);

bool parse_datagram(unsigned char *datagram, uint32_t len, uint64_t &session_id, int8_t &turn_direction, uint32_t &next_expected_event_no, std::string &player_name);

#endif