#include <netdb.h>
#include <iostream>
#include <cstring>
#include <vector>

#include "helper.h"
#include "player.h"
#include "err.h"

void send_datagram(Player *player, unsigned char *datagram, int len)
{
    int sflags = 0;
    ssize_t snd_len;
    socklen_t rcva_len;

    rcva_len = (socklen_t) sizeof(player->my_address);
    snd_len = sendto(player->sock, datagram, len, sflags,
        (struct sockaddr *) &player->my_address, rcva_len);
    if (snd_len != (ssize_t) len)
        syserr("partial / failed write");
}

void receive_datagram(Player *player, unsigned char *datagram, int len)
{
    int flags = 0;
    socklen_t rcva_len;
    ssize_t rcv_len;
    struct sockaddr_in srvr_address;

    memset(datagram, 0, sizeof(datagram));
    rcv_len = recvfrom(player->sock, datagram, len, flags,
        (struct sockaddr *) &srvr_address, &rcva_len);
    if (rcv_len < 0)
        syserr("read");
}

unsigned long long read_time(unsigned  char* buffer)
{
    unsigned long long res = 0;
    int i;
    for (i = 0; i < 8; ++i)
        res += buffer[i]*(1LL<<(56-8*i));
    return res;
}

uint64_t read_8_byte_number(unsigned char *buffer)
{
    uint64_t result = 0;
    for (size_t i = 0; i < 8; ++i)
        result += buffer[i]*(1LL<<(8*(7-i)));
    return result;
}

int8_t read_1_byte_number(unsigned char *buffer)
{
    int8_t result = buffer[0];
    return result;
}

uint32_t read_4_byte_number(unsigned char *buffer)
{
    uint32_t result = 0;
    for (size_t i = 0; i < 4; ++i)
        result += buffer[i]*(1LL<<(8*(3-i)));
    return result;
}

std::string make_message_from_n_byte(uint64_t timestamp, uint8_t n)
{
    int i;
    std::string message = "";
    for (i = n-1; i >= 0; --i)
        message += make_char(timestamp, 8*i, 8*i+7);
    return message;
}

unsigned char make_char(unsigned long long my_time, int from, int to)
{
    unsigned char result = 0;
    int i;
    for (i = from; i <= to; ++i)
        if (my_time&(1LL<<i))
            result += (1<<(i-from));
    return result;
}

std::string get_string_from_datagram(unsigned char *datagram, uint8_t from, uint8_t to)
{
    std::string result_string = "";
    for (int i = from; i <= to; ++i)
        result_string += datagram[i];
    return result_string;
}

// datagram size 100, zal: >= 13, < 100, sprawdzic dla == 100 (>=100 tez) czy if bedzie dzialal (Wyslac dlugie imie)
bool parse_datagram(unsigned char *datagram, uint32_t len, uint64_t &session_id, int8_t &turn_direction, uint32_t &next_expected_event_no, std::string &player_name)
{
    unsigned char session_id_str[8];
    for (size_t i = 0; i < 8; ++i)
        session_id_str[i] = datagram[i];

    unsigned char turn_direction_str[1];
    turn_direction_str[0] = datagram[8];

    unsigned char next_expected_event_no_str[4];
    for (size_t i = 9; i < 13; ++i)
        next_expected_event_no_str[i-9] = datagram[i];

    session_id = read_8_byte_number(session_id_str);
    turn_direction = read_1_byte_number(turn_direction_str);
    next_expected_event_no = read_4_byte_number(next_expected_event_no_str);
    player_name = get_string_from_datagram(datagram, 13, len-1);

    if (player_name.size() > 64)
        return false;

    for (size_t i = 0; i < player_name.size(); ++i)
        if (player_name[i] < 33 || player_name[i] > 126)
            return false;

    return true;
}

Datagram::Datagram(unsigned char *buffer, size_t len)
{
    parse_datagram(buffer, len, this->session_id, this->turn_direction, this->next_expected_event_no, this->player_name);
}