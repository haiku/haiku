// Sun, 18 Jun 2000
// Y.Takagi

#include <sys/socket.h>
#include <netdb.h>

#include <errno.h>
#include <string.h>

#include <iomanip>
#include <algorithm>
#include "LpsClient.h"
#include "Socket.h"


using namespace std;


#define LPS_SERVER_PORT		515
#define LPS_CLIENT_PORT_S	721
#define LPS_CLIENT_PORT_E	731

#define LPS_CHECK_QUEUE 			'\001'
#define LPS_PRINT_JOB				'\002'
#define LPS_DISPLAY_SHORT_QUEUE 	'\003'
#define LPS_DISPLAY_LONG_QUEUE		'\004'
#define LPS_REMOVE_JOB				'\005'
#define LPS_END_TRANSFER			'\000'
#define LPS_ABORT					'\001'
#define LPS_RECEIVE_CONTROL_FILE	'\002'
#define LPS_RECEIVE_DATA_FILE		'\003'

#define LPS_OK						'\0'
#define LPS_ERROR					'\1'
#define LPS_NO_SPOOL_SPACE			'\2'

#ifndef INADDR_NONE
#define INADDR_NONE		0xffffffff
#endif

#define ERRNO	errno


LpsClient::LpsClient(const char *host)
	:
	fConnected(false),
	fHost(host),
	fSock(NULL)
{
}


LpsClient::~LpsClient()
{
	close();
}


void
LpsClient::connect() throw(LPSException)
{
	for (int localPort = LPS_CLIENT_PORT_S; localPort <=  LPS_CLIENT_PORT_E;
		localPort++) {

		if (fSock) {
			delete fSock;
		}
		fSock = new Socket(fHost.c_str(), LPS_SERVER_PORT, localPort);
		if (fSock->good()) {
			fInput = &fSock->getInputStream();
			fOutput = &fSock->getOutputStream();
			fConnected = true;
			return;
		}
	}

	throw(LPSException(fSock->getLastError()));
}


void
LpsClient::close()
{
	fConnected = false;
	if (fSock) {
		delete fSock;
		fSock = NULL;
	}
}


void
LpsClient::receiveJob(const char *printer) throw(LPSException)
{
	if (fConnected) {
		*fOutput << LPS_PRINT_JOB << printer << '\n' << flush;
		checkAck();
	}
}


void
LpsClient::receiveControlFile(int size, const char *name) throw(LPSException)
{
	if (fConnected) {

		char cfname[32];
		strncpy(cfname, name, sizeof(cfname));
		cfname[sizeof(cfname) - 1] = '\0';

		*fOutput << LPS_RECEIVE_CONTROL_FILE << size << ' ' << cfname << '\n' << flush;

		checkAck();
	}
}

void
LpsClient::receiveDataFile(int size, const char *name) throw(LPSException)
{
	if (fConnected) {

		char dfname[32];
		strncpy(dfname, name, sizeof(dfname));
		dfname[sizeof(dfname) - 1] = '\0';

		*fOutput << LPS_RECEIVE_DATA_FILE << size << ' ' << dfname << '\n' << flush;

		checkAck();
	}
}


void
LpsClient::transferData(const char *buffer, int size) throw(LPSException)
{
	if (fConnected) {

		if (size < 0) {
			size = strlen(buffer);
		}

		if (!fOutput->write(buffer, size)) {
			close();
			throw(LPSException("error talking to lpd server"));
		}
	}
}


void
LpsClient::transferData(istream &is, int size) throw(LPSException)
{
	if (fConnected) {

		if (size < 0) {
			is.seekg(0, ios::end);
			size = is.tellg();
			is.seekg(0, ios::beg);
		}

		char c;
		while (is.get(c)) {
			if (!fOutput->put(c)) {
				close();
				throw(LPSException("error reading file."));
				return;
			}
		}
	}
}


void
LpsClient::endTransfer() throw(LPSException)
{
	if (fConnected) {
		*fOutput << LPS_END_TRANSFER << flush;
		checkAck();
	}
}


void
LpsClient::checkAck() throw(LPSException)
{
	if (fConnected) {

		char c;

		if (!fInput->get(c)) {
			close();
			throw(LPSException("server not responding."));
			return;
		}

		switch (c) {
			case LPS_OK:
				break;
			case LPS_ERROR:
				close();
				throw(LPSException("server error."));
				break;
			case LPS_NO_SPOOL_SPACE:
				close();
				throw(LPSException("not enough spool space on server."));
				break;
		}
	}
}
