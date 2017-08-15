#ifndef _HELPER_H_
#define _HELPER_H_

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

unsigned char make_char(unsigned long long my_time, int from, int to)
{
    unsigned char result = 0;
    int i;
    for (i = from; i <= to; ++i)
        if (my_time&(1LL<<i))
            result += (1<<(i-from));
    return result;
}

unsigned long long read_time(unsigned  char* buffer)
{
    unsigned long long res = 0;
    int i;
    for (i = 0; i < 8; ++i)
        res += buffer[i]*(1LL<<(56-8*i));
    return res;
}

std::string make_message_from_n_byte(uint64_t timestamp, uint8_t n)
{
    int i;
    std::string message = "";
    for (i = n-1; i >= 0; --i)
        message += make_char(timestamp, 8*i, 8*i+7);
    return message;
}

#endif