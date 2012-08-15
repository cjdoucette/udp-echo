/*
 * eutils.h
 *
 * Cody Doucette
 * Boston University
 *
 * A header file for the utilities that support the echo clients and server.
 *
 */

#ifndef _ECHO_UTILS_H
#define _ECHO_UTILS_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* 0xffff - sizeof(Maximum UDP header) */
#define MAX_UDP (0xffff - 8)

static inline int is_file(const char *x)
{
	return (*x == '-') && (*(x + 1) == 'f');
}

void check_cli_params(int argc, char * const argv[]);

void send_packet(int s, const char *buf, int n, const struct sockaddr_in *srv);

int recv_packet(int s, char *buf, int n, const struct sockaddr_in *srv);

void recv_write(int s, FILE *copy, int n, const struct sockaddr_in *srv);

void process_file(int s, const struct sockaddr_in *srv, const char *orig_name,
	int chunk_size);

#endif /* _ECHO_UTILS_H */
