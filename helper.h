#ifndef _HELPER_H_
#define _HELPER_H_

#include "player.h"

void send_datagram(Player *player, unsigned char *datagram, int len);

ssize_t receive_datagram(Player *player, unsigned char *datagram, int len);

unsigned char make_char(unsigned long long my_time, int from, int to);

unsigned long long read_time(unsigned char* buffer);

int8_t read_1_byte_number(unsigned char *buffer);

uint32_t read_4_byte_number(unsigned char *buffer);

uint64_t read_8_byte_number(unsigned char *buffer);

std::string make_message_from_n_byte(uint64_t timestamp, uint8_t n);

#endif