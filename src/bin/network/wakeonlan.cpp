/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("usage: %s <MAC address>\n", argv[0]);
		return 1;
	}

	unsigned char mac[6];
	if (sscanf(argv[1], "%2x%*c%2x%*c%2x%*c%2x%*c%2x%*c%2x", &mac[0], &mac[1],
		&mac[2], &mac[3], &mac[4], &mac[5]) != 6) {
		printf("unrecognized MAC format\n");
		return 2;
	}

	char buffer[102];
	memset(buffer, 0xff, 6);
	for (int i = 1; i <= 16; i++)
		memcpy(buffer + i * 6, mac, sizeof(mac));

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		printf("failed to create socket: %s\n", strerror(sock));
		return 3;
	}

	int value = 1;
	int result = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &value,
		sizeof(value));
	if (result < 0) {
		printf("failed to set broadcast socket option: %s\n", strerror(result));
		return 4;
	}

	sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = 0xffffffff;
	address.sin_port = 0;

	result = sendto(sock, buffer, sizeof(buffer), 0,
		(struct sockaddr *)&address, sizeof(address));
	if (result < 0) {
		printf("failed to send magic packet: %s\n", strerror(result));
		return 5;
	}

	printf("magic packet sent to %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0],
		mac[1], mac[2], mac[3], mac[4], mac[5]);
	return 0;
}
