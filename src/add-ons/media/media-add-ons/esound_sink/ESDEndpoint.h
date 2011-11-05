/*
 * ESounD media addon for BeOS
 *
 * Copyright (c) 2006 François Revol (revol@free.fr)
 * 
 * Based on Multi Audio addon for Haiku,
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice, 
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation 
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef ESDENDPOINT_H
#define ESDENDPOINT_H

#include "esdproto.h"
#include <DataIO.h>
#include <String.h>

//#define ESD_FMT 8
#define ESD_FMT 16

class ESDEndpoint : public BDataIO {
public:
	ESDEndpoint();
	~ESDEndpoint();
	
	/*  */
status_t	InitCheck() const;
void		Reset();
status_t	SendAuthKey();

bool		Connected() const;
status_t	Connect(const char *host, uint16 port=ESD_DEFAULT_PORT);
status_t	WaitForConnect();
status_t	Disconnect();
	
	/* set the default command and format for BDataIO interface */
status_t	SetCommand(esd_command_t cmd=ESD_PROTO_STREAM_PLAY);
status_t	SetFormat(int bits, int channels, float rate=ESD_DEFAULT_RATE);

status_t	GetServerInfo();

	/*  */

bigtime_t	GetLatency() const { return fLatency; };
const char	*Host() const { return fHost.String(); };
uint16		Port() const { return fPort; };
void		GetFriendlyName(BString &name);

bool		CanSend();

	/* BDataIO */
	
virtual ssize_t		Read(void *buffer, size_t size);
virtual ssize_t		Write(const void *buffer, size_t size);

	status_t	SendCommand(esd_command_t cmd, const uint8 *obuf, size_t olen, uint8 *ibuf, size_t ilen);
	status_t	SendDefaultCommand();
private:

	static int32	_ConnectThread(void *_arg);
	int32			ConnectThread(void);

	status_t		fInitStatus;
	thread_id		fConnectThread;
	BString			fHost;
	uint16			fPort;
	int				fSocket;
	esd_key_t		fAuthKey;
	esd_command_t	fDefaultCommand;
	bool			fDefaultCommandSent;
	esd_format_t	fDefaultFormat;
	esd_rate_t		fDefaultRate;
	bigtime_t		fLatency;
};


#endif /* ESDENDPOINT_H */
