#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

#define BUFFER_SIZE 80000

void syserr(const char *fmt, ...)
{
  va_list fmt_args;

  fprintf(stderr, "ERROR: ");
  va_start(fmt_args, fmt);
  vfprintf(stderr, fmt, fmt_args);
  va_end(fmt_args);
  fprintf(stderr, " (%d; %s)\n", errno, strerror(errno));
  exit(1);
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

void make_message(unsigned char *message, unsigned long long timestamp, unsigned char c)
{
	int i;
	for (i = 7; i >= 0; --i)
		message[7-i] = make_char(timestamp, 8*i, 8*i+7);
	message[8] = c;
}

void make_socket(int argc, char *argv[], struct addrinfo *addr_hints, struct addrinfo **addr_result, struct sockaddr_in *my_address, int *sock)
{
	(void) memset(addr_hints, 0, sizeof(struct addrinfo));
	(*addr_hints).ai_family = AF_INET; // IPv4
	(*addr_hints).ai_socktype = SOCK_DGRAM;
	(*addr_hints).ai_protocol = IPPROTO_UDP;
	(*addr_hints).ai_flags = 0;
	(*addr_hints).ai_addrlen = 0;
	(*addr_hints).ai_addr = NULL;
	(*addr_hints).ai_canonname = NULL;
	(*addr_hints).ai_next = NULL;

	if (getaddrinfo(argv[3], NULL, addr_hints, addr_result) != 0) {
		syserr("getaddrinfo");
	}

	int port = 10001;
	if (argc > 4)
		port = atoi(argv[4]);

	(*my_address).sin_family = AF_INET;
	(*my_address).sin_addr.s_addr =
		((struct sockaddr_in*) ((*addr_result)->ai_addr))->sin_addr.s_addr;
	(*my_address).sin_port = htons((uint16_t) port);

	freeaddrinfo(*addr_result);

	*sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (*sock < 0)
		syserr("socket");
}

bool is_number(char *arg, int len)
{
	int i;
	for (i = 0; i < len; ++i)
		if (*(arg+i) < '0' || *(arg+i) > '9')
			return false;
	return true;
}

void check_arg(int argc, char *argv[])
{
	if (argc != 4 && argc != 5)
		syserr("You should put 3 or 4 arguments.\n");
	
	if (strlen(argv[2]) != 1)
		syserr("Second argument should be single character.\n");
	
	if (argc == 5 && (strlen(argv[4]) > 5 || !is_number(argv[4], strlen(argv[4])) || atoi(argv[4]) > 65535))
		syserr("Fourth argument is not a port number.\n");
}

int main(int argc, char *argv[]) 
{	
	int sock, flags, sflags;
	size_t len;
	ssize_t snd_len, rcv_len;
	unsigned char buffer[BUFFER_SIZE];
	struct sockaddr_in srvr_address;
	socklen_t rcva_len;

	struct addrinfo addr_hints;
	struct addrinfo *addr_result;
	struct sockaddr_in my_address;

	check_arg(argc, argv);

	make_socket(argc, argv, &addr_hints, &addr_result, &my_address, &sock);

	unsigned long long timestamp = atoll(argv[1]);
	unsigned char c = **(argv+2);

	unsigned char message[9];
	make_message(message, timestamp, c);

	len = 9;
	sflags = 0;

	rcva_len = (socklen_t) sizeof(my_address);
	snd_len = sendto(sock, message, len, sflags,
		(struct sockaddr *) &my_address, rcva_len);
	if (snd_len != (ssize_t) len) {
		syserr("partial / failed write");
	}

	while (true) {
		(void) memset(buffer, 0, sizeof(buffer));
		flags = 0;
		len = (size_t) sizeof(buffer) - 1;
		rcva_len = (socklen_t) sizeof(srvr_address);
		rcv_len = recvfrom(sock, buffer, len, flags,
			(struct sockaddr *) &srvr_address, &rcva_len);
		if (rcv_len < 0) {
			syserr("read");
		}
		(void) printf("%llu %c %s\n", read_time(buffer), *(buffer+8), buffer+9);
	}

	if (close(sock) == -1) { 
		syserr("close");
	}

	return 0;
}
