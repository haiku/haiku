// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __LPSCLIENT_H
#define __LPSCLIENT_H

#ifdef WIN32
#include <istream>
#include <ostream>
#else
#include <istream.h>
#include <ostream.h>
#endif
#include <string>

#if (!__MWERKS__ || defined(WIN32))
using namespace std;
#else 
#define std
#endif

class Socket;

class LPSException {
private:
	string str;
public:
	LPSException(const string &what_arg) : str(what_arg) {}
	const char *what() const { return str.c_str(); }
};

class LpsClient {
public:
	LpsClient(const char *host);
	~LpsClient();
	void connect() throw(LPSException);
	void close();
	void receiveJob(const char *queue) throw(LPSException);
	void receiveControlFile(int cfsize, const char *cfname) throw(LPSException);
	void receiveDataFile(int dfsize, const char *dfname) throw(LPSException);
	void transferData(const char *buffer, int size = -1) throw(LPSException);
	void transferData(istream &is, int size = -1) throw(LPSException);
	void endTransfer() throw(LPSException);
	void checkAck() throw(LPSException);

protected:
	bool connected;

private:
	string  __host;
	Socket  *__sock;
	istream *__is;
	ostream *__os;
};

#endif	/* __LPSCLIENT_H */
