/*
 * eclicork.c
 *
 * Cody Doucette
 * Boston University
 *
 * This program implements an echo client that uses a corked socket to interact
 * with eserv.c, an echo server.
 *
 */

#include "eutils.h"

#define CORK_SIZE 64

static int bytes_corked; /* Number of corked bytes in current packet. */

/**
 * cork(): Cork socket to CORK_SIZE bytes.
 */
static void cork(int s)
{
	int one = 1;
	if (setsockopt(s, IPPROTO_UDP, UDP_CORK, &one, sizeof(int)) < 0)
		perrorq("setsockopt cork");
}

/**
 * uncork(): Uncork socket.
 */
static void uncork(int s)
{
	int zero = 0;
	if (setsockopt(s, IPPROTO_UDP, UDP_CORK, &zero, sizeof(int)) < 0)
		perrorq("setsockopt uncork");
}

/**
 * check_validity(): Check the validity of the program; ensure that the
 * correct number of arguments have been given and that the CORK_SIZE is
 * not greater than the maximum size of a UDP packet.
 */
static void check_validity(int argc)
{
	if (argc != 3) {
		printf("usage: ./corkclient ip port\n");
		exit(1);
	}

	if (CORK_SIZE > MAX_UDP) {
		printf("cork size > maximum UDP packet size\n");
		exit(1);
	}
}

/**
 * empty_cork(): Empties a corked socket by uncorking, receiving the resulting
 * packet, and writing it to a file before corking the socket again.
 */
static void empty_cork(int s, const struct sockaddr_in *srv)
{
	uncork(s);
	recv_write(s, stdout, bytes_corked, srv);
	cork(s);
	bytes_corked = 0;
}
	
/**
 * process_file(): Sets up the input and output files, uncorks the current
 * packet, and then processes the desired file.
 */
static void process_file(int s, struct sockaddr_in *srv, char *in)
{
	FILE *orig, *copy;
	int n_read, n_rcvd;

	int name_size = strlen(in) + strlen(FILE_APPEND) + 1;
	char *filename = (char *)malloc(name_size);
	char *line = NULL;
	size_t line_size = 0;

	setup_output_file(in, filename, name_size);

	if ((orig = fopen(in, "rb")) == NULL)
		perrorq("input fopen");

	if ((copy = fopen(filename, "wb")) == NULL)
		perrorq("output fopen");

	if (bytes_corked)
		empty_cork(s, srv);

	uncork(s);

	while ((n_read = getdelim(&line, &line_size, EOF, orig)) > 0)
		__process_file(s, srv, line, n_read, copy, CORK_SIZE);

	cork(s);

	if (fclose(copy) != 0)
		perrorq("output fclose");

	if (fclose(orig) != 0)
		perrorq("input fclose");

	free(filename);
}

/**
 * process_text(): Depending on whether a packet will exceed CORK_SIZE bytes,
 * this function either splits up the message into separate packets or fits the
 * message into the current packet.
 */
static void process_text(int s, struct sockaddr_in *srv, char *in, int n_read)
{
	if (n_read + bytes_corked > CORK_SIZE) {
		int bytes_avail = CORK_SIZE - bytes_corked;
		send_packet(s, in, bytes_avail, srv);
		bytes_corked += bytes_avail;

		empty_cork(s, srv);
		process_text(s, srv, in + bytes_avail, n_read - bytes_avail);
	}

	send_packet(s, in, n_read, srv);
	bytes_corked += n_read;

	if (bytes_corked == CORK_SIZE)
		empty_cork(s, srv);
}

int main(int argc, char *argv[])
{
	struct sockaddr_in srv;
	int s;
	int n_read;

	char *input = NULL;
	size_t line_size = 0;

	check_validity(argc);

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
		perrorq("socket");

	memset((char *)&srv, 0, sizeof(struct sockaddr_in));
	srv.sin_family = AF_INET;
	srv.sin_port = htons(atoi(argv[2]));
	if (inet_aton(argv[1], &srv.sin_addr) == 0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	while (1) {
		if ((n_read = getline(&input, &line_size, stdin)) < 0)
			perrorq("getline");

		/* Empty message. */
		if (n_read == 1)
			continue;

		strtok(input, "\n");

		/* Quit client. */
		if (strcmp(input, "-q") == 0)
			break;

		cork(s);

		if (is_file(input))
			process_file(s, &srv, input + 3);
		else
			process_text(s, &srv, input, n_read - 1);

		puts("\n");
	}

	if (close(s) == -1)
		perrorq("close");

	free(input);

	return 0;
}