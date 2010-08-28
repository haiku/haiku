/*
 * Copyright 2010 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/L2CAP/btL2CAP.h>

int
CreateL2capSocket(const bdaddr_t* bdaddr, struct sockaddr_l2cap& l2sa, uint16 psm)
{
	int sock;
	size_t size;
	status_t error;

	/* Create the socket. */
	printf("Creating socket ...\n");

	sock = socket(PF_BLUETOOTH, SOCK_STREAM, BLUETOOTH_PROTO_L2CAP);
	if (sock < 0) {
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	/* Bind a name to the socket. */
	printf("Binding socket ...\n");

	size = sizeof(struct sockaddr_l2cap);

	l2sa.l2cap_family = PF_BLUETOOTH;
	l2sa.l2cap_len = size;
	memcpy(&l2sa.l2cap_bdaddr, bdaddr, sizeof(bdaddr_t));
    l2sa.l2cap_psm = psm;

	if (bind(sock, (struct sockaddr *)&l2sa, size) < 0) {
		perror ("bind");
		exit (EXIT_FAILURE);
	}

	printf("Connecting socket for %s\n", bdaddrUtils::ToString(*bdaddr));
	if ((error = connect(sock, (struct sockaddr *)&l2sa, sizeof(l2sa))) < 0) {
		perror ("connect");
		exit (EXIT_FAILURE);
	}

	printf("Return status of the connection is %ld \n", error );

	return sock;
}

int main(int argc, char **argv)
{
	struct sockaddr_l2cap l2sa;
	char string[512];
	size_t len;
	uint16 psm = 1;

	if (argc < 2 ) {
		printf("I need a bdaddr!\nUsage:\n\t%s bluetooth_address [psm]\n",
			argv[0]);
		return 0;
	}

	if (argc > 2) {
		psm = atoi(argv[2]);
		printf("PSM requested %d\n", psm);
	}

	if ((psm & 1) == 0)
		printf("WARNING: PSM requested is not pair\n");

	const bdaddr_t bdaddr = bdaddrUtils::FromString(argv[1]);

	int sock = CreateL2capSocket(&bdaddr, l2sa, psm);

	while (strcmp(string,"bye") != 0) {

		fscanf(stdin, "%s", string);
		len = sendto(sock, string, strlen(string) + 1 /*\0*/, 0,
			(struct sockaddr*) &l2sa, sizeof(struct sockaddr_l2cap));


		printf("Sent %ld bytes\n", len);

		// len = send(sock, string + 1 /*\0*/, strlen(string), 0);
		// recvfrom(sock, buff, 4096-1, 0, (struct sockaddr *)  &l2, &len);
	}

	printf("Transmission done ... (press key to close socket)\n");
	getchar();
	getchar();
	printf("Closing ... \n");
	close(sock);
}
