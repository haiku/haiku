// Sun, 18 Jun 2000
// Y.Takagi

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
	#include <sstream>
#else	/* BeOS */
	#include <cerrno>
	#include <net/socket.h>
	#include <net/netdb.h>
	#include "_sstream"
#endif

#include <iomanip>
#include <algorithm>
#include "LpsClient.h"
#include "Socket.h"
//#include "DbgMsg.h"

#if (!__MWERKS__ || defined(WIN32))
using namespace std;
#else 
#define std
#endif

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

#ifdef WIN32
	#define EADDRINUSE	WSAEADDRINUSE
	#define ERRNO	WSAGetLastError()
#else
	#define ERRNO	errno
#endif


LpsClient::LpsClient(const char *host)
	: __host(host), __sock(NULL), connected(false)
{
}

LpsClient::~LpsClient()
{
	close();
}

void LpsClient::connect() throw(LPSException)
{
//	DBGMSG(("connect\n"));

	for (int localPort = LPS_CLIENT_PORT_S ; localPort <=  LPS_CLIENT_PORT_E ; localPort++) {
		if (__sock) {
			delete __sock;
		}
		__sock = new Socket(__host.c_str(), LPS_SERVER_PORT, localPort);
		if (__sock->good()) {
			__is = &__sock->getInputStream();
			__os = &__sock->getOutputStream();
			connected = true;
			return;
		}
	}

	throw(LPSException(__sock->getLastError()));
}

void LpsClient::close()
{
//	DBGMSG(("close\n"));

	connected = false;
	if (__sock) {
		delete __sock;
		__sock = NULL;
	}
}

void LpsClient::receiveJob(const char *printer) throw(LPSException)
{
//	DBGMSG(("tell_receive_job\n"));

	if (connected) {
		*__os << LPS_PRINT_JOB << printer << '\n' << flush;
		checkAck();
	}
}

void LpsClient::receiveControlFile(int size, const char *name) throw(LPSException)
{
//	DBGMSG(("tell_receive_control_file\n"));

	if (connected) {

		char cfname[32];
		strncpy(cfname, name, sizeof(cfname));
		cfname[sizeof(cfname) - 1] = '\0';

		*__os << LPS_RECEIVE_CONTROL_FILE << size << ' ' << cfname << '\n' << flush;

		checkAck();
	}
}

void LpsClient::receiveDataFile(int size, const char *name) throw(LPSException)
{
//	DBGMSG(("tell_receive_data_file\n"));

	if (connected) {

		char dfname[32];
		strncpy(dfname, name, sizeof(dfname));
		dfname[sizeof(dfname) - 1] = '\0';

		*__os << LPS_RECEIVE_DATA_FILE << size << ' ' << dfname << '\n' << flush;

		checkAck();
	}
}

void LpsClient::transferData(const char *buffer, int size) throw(LPSException)
{
//	DBGMSG(("send: %d\n", size));

	if (connected) {

		if (size < 0) {
			size = strlen(buffer);
		}

		if (!__os->write(buffer, size)) {
			close();
			throw(LPSException("error talking to lpd server"));
		}
	}
}

void LpsClient::transferData(istream &is, int size) throw(LPSException)
{
//	DBGMSG(("send: %d\n", size));

	if (connected) {

		if (size < 0) {
			is.seekg(0, ios::end);
#if __MWERKS__
			size = is.tellg().offset();
#else
			size = is.tellg();
#endif
			is.seekg(0, ios::beg);
		}

		char c;
		while (is.get(c)) {
			if (!__os->put(c)) {
				close();
				throw(LPSException("error reading file."));
				return;
			}
		}
	}
}

void LpsClient::endTransfer() throw(LPSException)
{
//	DBGMSG(("tell_end_transfer\n"));

	if (connected) {
		*__os << LPS_END_TRANSFER << flush;
		checkAck();
	}
}

void LpsClient::checkAck() throw(LPSException)
{
//	DBGMSG(("check_ack\n"));

	if (connected) {

		char c;

		if (!__is->get(c)) {
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
