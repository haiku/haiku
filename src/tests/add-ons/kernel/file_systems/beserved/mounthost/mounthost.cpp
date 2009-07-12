#include <string.h>
#include <netdb.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <Application.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <Roster.h>
#ifdef __BEOS__
#include <bone/sys/socket.h>
#else
#include "socket.h"
#endif

#include "betalk.h"
#include "rpc.h"
#include "md5.h"
#include "BlowFish.h"
#include "LoginPanel.h"

#ifndef BONE_VERSION
#include "ksocket_internal.h"
#endif

struct mount_nfs_params
{
	unsigned int serverIP;
	char *server;
	char *_export;	
	uid_t uid;
	gid_t gid;
	char *hostname;
	char *folder;
	char user[MAX_USERNAME_LENGTH + 1];
	char password[BT_AUTH_TOKEN_LENGTH * 2 + 1];
};

int main (int argc, char *argv[]);
bool getAuthentication(unsigned int serverIP, char *shareName);
bool authenticateSelf(char *server, char *share, char *user, char *password, bool useTracker);
void getString(char *string, int length, bool displayChars);
void usage();


int main(int argc, char *argv[])
{
	mount_nfs_params params;
	blf_ctx ctx;
	hostent *ent;
	struct stat st;	
	char hostname[256], server[256], share[256], folder[B_PATH_NAME_LENGTH], password[MAX_NAME_LENGTH + 1], *colon;
	port_id port;
	int length, hostArg = 1, pathArg = 2;
	bool useTracker = false;

	BApplication theApp("application/x-vnd.teldar-mounthost");

	if (argc < 3 || argc > 5)
	{
		usage();
		return 1;

	}

	if (*argv[1] == '-')
	{
		hostArg++;
		pathArg++;
		if (strcmp(argv[1], "-t") == 0)
			useTracker = true;
		else
			printf("Option %s not understood and ignored\n", argv[1]);
	}

	if (strcasecmp(argv[pathArg], "at") == 0)
		pathArg++;

	if (stat(argv[pathArg], &st) != 0)
	{
		printf("The specified mount path does not exist.\n");
		return 1;
	}

	strcpy(server, argv[hostArg]);
	colon = strchr(server, ':');
	if (colon == NULL)
	{
		usage();
		return 1;
	}

	*colon = 0;
	strcpy(share, colon + 1);
	strcpy(folder, argv[pathArg]);

	ent = gethostbyname(server);
	if (ent == NULL)
	{
		printf("Server %s is unknown or its network address could not be\nresolved from that host name.\n", server);
		return 1;
	}

	unsigned int serverIP = ntohl(*((unsigned int *) ent->h_addr));

#ifndef BONE_VERSION
	if (!be_roster->IsRunning(KSOCKETD_SIGNATURE))
		if (be_roster->Launch(KSOCKETD_SIGNATURE) < B_NO_ERROR)
		{
			printf("The kernel socket daemon ksocketd could not be started.\n");
			return 1;
		}

	for (int32 i = 0; i < 10; i++)
	{
		port = find_port(KSOCKET_DAEMON_NAME);

		if (port < B_NO_ERROR)
			snooze(1000000LL);
		else
			break;
	}

	if (port < B_NO_ERROR)
	{
		printf ("The kernel socket daemon ksocketd is not responding.\n");
		return 1;
	}
#endif

	int result = B_OK;

	params.user[0] = params.password[0] = 0;

	if (getAuthentication(serverIP, share))
		if (!authenticateSelf(server, share, params.user, password, useTracker))
			return 1;
		else
		{
			// Copy the user name and password supplied in the authentication dialog.
			sprintf(params.password, "%-*s%-*s", B_FILE_NAME_LENGTH, share, MAX_USERNAME_LENGTH, params.user);		//crypt(password, "p1"));
			params.password[BT_AUTH_TOKEN_LENGTH] = 0;

			blf_key(&ctx, (unsigned char *) password, strlen(password));
			blf_enc(&ctx, (unsigned long *) params.password, BT_AUTH_TOKEN_LENGTH / 4);
		}

	params.serverIP = serverIP;
	params.server = server;
	params._export = share;
	params.folder = folder;
	params.uid = 0;
	params.gid = 0;

	gethostname(hostname, 256);
	params.hostname = hostname;

	result = mount("beserved_client", folder, NULL, 0, &params, sizeof(params));

	if (result < B_NO_ERROR)
	{
		printf ("Could not mount remote file share (%s).\n", strerror(errno));
		return 1;
	}

	return 0;
}

bool getAuthentication(unsigned int serverIP, char *shareName)
{
	bt_inPacket *inPacket;
	bt_outPacket *outPacket;
	int security;

	security = EHOSTUNREACH;

	outPacket = btRPCPutHeader(BT_CMD_PREMOUNT, 1, strlen(shareName));
	btRPCPutArg(outPacket, B_STRING_TYPE, shareName, strlen(shareName));
	inPacket = btRPCSimpleCall(serverIP, BT_TCPIP_PORT, outPacket);
	if (inPacket)
	{
		security = btRPCGetInt32(inPacket);
		free(inPacket->buffer);
		free(inPacket);
	}

	return (security == BT_AUTH_BESURE);
}

bool authenticateSelf(char *server, char *share, char *user, char *password, bool useTracker)
{
	char pwBuffer[50];

	if (useTracker)
	{
		BRect frame(0, 0, LOGIN_PANEL_WIDTH, LOGIN_PANEL_HEIGHT);
		LoginPanel *login = new LoginPanel(frame, server, share, false);
		login->Center();
		status_t loginExit;
		wait_for_thread(login->Thread(), &loginExit);
		if (login->IsCancelled())
			return false;

		strcpy(user, login->user);
		strcpy(password, login->md5passwd);
	}
	else
	{
		printf("Username: ");
		getString(user, MAX_NAME_LENGTH, true);
		printf("Password: ");
		getString(pwBuffer, MAX_NAME_LENGTH, false);
		md5EncodeString(pwBuffer, password);
	}

	return true;
}

void getString(char *string, int length, bool displayChars)
{
	char ch;
	int pos = 0;

	while ((ch = getchar()) != '\n')
		if (pos < length - 1)
			string[pos++] = ch;

	string[pos] = 0;
}

void usage()
{
	printf("Usage: mounthost [-bt] server:share [at] path\n");
	printf("Options:\n");
	printf("\t-t\tUse the BeOS Tracker to request the user name and password\n");
}
