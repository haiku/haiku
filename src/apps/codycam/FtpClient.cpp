#include "FtpClient.h"

FtpClient::FtpClient()
{
	m_control = 0;
	m_state = 0;
	m_data = 0;
}



FtpClient::~FtpClient()
{
	delete m_control;
	delete m_data;
}


bool FtpClient::cd(const string &dir)
{
	bool rc = false;
	int code, codetype;
	string cmd = "CWD ", replystr;

	
	cmd += dir;
	
	if(dir.length() == 0)
		cmd += '/';
		
	if(p_sendRequest(cmd) == true)
	{
		if(p_getReply(replystr, code, codetype) == true)
		{
			if(codetype == 2)
				rc = true;
		}
	}
	return rc;
}


bool FtpClient::ls(string &listing)
{
	bool rc = false;
	string cmd, replystr;
	int code, codetype, numread;
	char buf[513];
	
	cmd = "TYPE A";
		
	if(p_sendRequest(cmd))
		p_getReply(replystr, code, codetype);

	if(p_openDataConnection())
	{
		cmd = "LIST";
			
		if(p_sendRequest(cmd))
		{
			if(p_getReply(replystr, code, codetype))
			{
				if(codetype <= 2)
				{
					if(p_acceptDataConnection())
					{
						numread = 1;
						while(numread > 0)
						{
							memset(buf, 0, sizeof(buf));
							numread = m_data->Receive(buf, sizeof(buf) - 1);
							listing += buf;
							printf(buf);
						}
						if(p_getReply(replystr, code, codetype))
						{
							if(codetype <= 2)
							{
								rc = true;
							}
						}
					}
				}
			}
		}
	}

	delete m_data;
	m_data = 0;

	return rc;
}


bool FtpClient::pwd(string &dir)
{
	bool rc = false;
	int code, codetype;
	string cmd = "PWD", replystr;
	long i;
	
	if(p_sendRequest(cmd) == true)
	{
		if(p_getReply(replystr, code, codetype) == true)
		{
			if(codetype == 2)
			{
				i = replystr.find('"');
				if(i != -1)
				{
					i++;
					dir = replystr.substr(i, replystr.find('"') - i);
					rc = true;
				}
			}
		}
	}

	return rc;
}


bool FtpClient::connect(const string &server, const string &login, const string &passwd)
{
	bool rc = false;
	int code, codetype;
	string cmd, replystr;
	BNetAddress addr;
	
	delete m_control;
	delete m_data;
	
	
	m_control = new BNetEndpoint;
	
	if(m_control->InitCheck() != B_NO_ERROR)
		return false;
		
	addr.SetTo(server.c_str(), "tcp", "ftp");
	if(m_control->Connect(addr) == B_NO_ERROR)
	{
		//
		// read the welcome message, do the login
		//
		
		if(p_getReply(replystr, code, codetype))
		{
			if(code != 421 && codetype != 5)
			{
				cmd = "USER "; cmd += login;
				p_sendRequest(cmd);
				if(p_getReply(replystr, code, codetype))
				{
					switch(code)
					{
						case 230:	
						case 202:	
							rc = true;
						break;
						
						case 331:  // password needed
							cmd = "PASS "; cmd += passwd;
							p_sendRequest(cmd);
							if(p_getReply(replystr, code, codetype))
							{
								if(codetype == 2)
								{
									rc = true;
								}
							}
						break;
							
						default:
						break;
	
					}
				}
			}
		}
	}	
	
	if(rc == true)
	{
		p_setState(ftp_connected);

	} else {
		delete m_control;
		m_control = 0;
	}
	
	return rc;
}



bool FtpClient::putFile(const string &local, const string &remote, ftp_mode mode)
{
	bool rc = false;
	string cmd, replystr;
	int code, codetype, rlen, slen, i;
	BFile infile(local.c_str(), B_READ_ONLY);
	char buf[8192], sbuf[16384], *stmp;
	
	if(infile.InitCheck() != B_NO_ERROR)
		return false;
	
	if(mode == binary_mode)
		cmd = "TYPE I";
	else
		cmd = "TYPE A";
		
	if(p_sendRequest(cmd))
		p_getReply(replystr, code, codetype);

	try
	{
		if(p_openDataConnection())
		{
			cmd = "STOR ";
			cmd += remote;
			
			if(p_sendRequest(cmd))
			{
				if(p_getReply(replystr, code, codetype))
				{
					if(codetype <= 2)
					{
						if(p_acceptDataConnection())
						{
							rlen = 1;
							while(rlen > 0)
							{
								memset(buf, 0, sizeof(buf));
								memset(sbuf, 0, sizeof(sbuf));
								rlen = infile.Read((void *) buf, sizeof(buf));
								slen = rlen;
								stmp = buf;
								if(mode == ascii_mode)
								{
									stmp = sbuf;
									slen = 0;
									for(i=0;i<rlen;i++)
									{
										if(buf[i] == '\n')
										{
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
								if(slen > 0)
								{
									if(m_data->Send(stmp, slen) < 0)
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
	
	catch(const char *errstr)
	{
	
	}
	
	delete m_data;
	m_data = 0;
	
	if(rc == true)
	{
		p_getReply(replystr, code, codetype);
		rc = (bool) codetype <= 2;
	}
		
	return rc;
}



bool FtpClient::getFile(const string &remote, const string &local, ftp_mode mode)
{
	bool rc = false;
	string cmd, replystr;
	int code, codetype, rlen, slen, i;
	BFile outfile(local.c_str(), B_READ_WRITE | B_CREATE_FILE);
	char buf[8192], sbuf[16384], *stmp;
	bool writeerr = false;
	
	if(outfile.InitCheck() != B_NO_ERROR)
		return false;
	
	if(mode == binary_mode)
		cmd = "TYPE I";
	else
		cmd = "TYPE A";
		
	if(p_sendRequest(cmd))
		p_getReply(replystr, code, codetype);

	
	if(p_openDataConnection())
	{
		cmd = "RETR ";
		cmd += remote;
		
		if(p_sendRequest(cmd))
		{
			if(p_getReply(replystr, code, codetype))
			{
				if(codetype <= 2)
				{
					if(p_acceptDataConnection())
					{
						rlen = 1;
						rc = true;
						while(rlen > 0)
						{
							memset(buf, 0, sizeof(buf));
							memset(sbuf, 0, sizeof(sbuf));
							rlen = m_data->Receive(buf, sizeof(buf));
							
							if(rlen > 0)
							{	
									
								slen = rlen;
								stmp = buf;
								if(mode == ascii_mode)
								{
									stmp = sbuf;
									slen = 0;
									for(i=0;i<rlen;i++)
									{
										if(buf[i] == '\r')
										{
											i++;
										}
										*stmp = buf[i];
										stmp++;
										slen++;
									}
									stmp = sbuf;
								}
								
								if(slen > 0)
								{
									if(outfile.Write(stmp, slen) < 0)
									{
										writeerr = true;
									}									
								}
							}
						}
						
					}
				}
			}
		}
	}
	
	delete m_data;
	m_data = 0;
	
	if(rc == true)
	{						
		p_getReply(replystr, code, codetype);
		rc = (bool) ((codetype <= 2) && (writeerr == false));
	}	
	return rc;
}

//
// Note: this only works for local remote moves, cross filesystem moves
// will not work
//
bool FtpClient::moveFile(const string &oldpath, const string &newpath)
{
	bool rc = false;
	string from = "RNFR ";
	string to = "RNTO ";
	string  replystr;
	int code, codetype;

	from += oldpath;
	to += newpath;
	
	if(p_sendRequest(from))
	{
		if(p_getReply(replystr, code, codetype))
		{
			if(codetype == 3)
			{
				if(p_sendRequest(to))
				{
					if(p_getReply(replystr, code, codetype))
					{
						if(codetype == 2)
							rc = true;
					}
				}
			}
		}
	}
	return rc;
}


void FtpClient::setPassive(bool on)
{
	if(on)
		p_setState(ftp_passive);
	else
		p_clearState(ftp_passive);
}



bool FtpClient::p_testState(unsigned long state)
{
	return (bool) ((m_state & state) != 0);
}



void FtpClient::p_setState(unsigned long state)
{
	m_state |= state;
}



void FtpClient::p_clearState(unsigned long state)
{
	m_state &= ~state;
}




bool FtpClient::p_sendRequest(const string &cmd)
{
	bool rc = false;
	string ccmd = cmd;
	
	if(m_control != 0)
	{
		if(cmd.find("PASS") != string::npos)
			printf("PASS <suppressed>  (real password sent)\n");
		else
			printf("%s\n", ccmd.c_str());
		ccmd += "\r\n";
		if(m_control->Send(ccmd.c_str(), ccmd.length()) >= 0)
		{
			rc = true;
		}
	}
	
	return rc;
}



bool FtpClient::p_getReplyLine(string &line)
{
	bool rc = false;
	int c = 0;
	bool done = false;

	line = "";  // Thanks to Stephen van Egmond for catching a bug here
	
	if(m_control != 0)
	{
		rc = true;
		while(done == false && (m_control->Receive(&c, 1) > 0))
		{
			if(c == EOF || c == xEOF || c == '\n')
			{
				done = true;
			} else {
				if(c == IAC)
				{
					m_control->Receive(&c, 1);
					switch(c)
					{
						unsigned char treply[3];
						case WILL:
						case WONT:
							m_control->Receive(&c, 1);
							treply[0] = IAC;
							treply[1] = DONT;
							treply[2] = c;
							m_control->Send(treply, 3);
						break;
						
						case DO:
						case DONT:
							m_control->Receive(&c, 1);
							m_control->Receive(&c, 1);
							treply[0] = IAC;
							treply[1] = WONT;
							treply[2] = c;
							m_control->Send(treply, 3);
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
					//
					// normal char
					//
					if(c != '\r')
						line += c;
				}
			}
		}		
	}
	
	return rc;
}


bool FtpClient::p_getReply(string &outstr, int &outcode, int &codetype)
{
	bool rc = false;
	string line, tempstr;
	
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
	 
	if((rc = p_getReplyLine(line)) == true)
	{
		outstr = line; 	
		outstr += '\n';
		printf(outstr.c_str());
		tempstr = line.substr(0, 3);
		outcode = atoi(tempstr.c_str());
		
		if(line[3] == '-')
		{
			while((rc = p_getReplyLine(line)) == true)
			{
				outstr += line; 	
				outstr += '\n';
				printf(outstr.c_str());
				//
				// we're done with nnn when we get to a "nnn blahblahblah"
				//
				if((line.find(tempstr) == 0) && line[3] == ' ')
					break;
			}
		}
	}
	
	if(rc == false && outcode != 421)
	{
		outstr += "Remote host has closed the connection.\n";
		outcode = 421;
	}

	if(outcode == 421)
	{
		delete m_control; 
		m_control = 0;
		p_clearState(ftp_connected);
	}
	
	codetype = outcode / 100;
	
	return rc;
}



bool FtpClient::p_openDataConnection()
{
	string host, cmd, repstr;
	unsigned short port;
	BNetAddress addr;
	int i, code, codetype;
	bool rc = false;
	struct sockaddr_in sa;
	
	delete m_data;
	m_data = 0;
	
	m_data = new BNetEndpoint;
	
	if(p_testState(ftp_passive))
	{
		//
		// Here we send a "pasv" command and connect to the remote server
		// on the port it sends back to us
		//
		cmd = "PASV";
		if(p_sendRequest(cmd))
		{
			if(p_getReply(repstr, code, codetype))
			{
		
				if(codetype == 2)
				{
					 //  It should give us something like:
			 		 // "227 Entering Passive Mode (192,168,1,1,10,187)"
					int paddr[6];
					unsigned char ucaddr[6];
					
					i = repstr.find('(');
					i++;

					repstr = repstr.substr(i, repstr.find(')') - i);
					if (sscanf(repstr.c_str(), "%d,%d,%d,%d,%d,%d",
						&paddr[0], &paddr[1], &paddr[2], &paddr[3], 
						&paddr[4], &paddr[5]) != 6) 
						{
							//
							// cannot do passive.  Do a little harmless rercursion here
							//
							p_clearState(ftp_passive);
							return p_openDataConnection();
						}
					for(i=0;i<6;i++)
					{
						ucaddr[i] = (unsigned char) (paddr[i] & 0xff);
					}
					memcpy(&sa.sin_addr, &ucaddr[0], (size_t) 4);
					memcpy(&sa.sin_port, &ucaddr[4], (size_t) 2);
					addr.SetTo(sa);
					if(m_data->Connect(addr) == B_NO_ERROR)
					{
						rc = true;
					}
				}
			}
		} else {
			//
			// cannot do passive.  Do a little harmless rercursion here
			//
			p_clearState(ftp_passive);
			rc = p_openDataConnection();
		}

	} else {
		//
		// Here we bind to a local port and send a PORT command
		//
		if(m_data->Bind() == B_NO_ERROR)
		{
			char buf[255];
			
			m_data->Listen();
			addr = m_data->LocalAddr();
			addr.GetAddr(buf, &port);
			host = buf;
			
			i=0;
			while(i >= 0)
			{
				i = host.find('.', i);
				if(i >= 0)
					host[i] = ',';
			}
			
			sprintf(buf, ",%d,%d", (port & 0xff00) >> 8, port & 0x00ff);
			cmd = "PORT ";
			cmd += host; cmd += buf;
			p_sendRequest(cmd);
			p_getReply(repstr, code, codetype);
			//
			// PORT failure is in the 500-range
			//
			if(codetype == 2)
				rc = true;
		}
	}
	
	return rc;
}


bool FtpClient::p_acceptDataConnection()
{
	BNetEndpoint *ep;
	bool rc = false;

	if(p_testState(ftp_passive) == false)
	{		
		if(m_data != 0)
		{
			ep = m_data->Accept();
			if(ep != 0)
			{
				delete m_data;
				m_data = ep;
				rc = true;
			}		
		}
		
	} else {
		rc = true;
	}	
	
	return rc;		
}





