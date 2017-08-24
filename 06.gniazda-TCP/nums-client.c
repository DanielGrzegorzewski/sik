#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "err.h"

// 1) We send uint16_t, uint32_t etc., not int (the length of int is platform-dependent).
// 2) If we want to send a structure, we have to declare it with __attribute__((__packed__)).
//    Otherwise the compiler may add a padding bewteen fields (and then e.g. sizeof(struct DataStucture) is 8, not 6).
struct __attribute__((__packed__)) DataStructure {
  uint16_t seq_no;
  uint32_t number;
};

int main(int argc, char *argv[])
{
  int sock;
  struct addrinfo addr_hints;
  struct addrinfo *addr_result;

  int err, seq_no = 0, number;
  ssize_t len;
  struct DataStructure data_to_send;

  if (argc != 3)
    fatal("Usage: %s host port\n", argv[0]);

  // 'converting' host/port in string to struct addrinfo
  memset(&addr_hints, 0, sizeof(struct addrinfo));
  addr_hints.ai_family = AF_INET; // IPv4
  addr_hints.ai_socktype = SOCK_STREAM;
  addr_hints.ai_protocol = IPPROTO_TCP;
  err = getaddrinfo(argv[1], argv[2], &addr_hints, &addr_result);
  if (err == EAI_SYSTEM) // system error
    syserr("getaddrinfo: %s", gai_strerror(err));
  else if (err != 0) // other error (host not found, etc.)
    fatal("getaddrinfo: %s", gai_strerror(err));

  // initialize socket according to getaddrinfo results
  sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
  if (sock < 0)
    syserr("socket");

  // connect socket to the server
  if (connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
    syserr("connect");

  freeaddrinfo(addr_result);

  while (scanf("%d", &number) == 1) { // read all numbers from the standard input

    printf("sending number %d\n", number);

    // we send numbers in network byte order
    data_to_send.seq_no = htons(seq_no++);
    data_to_send.number = htonl(number);

    len = sizeof(data_to_send);
    if (write(sock, &data_to_send, len) != len)
      syserr("partial / failed write");
  }

  if (close(sock) < 0) // socket would be closed anyway when the program ends
    syserr("close");

  return 0;
}
