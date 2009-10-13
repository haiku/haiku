// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __LPSCLIENT_H
#define __LPSCLIENT_H

#include <iostream>
#include <string>


using namespace std;


class Socket;


class LPSException {
public:
					LPSException(const string& what)
						:
						fWhat(what)
					{
					}

	const char *	what() const
					{
						return fWhat.c_str();
					}

private:
	string fWhat;

};


class LpsClient {
public:
					LpsClient(const char* host);
					~LpsClient();
	void			connect() throw(LPSException);
	void			close();
	void			receiveJob(const char* queue) throw(LPSException);
	void			receiveControlFile(int cfsize, const char* cfname)
						throw(LPSException);
	void			receiveDataFile(int dfsize, const char* dfname)
						throw(LPSException);
	void			transferData(const char* buffer, int size = -1)
						throw(LPSException);
	void			transferData(istream& is, int size = -1)
						throw(LPSException);
	void			endTransfer() throw(LPSException);
	void			checkAck() throw(LPSException);

private:
	bool		fConnected;
	string  	fHost;
	Socket* 	fSock;
	istream*	fInput;
	ostream*	fOutput;
};


#endif	/* __LPSCLIENT_H */
