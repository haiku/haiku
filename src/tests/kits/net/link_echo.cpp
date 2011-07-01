/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <ctype.h>
#include <errno.h>
#include <net/if_dl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include <NetworkInterface.h>


extern const char* __progname;


static const size_t kBufferSize = 1500;
static const uint32 kFrameType = 0x8998;

static const char* kProgramName = __progname;


static void
parse_mac_address(const char* string, uint8* mac)
{
	if (sscanf(string, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx", &mac[0],
			&mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) < 6) {
		fprintf(stderr, "%s: Invalid MAC address.\n", kProgramName);
		exit(EXIT_FAILURE);
	}
}


static void
link_client(int fd, const BNetworkAddress& server)
{
	char buffer[kBufferSize];

	while (fgets(buffer, kBufferSize, stdin) != NULL) {
		size_t length = strlen(buffer);
		if (length > 0)
			length--;

		if (sendto(fd, buffer, length, 0, server, server.Length()) < 0) {
			fprintf(stderr, "%s: sendto(): %s\n", kProgramName,
				strerror(errno));
			exit(EXIT_FAILURE);
		}
		printf("sent %" B_PRIuSIZE " bytes...\n", length);

		ssize_t bytesRead = recvfrom(fd, buffer, kBufferSize - 1, 0, NULL,
			NULL);
		if (bytesRead < 0) {
			fprintf(stderr, "%s: recvfrom(): %s\n", kProgramName,
				strerror(errno));
			exit(EXIT_FAILURE);
		}
		buffer[bytesRead] = 0;
		printf("-> %s\n", buffer);
	}
}


static void
link_server(int fd)
{
	while (true) {
		BNetworkAddress client;
		socklen_t length = sizeof(client);

		char buffer[kBufferSize];
		ssize_t bytesRead = recvfrom(fd, buffer, kBufferSize - 1, 0, client,
			&length);
		if (bytesRead < 0) {
			fprintf(stderr, "%s: recvfrom(): %s\n", kProgramName,
				strerror(errno));
			exit(EXIT_FAILURE);
		}
		buffer[bytesRead] = '\0';

		printf("got <%s> from client %s\n", buffer, client.ToString().String());

		for (int i = 0; i < bytesRead; i++) {
			if (islower(buffer[i]))
				buffer[i] = toupper(buffer[i]);
			else if (isupper(buffer[i]))
				buffer[i] = tolower(buffer[i]);
		}
		printf("replying <%s>\n", buffer);

		if (sendto(fd, buffer, bytesRead, 0, client, client.Length()) < 0) {
			fprintf(stderr, "%s: sendto(): %s\n", kProgramName,
				strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}


int
main(int argc, char** argv)
{
	enum {
		CLIENT_MODE,
		SERVER_MODE,
		BROADCAST_MODE,
	} mode = CLIENT_MODE;

	if (argc < 3) {
		fprintf(stderr, "usage: %s <device> client <server-mac-address>\n"
			"or     %s <device> broadcast\n"
			"or     %s <device> server\n", kProgramName, kProgramName,
			kProgramName);
		exit(EXIT_FAILURE);
	}

	BNetworkInterface interface(argv[1]);
	BNetworkAddress link;
	if (interface.GetHardwareAddress(link) != B_OK)
		perror("get hardware address");

	BNetworkAddress server;

	if (!strcmp(argv[2], "client")) {
		mode = CLIENT_MODE;
		if (argc < 4) {
			fprintf(stderr, "usage: %s client <server-mac-address>\n",
				kProgramName);
			exit(EXIT_FAILURE);
		}

		uint8 macAddress[6];
		parse_mac_address(argv[3], macAddress);
		server.SetToLinkLevel(macAddress, sizeof(macAddress));
	} else if (!strcmp(argv[2], "broadcast")) {
		mode = BROADCAST_MODE;

		uint8 broadcastAddress[6];
		for (size_t i = 0; i < sizeof(broadcastAddress); i++)
			broadcastAddress[i] = 0xff;
		server.SetToLinkLevel(broadcastAddress, sizeof(broadcastAddress));
	} else if (!strcmp(argv[2], "server"))
		mode = SERVER_MODE;

	int fd = socket(AF_LINK, SOCK_DGRAM, 0);
	if (fd < 0)
		perror("socket");

	// bind to protocol
	link.SetLinkLevelFrameType(kFrameType);
	server.SetLinkLevelFrameType(kFrameType);

	if (bind(fd, link, link.Length()) != 0)
		perror("bind");

	socklen_t length = sizeof(link);
	if (getsockname(fd, link, &length) != 0)
		perror("getsockname");

	printf("bound to %s\n", link.ToString().String());

	if (mode == BROADCAST_MODE) {
		int option = 1;
		setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &option, sizeof(option));
	}

	switch (mode) {
		case CLIENT_MODE:
		case BROADCAST_MODE:
			link_client(fd, server);
			break;
		case SERVER_MODE:
			link_server(fd);
			break;
	}

	return 0;
}
