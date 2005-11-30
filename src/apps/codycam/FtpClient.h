#include <string>
#include <File.h>
#include <stdio.h>
#include <NetworkKit.h>

using std::string;

class FtpClient
{
public:
	FtpClient();
	~FtpClient();
	
	enum ftp_mode
	{
		binary_mode,
		ascii_mode
	};
	
	bool connect(const string &server, const string &login, const string &passwd);
	bool putFile(const string &local, const string &remote, ftp_mode mode = binary_mode);
	bool getFile(const string &remote, const string &local, ftp_mode mode = binary_mode);
	bool moveFile(const string &oldpath, const string &newpath);
	bool cd(const string &dir);
	bool pwd(string &dir);
	bool ls(string &listing);

	void setPassive(bool on);
	
protected:
	enum {
		ftp_complete = 1UL,
		ftp_connected = 2,
		ftp_passive = 4
	};
	
	unsigned long m_state;
	bool p_testState(unsigned long state);
	void p_setState(unsigned long state);
	void p_clearState(unsigned long state);
	
	bool p_sendRequest(const string &cmd);
	bool p_getReply(string &outstr, int &outcode, int &codetype);
	bool p_getReplyLine(string &line);
	bool p_openDataConnection();
	bool p_acceptDataConnection();
	
	BNetEndpoint *m_control;
	BNetEndpoint *m_data;

};

/*
 * Definitions for the TELNET protocol. Snarfed from the BSD source.
 */
#define IAC     255             
#define DONT    254             
#define DO      253             
#define WONT    252             
#define WILL    251             
#define xEOF    236            
      
