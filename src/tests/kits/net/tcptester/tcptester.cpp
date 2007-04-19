/*
 * A very simple controlable traffic generator for TCP testing.
 *
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include <OS.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>


struct context {
	int sock;
	uint8 generator;
	int index;
	int8_t buffer[256];
};

static int process_command(context *ctx);

static int
number(context *ctx)
{
	int result = 0;

	while (isdigit(ctx->buffer[ctx->index])) {
		result *= 10;
		result += ctx->buffer[ctx->index] - '0';
		ctx->index++;
	}

	return result;
}


static int
value(context *ctx)
{
	if (ctx->buffer[ctx->index] == '[') {
		ctx->index++;
		int upper, lower = number(ctx);
		if (ctx->buffer[ctx->index] == ',') {
			ctx->index++;
			upper = number(ctx);
		} else {
			upper = lower + 50;
			lower -= 50;
		}

		return lower + rand() % (upper - lower + 1);
	}

	return number(ctx);
}


static int
repeat(context *ctx)
{
	int max, saved, count = number(ctx);

	max = saved = ctx->index;
	for (int i = 0; i < count; i++) {
		ctx->index = saved;
		if (process_command(ctx) < 0)
			return -1;
		if (ctx->index > max)
			max = ctx->index;
	}

	ctx->index = max;
	return 0;
}


static void
send_packet(context *ctx, size_t bytes)
{
	uint8_t buffer[1024];
	uint8_t *ptr = buffer;

	if (bytes > sizeof(buffer))
		ptr = new uint8_t[bytes];

	for (size_t i = 0; i < bytes; i++) {
		ptr[i] = ctx->generator + '0';
		ctx->generator = (ctx->generator + 1) % 10;
	}

	send(ctx->sock, ptr, bytes, 0);

	if (ptr != buffer)
		delete [] ptr;
}


static int
process_command(context *ctx)
{
	while (ctx->buffer[ctx->index] != '.') {
		ctx->index++;

		switch (ctx->buffer[ctx->index - 1]) {
			case 'r':
				if (repeat(ctx) < 0)
					return -1;
				break;

			case 'b':
				send_packet(ctx, 1);
				break;

			case 'p':
				send_packet(ctx, value(ctx));
				break;

			case 's':
				usleep(value(ctx) * 1000);
				break;

			case 'W':
			{
				int value = number(ctx);
				setsockopt(ctx->sock, SOL_SOCKET, SO_SNDBUF, &value,
					sizeof(value));
				break;
			}

			case 'k':
				return -1;
		}
	}

	return 0;
}


static int
read_command(context *ctx)
{
	int index = 0;

	do {
		int size = recv(ctx->sock, ctx->buffer + index, 1, 0);
		if (size == 0)
			return -1;
		else if (size < 0)
			continue;

		index++;
	} while (ctx->buffer[index - 1] != '.');

	ctx->index = 0;
	return process_command(ctx);
}


static int32
handle_client(void *data)
{
	context ctx = { *(int *)data, 0 };

	while (read_command(&ctx) == 0);

	fprintf(stderr, "Client %d leaving.\n", ctx.sock);

	close(ctx.sock);

	return 0;
}


int
main(int argc, char *argv[])
{
	int port = 12345;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-p")) {
			i++;
			assert(i < argc);
			port = atoi(argv[i]);
		} else if (!strcmp(argv[i], "-h")) {
			fprintf(stderr, "tcptester [-p port]\n");
			return 1;
		}
	}

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if (sock < 0) {
		perror("socket()");
		return -1;
	}

	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	if (bind(sock, (sockaddr *)&sin, sizeof(sockaddr_in)) < 0) {
		perror("bind()");
		return -1;
	}

	if (listen(sock, 5) < 0) {
		perror("listen()");
		return -1;
	}

	while (1) {
		sockaddr_in peer;
		socklen_t peerLen = sizeof(peer);

		int newSock = accept(sock, (sockaddr *)&peer, &peerLen);
		if (newSock < 0) {
			perror("accept()");
			return -1;
		}

		char buf[64];
		inet_ntop(AF_INET, &peer.sin_addr, buf, sizeof(buf));

		thread_id newThread = spawn_thread(handle_client, "client",
			B_NORMAL_PRIORITY, &newSock);

		fprintf(stderr, "New client %d from %s with thread id %ld.\n",
			newSock, buf, (int32)newThread);

		resume_thread(newThread);
	}

	return 0;
}

