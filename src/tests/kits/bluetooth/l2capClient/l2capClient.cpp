#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/bdaddrUtils.h>
#include <bluetooth/L2CAP/btL2CAP.h>

struct sockaddr_l2cap l2sa;

int
make_named_socket (const bdaddr_t* bdaddr)
{

  int sock;
  size_t size;
  status_t	error;

  /* Create the socket. */
  //printf("Creating socket for %s\n",pbatostr(bdaddr));

  sock = socket (PF_BLUETOOTH, SOCK_STREAM, BLUETOOTH_PROTO_L2CAP);	//	BLUETOOTH_PROTO_L2CAP
  if (sock < 0)
	{
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  /* Bind a name to the socket. */
  l2sa.l2cap_psm    = 1;
  l2sa.l2cap_family = PF_BLUETOOTH;
  memcpy(&l2sa.l2cap_bdaddr, bdaddr, sizeof(bdaddr_t));

  size = sizeof(struct sockaddr_l2cap);
  l2sa.l2cap_len = size;

  if (bind (sock, (struct sockaddr *) &l2sa, size) < 0)
  {
      	perror ("bind");
      	exit (EXIT_FAILURE);
  }

  printf("Connecting socket for %s\n", bdaddrUtils::ToString(*bdaddr));
  if ( (error = connect(sock , (struct sockaddr *) &l2sa, sizeof(l2sa))) < 0) {
	    perror ("connect");
    	exit (EXIT_FAILURE);
  }
  printf("Return status of the connection is %ld \n", error );

  return sock;
}

int main(int argc, char **argv)
{
	int 					sock;
	char 					string[512];
	size_t					len;

	if (argc != 2 ) {
		printf("i need a bdaddr! (%d)\n", argc);
		return 0;
	}

	const bdaddr_t bdaddr = bdaddrUtils::FromString(argv[1]);

	sock = make_named_socket(&bdaddr);

	while (strcmp(string,"bye")!= 0 )	{

		fscanf(stdin, "%s", string );
	//  len = send(sock, string, strlen(string), 0);
		len = sendto(sock, string, strlen(string) + 1 /*\0*/, 0, (struct sockaddr*) &l2sa, sizeof(struct sockaddr_l2cap) );

		printf("Sent %ld bytes\n",len);
		//recvfrom(sock, buff, 4096-1, 0, (struct sockaddr *)  &l2, &len);
	}

	printf("Transmission done ...\n");
	getchar();
	close(sock);
}
