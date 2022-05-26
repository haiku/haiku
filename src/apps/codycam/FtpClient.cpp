/*
 * Copyright 1998-1999 Be, Inc. All Rights Reserved.
 * Copyright 2003-2019 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FtpClient.h"

#include <stdlib.h>
#include <string.h>

#include <Catalog.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FtpClient"


FtpClient::FtpClient()
	:
	FileUploadClient(),
	fState(0),
	fControl(NULL),
	fData(NULL)
{
}


FtpClient::~FtpClient()
{
	delete fControl;
	delete fData;
}


bool
FtpClient::ChangeDir(const string& dir)
{
	bool rc = false;
	int code, codeType;
	string cmd = "CWD ";
	string replyString;

	cmd += dir;

	if (dir.length() == 0)
		cmd += '/';

	if (_SendRequest(cmd) == true) {
		if (_GetReply(replyString, code, codeType) == true) {
			if (codeType == 2)
				rc = true;
		}
	}
	return rc;
}


bool
FtpClient::ListDirContents(string& listing)
{
	bool rc = false;
	string cmd, replyString;
	int code, codeType, numRead;
	char buf[513];

	cmd = "TYPE A";

	if (_SendRequest(cmd))
		_GetReply(replyString, code, codeType);

	if (_OpenDataConnection()) {
		cmd = "LIST";

		if (_SendRequest(cmd)) {
			if (_GetReply(replyString, code, codeType)) {
				if (codeType <= 2) {
					if (_AcceptDataConnection()) {
						numRead = 1;
						while (numRead > 0) {
							memset(buf, 0, sizeof(buf));
							numRead = fData->Receive(buf, sizeof(buf) - 1);
							listing += buf;
							printf("%s", buf);
						}
						if (_GetReply(replyString, code, codeType)) {
							if (codeType <= 2)
								rc = true;
						}
					}
				}
			}
		}
	}

	delete fData;
	fData = NULL;

	return rc;
}


bool
FtpClient::PrintWorkingDir(string& dir)
{
	bool rc = false;
	int code, codeType;
	string cmd = "PWD";
	string replyString;
	long i;

	if (_SendRequest(cmd) == true) {
		if (_GetReply(replyString, code, codeType) == true) {
			if (codeType == 2) {
				i = replyString.find('"');
				if (i != -1) {
					i++;
					dir = replyString.substr(i, replyString.find('"') - i);
					rc = true;
				}
			}
		}
	}

	return rc;
}


bool
FtpClient::Connect(const string& server, const string& login,
	const string& passwd)
{
	bool rc = false;
	int code, codeType;
	string cmd, replyString;
	BNetAddress addr;

	delete fControl;
	delete fData;

	fControl = new BNetEndpoint();

	if (fControl->InitCheck() != B_NO_ERROR)
		return false;

	addr.SetTo(server.c_str(), "tcp", "ftp");
	if (fControl->Connect(addr) == B_NO_ERROR) {
		// read the welcome message, do the login

		if (_GetReply(replyString, code, codeType)) {
			if (code != 421 && codeType != 5) {
				cmd = "USER ";
				cmd += login;
				_SendRequest(cmd);

				if (_GetReply(replyString, code, codeType)) {
					switch (code) {
						case 230:
						case 202:
							rc = true;
							break;

						case 331:  // password needed
							cmd = "PASS ";
							cmd += passwd;
							_SendRequest(cmd);
							if (_GetReply(replyString, code, codeType)) {
								if (codeType == 2)
									rc = true;
							}
							break;

						default:
							break;

					}
				}
			}
		}
	}

	if (rc == true)
		_SetState(ftp_connected);
	else {
		delete fControl;
		fControl = NULL;
	}

	return rc;
}


bool
FtpClient::PutFile(const string& local, const string& remote, ftp_mode mode)
{
	bool rc = false;
	string cmd, replyString;
	int code, codeType, rlen, slen, i;
	BFile infile(local.c_str(), B_READ_ONLY);
	char buf[8192];
	char sbuf[16384];
	char* stmp;

	if (infile.InitCheck() != B_NO_ERROR)
		return false;

	if (mode == binary_mode)
		cmd = "TYPE I";
	else
		cmd = "TYPE A";

	if (_SendRequest(cmd))
		_GetReply(replyString, code, codeType);

	try {
		if (_OpenDataConnection()) {
			cmd = "STOR ";
			cmd += remote;

			if (_SendRequest(cmd)) {
				if (_GetReply(replyString, code, codeType)) {
					if (codeType <= 2) {
						if (_AcceptDataConnection()) {
							rlen = 1;
							while (rlen > 0) {
								memset(buf, 0, sizeof(buf));
								memset(sbuf, 0, sizeof(sbuf));
								rlen = infile.Read((void*)buf, sizeof(buf));
								slen = rlen;
								stmp = buf;
								if (mode == ascii_mode) {
									stmp = sbuf;
									slen = 0;
									for (i = 0; i < rlen; i++) {
										if (buf[i] == '\n') {
											*stmp = '\r';
											stmp++;
											slen++;
										}
										*stmp = buf[i];
										stmp++;
										slen++;
									}
									stmp = sbuf;
								}
								if (slen > 0) {
									if (fData->Send(stmp, slen) < 0)
										throw "bail";
								}
							}

							rc = true;
						}
					}
				}
			}
		}
	}

	catch(const char* errorString)
	{
	}

	delete fData;
	fData = NULL;

	if (rc) {
		_GetReply(replyString, code, codeType);
		rc = codeType <= 2;
	}

	return rc;
}


bool
FtpClient::GetFile(const string& remote, const string& local, ftp_mode mode)
{
	bool rc = false;
	string cmd, replyString;
	int code, codeType, rlen, slen, i;
	BFile outfile(local.c_str(), B_READ_WRITE | B_CREATE_FILE);
	char buf[8192];
	char sbuf[16384];
	char* stmp;
	bool writeError = false;

	if (outfile.InitCheck() != B_NO_ERROR)
		return false;

	if (mode == binary_mode)
		cmd = "TYPE I";
	else
		cmd = "TYPE A";

	if (_SendRequest(cmd))
		_GetReply(replyString, code, codeType);

	if (_OpenDataConnection()) {
		cmd = "RETR ";
		cmd += remote;

		if (_SendRequest(cmd)) {
			if (_GetReply(replyString, code, codeType)) {
				if (codeType <= 2) {
					if (_AcceptDataConnection()) {
						rlen = 1;
						rc = true;
						while (rlen > 0) {
							memset(buf, 0, sizeof(buf));
							memset(sbuf, 0, sizeof(sbuf));
							rlen = fData->Receive(buf, sizeof(buf));

							if (rlen > 0) {

								slen = rlen;
								stmp = buf;
								if (mode == ascii_mode) {
									stmp = sbuf;
									slen = 0;
									for (i = 0; i < rlen; i++) {
										if (buf[i] == '\r')
											i++;
										*stmp = buf[i];
										stmp++;
										slen++;
									}
									stmp = sbuf;
								}

								if (slen > 0) {
									if (outfile.Write(stmp, slen) < 0)
										writeError = true;
								}
							}
						}
					}
				}
			}
		}
	}

	delete fData;
	fData = NULL;

	if (rc) {
		_GetReply(replyString, code, codeType);
		rc = (codeType <= 2 && writeError == false);
	}
	return rc;
}


// Note: this only works for local remote moves, cross filesystem moves
// will not work
bool
FtpClient::MoveFile(const string& oldPath, const string& newPath)
{
	bool rc = false;
	string from = "RNFR ";
	string to = "RNTO ";
	string  replyString;
	int code, codeType;

	from += oldPath;
	to += newPath;

	if (_SendRequest(from)) {
		if (_GetReply(replyString, code, codeType)) {
			if (codeType == 3) {
				if (_SendRequest(to)) {
					if (_GetReply(replyString, code, codeType)) {
						if(codeType == 2)
							rc = true;
					}
				}
			}
		}
	}
	return rc;
}


bool
FtpClient::Chmod(const string& path, const string& mod)
{
	bool rc = false;
	int code, codeType;
	string cmd = "SITE CHMOD ";
	string replyString;

	cmd += mod;
	cmd += " ";
	cmd += path;

	if (path.length() == 0)
		cmd += '/';
	printf(B_TRANSLATE("cmd: '%s'\n"), cmd.c_str());

	if (_SendRequest(cmd) == true) {
		if (_GetReply(replyString, code, codeType) == true) {
			printf(B_TRANSLATE("reply: %d, %d\n"), code, codeType);
			if (codeType == 2)
				rc = true;
		}
	}
	return rc;
}


void
FtpClient::SetPassive(bool on)
{
	if (on)
		_SetState(ftp_passive);
	else
		_ClearState(ftp_passive);
}


bool
FtpClient::_TestState(unsigned long state)
{
	return ((fState & state) != 0);
}


void
FtpClient::_SetState(unsigned long state)
{
	fState |= state;
}


void
FtpClient::_ClearState(unsigned long state)
{
	fState &= ~state;
}


bool
FtpClient::_SendRequest(const string& cmd)
{
	bool rc = false;
	string ccmd = cmd;

	if (fControl != 0) {
		if (cmd.find("PASS") != string::npos) {
			puts(B_TRANSLATE("PASS <suppressed>  (real password sent)"));
		} else {
			puts(ccmd.c_str());
		}

		ccmd += "\r\n";
		if (fControl->Send(ccmd.c_str(), ccmd.length()) >= 0)
			rc = true;
	}

	return rc;
}


bool
FtpClient::_GetReplyLine(string& line)
{
	bool rc = false;
	int c = 0;
	bool done = false;

	line = "";
		// Thanks to Stephen van Egmond for catching a bug here

	if (fControl != NULL) {
		rc = true;
		while (done == false && fControl->Receive(&c, 1) > 0) {
			if (c == EOF || c == xEOF || c == '\n') {
				done = true;
			} else {
				if (c == IAC) {
					fControl->Receive(&c, 1);
					switch (c) {
						unsigned char treply[3];
						case WILL:
						case WONT:
							fControl->Receive(&c, 1);
							treply[0] = IAC;
							treply[1] = DONT;
							treply[2] = c;
							fControl->Send(treply, 3);
						break;

						case DO:
						case DONT:
							fControl->Receive(&c, 1);
							fControl->Receive(&c, 1);
							treply[0] = IAC;
							treply[1] = WONT;
							treply[2] = c;
							fControl->Send(treply, 3);
						break;

						case EOF:
						case xEOF:
							done = true;
						break;

						default:
							line += c;
						break;
					}
				} else {
					// normal char
					if (c != '\r')
						line += c;
				}
			}
		}
	}

	return rc;
}


bool
FtpClient::_GetReply(string& outString, int& outCode, int& codeType)
{
	bool rc = false;
	string line, tempString;

	//
	// comment from the ncftp source:
	//

	/* RFC 959 states that a reply may span multiple lines.  A single
	 * line message would have the 3-digit code <space> then the msg.
	 * A multi-line message would have the code <dash> and the first
	 * line of the msg, then additional lines, until the last line,
	 * which has the code <space> and last line of the msg.
	 *
	 * For example:
	 *	123-First line
	 *	Second line
	 *	234 A line beginning with numbers
	 *	123 The last line
	 */

	rc = _GetReplyLine(line);
	if (rc == true) {
		outString = line;
		puts(outString.c_str());
		outString += '\n';
		tempString = line.substr(0, 3);
		outCode = atoi(tempString.c_str());

		if (line[3] == '-') {
			rc = _GetReplyLine(line);
			while (rc == true) {
				outString += line;
				puts(outString.c_str());
				outString += '\n';
				// we're done with nnn when we get to a "nnn blahblahblah"
				if ((line.find(tempString) == 0) && line[3] == ' ')
					break;

				rc = _GetReplyLine(line);
			}
		}
	}

	if (!rc && outCode != 421) {
		outString += B_TRANSLATE("Remote host has closed the connection.\n");
		outCode = 421;
	}

	if (outCode == 421) {
		delete fControl;
		fControl = NULL;
		_ClearState(ftp_connected);
	}

	codeType = outCode / 100;

	return rc;
}


bool
FtpClient::_OpenDataConnection()
{
	string host, cmd, replyString;
	unsigned short port;
	BNetAddress addr;
	int i, code, codeType;
	bool rc = false;
	struct sockaddr_in sa;

	delete fData;
	fData = NULL;

	fData = new BNetEndpoint();

	if (_TestState(ftp_passive)) {
		// Here we send a "pasv" command and connect to the remote server
		// on the port it sends back to us
		cmd = "PASV";
		if (_SendRequest(cmd)) {
			if (_GetReply(replyString, code, codeType)) {

				if (codeType == 2) {
					//  It should give us something like:
					// "227 Entering Passive Mode (192,168,1,1,10,187)"
					int paddr[6];
					unsigned char ucaddr[6];

					i = replyString.find('(');
					i++;

					replyString = replyString.substr(i,
						replyString.find(')') - i);
					if (sscanf(replyString.c_str(), "%d,%d,%d,%d,%d,%d",
						&paddr[0], &paddr[1], &paddr[2], &paddr[3],
						&paddr[4], &paddr[5]) != 6) {
							// Cannot do passive.
							// Do a little harmless rercursion here.
							_ClearState(ftp_passive);
							return _OpenDataConnection();
						}

					for (i = 0; i < 6; i++)
						ucaddr[i] = (unsigned char)(paddr[i] & 0xff);

					memcpy(&sa.sin_addr, &ucaddr[0], (size_t) 4);
					memcpy(&sa.sin_port, &ucaddr[4], (size_t) 2);
					addr.SetTo(sa);
					if (fData->Connect(addr) == B_NO_ERROR)
						rc = true;

				}
			}
		} else {
			// cannot do passive.  Do a little harmless rercursion here
			_ClearState(ftp_passive);
			rc = _OpenDataConnection();
		}

	} else {
		// Here we bind to a local port and send a PORT command
		if (fData->Bind() == B_NO_ERROR) {
			char buf[255];

			fData->Listen();
			addr = fData->LocalAddr();
			addr.GetAddr(buf, &port);
			host = buf;

			i = 0;
			while (i >= 0) {
				i = host.find('.', i);
				if (i >= 0)
					host[i] = ',';
			}

			sprintf(buf, ",%d,%d", (port & 0xff00) >> 8, port & 0x00ff);
			cmd = "PORT ";
			cmd += host;
			cmd += buf;
			_SendRequest(cmd);
			_GetReply(replyString, code, codeType);
			// PORT failure is in the 500-range
			if (codeType == 2)
				rc = true;
		}
	}

	return rc;
}


bool
FtpClient::_AcceptDataConnection()
{
	BNetEndpoint* endPoint;
	bool rc = false;

	if (_TestState(ftp_passive) == false) {
		if (fData != NULL) {
			endPoint = fData->Accept();
			if (endPoint != NULL) {
				delete fData;
				fData = endPoint;
				rc = true;
			}
		}

	}
	else
		rc = true;

	return rc;
}
