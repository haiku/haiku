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
#define _ZETA_TS_FIND_DIR_ 1
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <stdlib.h>
#include "compat.h"
//#undef DEBUG
//#define DEBUG 4
#include "debug.h"
#include <Debug.h>
#include "ESDEndpoint.h"


ESDEndpoint::ESDEndpoint()
 : BDataIO()
 , fHost(NULL)
 , fPort(ESD_DEFAULT_PORT)
 , fSocket(-1)
{
	CALLED();
	Reset();
}


ESDEndpoint::~ESDEndpoint()
{
	CALLED();
	if (fSocket > -1)
		closesocket(fSocket);
	fSocket = -1;
}


status_t
ESDEndpoint::InitCheck() const
{
	return fInitStatus;
}


void
ESDEndpoint::Reset()
{
	fInitStatus = B_NO_INIT;
	fDefaultCommand = ESD_PROTO_STREAM_PLAY;
	fDefaultCommandSent = false;
	fDefaultFormat = ESD_BITS8 | ESD_MONO;
	fDefaultRate = ESD_DEFAULT_RATE;
	fLatency = 0LL;
}


status_t
ESDEndpoint::SendAuthKey()
{
	CALLED();
	BPath kfPath;
	status_t err;
	off_t size;
	char key[ESD_MAX_KEY];
	err = find_directory(B_USER_SETTINGS_DIRECTORY, &kfPath);
	kfPath.Append("esd_auth");
	BFile keyFile(kfPath.Path(), B_READ_WRITE|B_CREATE_FILE);
	err = keyFile.GetSize(&size);
	if (err < 0)
		return err;
	if (size < ESD_MAX_KEY) {
		keyFile.Seek(0LL, SEEK_SET);
		srand(time(NULL));
		for (int i = 0; i < ESD_MAX_KEY; i++)
			key[i] = (char)(rand() % 256);
		err = keyFile.Write(key, ESD_MAX_KEY);
		if (err < 0)
			return err;
		if (err < ESD_MAX_KEY)
			return EIO;
	}
	err = keyFile.Read(key, ESD_MAX_KEY);
	if (err < 0)
		return err;
	if (err < ESD_MAX_KEY)
		return EIO;
	memcpy(fAuthKey, key, sizeof(esd_key_t));
	return write(fSocket, fAuthKey, ESD_MAX_KEY);
}


bool
ESDEndpoint::Connected() const
{
	return (fInitStatus == B_OK);
}


status_t
ESDEndpoint::Connect(const char *host, uint16 port)
{
	status_t err;
	// set up connection asynchronously
	fHost = host;
	fPort = port;

	err = fConnectThread = spawn_thread(_ConnectThread, "ESDEndpoint Connection", B_LOW_PRIORITY, this);
	if (err < B_OK)
		return err;
	err = resume_thread(fConnectThread);

	// TODO: return now instead and move Connect() call
	//wait_for_thread(fConnectThread, &err);

	return err;
}


status_t
ESDEndpoint::WaitForConnect()
{
	status_t err;
	int32 ret;
	err = wait_for_thread(fConnectThread, &ret);
	if (err < B_OK)
		return err;

	return ret;
}


int32
ESDEndpoint::_ConnectThread(void *_arg)
{
	ESDEndpoint *_this = (ESDEndpoint *)_arg;
	return _this->ConnectThread();
}

int32
ESDEndpoint::ConnectThread(void)
{
	const char *host = fHost.String();
	uint16 port = fPort;
	status_t err;
	int flag;
	struct timeval oldTimeout;
	socklen_t oldTimeoutLen = sizeof(struct timeval);
	struct timeval timeout = { 10, 0 }; // 10s should be enough on a LAN
	CALLED();
	
	struct hostent *he;
	struct sockaddr_in sin;
	he = gethostbyname(host);
	PRINT(("gethostbyname(%s) = %p\n", host, he));
	if (!he)
		return ENOENT;
	memcpy((struct in_addr *)&sin.sin_addr, he->h_addr, sizeof(struct in_addr));
	
	/* close old connection */
	if (fSocket > -1)
		closesocket(fSocket);
	Reset();
	
	fSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fSocket < 0)
		return errno;
	sin.sin_family = AF_INET;
	sin.sin_port = htons( port );
	
	/*
	flag = 128*1024;
	setsockopt(fSocket, SOL_SOCKET, SO_SNDBUF, &flag, sizeof(flag));
	setsockopt(fSocket, SOL_SOCKET, SO_RCVBUF, &flag, sizeof(flag));
	*/

	if (getsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&oldTimeout, &oldTimeoutLen) >= 0) {
		setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	}
	
	err = connect(fSocket, (struct sockaddr *) &sin, sizeof(sin));
	PRINT(("connect: %ld, %s\n", err, strerror(errno)));
	setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&oldTimeout, sizeof(struct timeval));
	if (err < 0)
		return errno;
	
	uint32 cmd;
	uint32 result;
/*	uint32 cmd = ESD_PROTO_CONNECT;
	err = write(fSocket, &cmd, sizeof(cmd));
	if (err < 0)
		return errno;
	if (err < sizeof(cmd))
		return EIO;
*/	
	/* send authentification key */
	err = SendAuthKey();
	if (err < 0)
		return errno;
	
	/* send endian tag, wait for 'ok' and measure the round-trip time 
	 * to account for the network latency
	 */
	bigtime_t ping = system_time();

	cmd = ESD_ENDIAN_TAG;
	err = write(fSocket, &cmd, sizeof(cmd));
	if (err < 0)
		return errno;
	if ((unsigned)err < sizeof(cmd))
		return EIO;
	
	read(fSocket, &result, sizeof(result));
	if (result != 1)
		fprintf(stderr, "ESDEndpoint::Connect: didn't get ok from ESD_PROTO_INIT (%ld)!\n", result);

	ping = (system_time() - ping) / 2; /* approximate one-way trip time */
	fLatency = ping;
	
	/* ask the server for its own latency
	 * sadly it seems to only give a fixed value,
	 * not counting the audio card's buffering into account.
	 * we take another measurement of the round-trip,
	 * and use the mean of the 2.
	 */
	
	ping = system_time();
	
	cmd = ESD_PROTO_LATENCY;
	err = write(fSocket, &cmd, sizeof(cmd));
	if (err < 0)
		return errno;
	if ((unsigned)err < sizeof(cmd))
		return EIO;
	
	read(fSocket, &result, sizeof(result));
	fprintf(stderr, "ESDEndpoint::Connect: ESD_PROTO_LATENCY: %ld\n", result);
	
	ping = (system_time() - ping) / 2; /* approximate one-way trip time */
	
	bigtime_t serverLatency = result * 1000000LL / (ESD_DEFAULT_RATE * 2/*16b*/ * 2/*stereo*/ );
	bigtime_t netLatency = (fLatency + ping) / 2; /* mean of both */
	fprintf(stderr, "ESDEndpoint::Connect: Latency: server: %Ld, net1: %Ld, net2: %Ld\n", serverLatency, fLatency, ping);
	
	fLatency = netLatency + serverLatency;
	
	
	
	flag = 1;
	//int len;
	/* disable Nagle */
	setsockopt(fSocket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
	//setsockopt(fSocket, SOL_SOCKET, SO_NONBLOCK, &flag, sizeof(flag));
	/*
	len = sizeof(flag);
	getsockopt(fSocket, SOL_SOCKET, SO_SNDBUF, &flag, &len);
	fprintf(stderr, "ESDEndpoint::Connect: SNDBUF: %ld\n", flag);
	flag = MIN(TCP_MAXWIN, MAX(flag, (4 * netLatency * (ESD_DEFAULT_RATE*2*2) / 1000000LL)));
	fprintf(stderr, "ESDEndpoint::Connect: SNDBUF: %ld\n", flag);
	setsockopt(fSocket, SOL_SOCKET, SO_SNDBUF, &flag, sizeof(flag));
	*/
	


	// TODO: get default format

	fInitStatus = B_OK;

	return B_OK;
}

status_t
ESDEndpoint::Disconnect()
{
	CALLED();
	if (fSocket > -1)
		closesocket(fSocket);
	fSocket = -1;
	return B_OK;
}

status_t
ESDEndpoint::SetCommand(esd_command_t cmd)
{
	CALLED();
	if (fDefaultCommandSent)
		return EALREADY;
	fDefaultCommand = cmd;
	return B_OK;
}

status_t
ESDEndpoint::SetFormat(int bits, int channels, float rate)
{
	esd_format_t fmt = 0;
	CALLED();
	if (fDefaultCommandSent)
		return EALREADY;
	PRINT(("SetFormat(%d,%d,%f)\n", bits, channels, rate));
	switch (bits) {
	case 8:
		fmt |= ESD_BITS8;
		break;
	case 16:
		fmt |= ESD_BITS16;
		break;
	default:
		return EINVAL;
	}
	switch (channels) {
	case 1:
		fmt |= ESD_MONO;
		break;
	case 2:
		fmt |= ESD_STEREO;
		break;
	default:
		return EINVAL;
	}
	fmt |= ESD_STREAM | ESD_FUNC_PLAY;
	PRINT(("SetFormat: %08lx\n", (long)fmt));
	fDefaultFormat = fmt;
	fDefaultRate = rate;
	return B_OK;
}

status_t
ESDEndpoint::GetServerInfo()
{
	CALLED();
	struct serverinfo {
		uint32 ver;
		uint32 rate;
		uint32 fmt;
	} si;
	status_t err;
	err = SendCommand(ESD_PROTO_SERVER_INFO, (const uint8 *)&si, 0, (uint8 *)&si, sizeof(si));
	if (err < 0)
		return err;
	PRINT(("err 0x%08lx, version: %lu, rate: %lu, fmt: %lu\n", err, si.ver, si.rate, si.fmt));
	return B_OK;
}


void
ESDEndpoint::GetFriendlyName(BString &name)
{
	name = "ESounD Out";
	name << " (" << Host();
	name << ":" << Port() << ")";

}

bool
ESDEndpoint::CanSend()
{
	CALLED();
	return fDefaultCommandSent;
}

ssize_t
ESDEndpoint::Read(void *buffer, size_t size)
{
	CALLED();
	return EINVAL;
}

ssize_t
ESDEndpoint::Write(const void *buffer, size_t size)
{
	status_t err = B_OK;
	CALLED();
	if (!fDefaultCommandSent)
		err = SendDefaultCommand();
	if (err < B_OK)
		return err;
	//PRINT(("write(fSocket, buffer, %d)\n", size));
	//fprintf(stderr, "ESDEndpoint::Write(, %d) %s\n", size, (size%2)?"ODD BUFFER SIZE":"");
	if (fDefaultFormat & ESD_BITS16) {
		size /= 2;
		size *= 2;
	}
	err = write(fSocket, buffer, size);
	if ((unsigned)err != size) {
		fprintf(stderr, "ESDEndpoint::Write: sent only %ld of %ld!\n", err, size);
		if (err < 0)
			fprintf(stderr, "ESDEndpoint::Write: %s\n", strerror(errno));
	}
	//PRINT(("write(fSocket, buffer, %d): %s\n", size, strerror(err)));
	if (err < B_OK)
		return errno;
	return err;
}

status_t
ESDEndpoint::SendCommand(esd_command_t cmd, const uint8 *obuf, size_t olen, uint8 *ibuf, size_t ilen)
{
	status_t err;
	CALLED();
	err = send(fSocket, &cmd, sizeof(cmd), 0);
	if (err < B_OK)
		return errno;
	if (obuf && olen) {
		err = send(fSocket, obuf, olen, 0);
		if (err < B_OK)
			return errno;
	}
	err = B_OK;
	if (ibuf && ilen) {
		err = recv(fSocket, ibuf, ilen, 0);
		if (err < B_OK)
			return errno;
		/* return received len */
	}
	return err;
}

status_t
ESDEndpoint::SendDefaultCommand()
{
	status_t err;
	struct {
		esd_format_t format;
		esd_rate_t rate;
		char name[ESD_MAX_NAME];
	} c;
	CALLED();
	if (fDefaultCommandSent)
		return EALREADY;
	c.format = fDefaultFormat;
	c.rate = fDefaultRate;
	strcpy(c.name, "BeOS/Haiku/ZETA Media Kit output");
	err = SendCommand(fDefaultCommand, (uint8 *)&c, sizeof(c), NULL, 0);
	if (err < B_OK)
		return err;
	PRINT(("SendCommand: %s\n", strerror(err)));
	fDefaultCommandSent = true;
	return B_OK;
}

