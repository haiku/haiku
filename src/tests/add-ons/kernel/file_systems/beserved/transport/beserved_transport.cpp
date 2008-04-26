#include <DataIO.h>
#include <Message.h>
#include <Directory.h>
#include <netdb.h>
#include <Alert.h>
#include <stdio.h>
#include <unistd.h>
#include <iomanip>

#include "betalk.h"
#include "beserved_transport.h"
#include "beserved_printer.h"
#include "beserved_rpc.h"
#include "transport_rpc.h"

#include "LoginPanel.h"
#include "BlowFish.h"

#define SPOOL_FILE_TYPE		"application/x-vnd.Be.printer-spool"


#if (!__MWERKS__ || defined(WIN32))
using namespace std;
#else 
#define std
#endif

BeServedTransport::BeServedTransport(BMessage *msg) :
  BDataIO()
{
	hostent *ent;
	char jobName[100], errorMsg[256];

	server[0] = 0;
	printerName[0] = 0;
	file[0] = 0;
	user[0] = 0;
	password[0] = 0;
	jobId = 0;
	error = true;

	const char *spool_path = msg->FindString(SPOOL_PATH);
	if (spool_path && *spool_path)
	{
		dir.SetTo(spool_path);

		dir.ReadAttr(SERVER_NAME, B_STRING_TYPE, 0, server, sizeof(server));
		dir.ReadAttr(PRINTER_NAME, B_STRING_TYPE, 0, printerName,  sizeof(printerName));
		dir.ReadAttr(USER_NAME, B_STRING_TYPE, 0, user, sizeof(user));
		dir.ReadAttr(USER_PASSWORD, B_STRING_TYPE, 0, password, sizeof(password));

		strcpy(jobName, "BeServed Print Job");
		fs.close();

		// Get the IP address of the server.
		ent = gethostbyname(server);
		if (ent == NULL)
		{
			sprintf(errorMsg, "The server %s, which hosts printer %s, cannot be found on the network.", server, printerName);
			BAlert *alert = new BAlert("", errorMsg, "OK");
			alert->Go();
			return;
		}

		unsigned int serverIP = ntohl(*((unsigned int *) ent->h_addr));

		// Initialize the RPC system.
		btRPCInit(&info);

		// Connect to the remote host.
		info.s = btConnect(&info, serverIP, BT_TCPIP_PORT);
		if (info.s != INVALID_SOCKET)
		{
			int result;
	
			// Try to start the new print job.  If we get an access denied message, we may need
			// to query for new credentials (permissions may have changed on the server since our
			// last print job).  Keep asking for a new username/password until we either get a
			// successful authentication or the user cancels.
			while (true)
			{
				result = btStartPrintJob(&info, printerName, user, password, jobName, &serverJobId);
				if (result != EACCES)
					break;
				else if (!getNewCredentials())
					break;
			}

			if (result != B_OK)
			{
				BAlert *alert = new BAlert("", "Your print job could not be started.  This network printer may no longer be available, or you may not have permission to use it.", "OK");
				alert->Go();
			}
			else
				error = false;
		}
		else
		{
			sprintf(errorMsg, "The server %s, which hosts printer %s, is not responding.", server, printerName);
			BAlert *alert = new BAlert("", errorMsg, "OK");
			alert->Go();
		}
	}
}

BeServedTransport::~BeServedTransport()
{
	// Now that all the data has been transmitted for the print job, start printing by
	// committing the job on the server.  This ends local responsibility for the task.
	// Then it's up to the server.
	btCommitPrintJob(&info, printerName, serverJobId);

	// End our connection with the remote host and close the RPC system.
	btDisconnect(&info);
	btRPCClose(&info);
}

ssize_t BeServedTransport::Read(void *, size_t)
{
	return 0;
}

ssize_t BeServedTransport::Write(const void *buffer, size_t size)
{
	if (size > 0)
		if (btPrintJobData(&info, printerName, serverJobId, (char *) buffer, size) != B_OK)
			return 0;

	return size;
}

bool BeServedTransport::getNewCredentials()
{
	blf_ctx ctx;
	int length;

	BRect frame(0, 0, LOGIN_PANEL_WIDTH, LOGIN_PANEL_HEIGHT);
	LoginPanel *login = new LoginPanel(frame, server, printerName, false);
	login->Center();
	status_t loginExit;
	wait_for_thread(login->Thread(), &loginExit);
	if (login->IsCancelled())
		return false;

	// Copy the user name.
	strcpy(user, login->user);

	// Copy the user name and password supplied in the authentication dialog.
	sprintf(password, "%-*s%-*s", B_FILE_NAME_LENGTH, printerName, MAX_USERNAME_LENGTH, login->user);
	length = strlen(password);
	assert(length == BT_AUTH_TOKEN_LENGTH);

	password[length] = 0;
	blf_key(&ctx, (unsigned char *) login->md5passwd, strlen(login->md5passwd));
	blf_enc(&ctx, (unsigned long *) password, length / 4);

	dir.WriteAttr(USER_NAME, B_STRING_TYPE, 0, user, sizeof(user));
	dir.WriteAttr(USER_PASSWORD, B_STRING_TYPE, 0, password, sizeof(password));

	return true;
}
