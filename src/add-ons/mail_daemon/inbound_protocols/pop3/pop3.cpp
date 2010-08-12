/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

//! POP3Protocol - implementation of the POP3 protocol

#include "pop3.h"

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#ifndef HAIKU_TARGET_PLATFORM_BEOS
	// These headers don't exist in BeOS R5.
#	include <arpa/inet.h>
#	include <sys/select.h>
#endif

#if USE_SSL
#	include <openssl/ssl.h>
#	include <openssl/rand.h>
#	include <openssl/md5.h>
#else
#	include "md5.h"
#endif

#include <DataIO.h>
#include <Alert.h>
#include <Debug.h>

#include <status.h>
#include <StringList.h>
#include <ProtocolConfigView.h>
#include <ChainRunner.h>

#include <MDRLanguage.h>

#define POP3_RETRIEVAL_TIMEOUT 60000000
#define CRLF	"\r\n"


POP3Protocol::POP3Protocol(BMessage *settings, BMailChainRunner *status)
	: SimpleMailProtocol(settings,status),
	fNumMessages(-1),
	fMailDropSize(0)
{
#ifdef USE_SSL
	fUseSSL = (settings->FindInt32("flavor") == 1);
#endif
	Init();
}


POP3Protocol::~POP3Protocol()
{
#ifdef USE_SSL
	if (!fUseSSL || fSSL)
#endif
	SendCommand("QUIT" CRLF);

#ifdef USE_SSL
	if (fUseSSL) {
		if (fSSL)
			SSL_shutdown(fSSL);
		if (fSSLContext)
			SSL_CTX_free(fSSLContext);
	}
#endif

#ifndef HAIKU_TARGET_PLATFORM_BEOS
	close(fSocket);
#else
	closesocket(fSocket);
#endif
}


status_t
POP3Protocol::Open(const char *server, int port, int)
{
	runner->ReportProgress(0, 0, MDR_DIALECT_CHOICE("Connecting to POP3 server...",
		"POP3サーバに接続しています..."));

	if (port <= 0) {
#ifdef USE_SSL
		port = fUseSSL ? 995 : 110;
#else
		port = 110;
#endif
	}

	fLog = "";

	// Prime the error message
	BString error_msg;
	error_msg << MDR_DIALECT_CHOICE("Error while connecting to server ",
		"サーバに接続中にエラーが発生しました ") << server;
	if (port != 110)
		error_msg << ":" << port;

	uint32 hostIP = inet_addr(server);
		// first see if we can parse it as a numeric address
	if (hostIP == 0 || hostIP == ~0UL) {
		struct hostent * he = gethostbyname(server);
		hostIP = he ? *((uint32*)he->h_addr) : 0;
	}

	if (hostIP == 0) {
		error_msg << MDR_DIALECT_CHOICE(": Connection refused or host not found",
			": ：接続が拒否されたかサーバーが見つかりません");
		runner->ShowError(error_msg.String());

		return B_NAME_NOT_FOUND;
	}

#ifndef HAIKU_TARGET_PLATFORM_BEOS
	fSocket = socket(AF_INET, SOCK_STREAM, 0);
#else
	fSocket = socket(AF_INET, 2, 0);
#endif
	if (fSocket >= 0) {
		struct sockaddr_in saAddr;
		memset(&saAddr, 0, sizeof(saAddr));
		saAddr.sin_family = AF_INET;
		saAddr.sin_port = htons(port);
		saAddr.sin_addr.s_addr = hostIP;
		int result = connect(fSocket, (struct sockaddr *) &saAddr, sizeof(saAddr));
		if (result < 0) {
#ifndef HAIKU_TARGET_PLATFORM_BEOS
			close(fSocket);
#else
			closesocket(fSocket);
#endif
			fSocket = -1;
			error_msg << ": " << strerror(errno);
			runner->ShowError(error_msg.String());
			return errno;
		}
	} else {
		error_msg << ": Could not allocate socket.";
		runner->ShowError(error_msg.String());
		return B_ERROR;
	}

#ifdef USE_SSL
	if (fUseSSL) {
		SSL_library_init();
		SSL_load_error_strings();
		RAND_seed(this,sizeof(POP3Protocol));
		/*--- Because we're an add-on loaded at an unpredictable time, all
			the memory addresses and things contained in ourself are
			esssentially random. */

		fSSLContext = SSL_CTX_new(SSLv23_method());
		fSSL = SSL_new(fSSLContext);
		fSSLBio = BIO_new_socket(fSocket, BIO_NOCLOSE);
		SSL_set_bio(fSSL, fSSLBio, fSSLBio);

		if (SSL_connect(fSSL) <= 0) {
			BString error;
			error << "Could not connect to POP3 server "
				<< settings->FindString("server");
			if (port != 995)
				error << ":" << port;
			error << ". (SSL connection error)";
			runner->ShowError(error.String());
			SSL_CTX_free(fSSLContext);
#ifndef HAIKU_TARGET_PLATFORM_BEOS
			close(fSocket);
#else
			closesocket(fSocket);
#endif
			runner->Stop(true);
			return B_ERROR;
		}
	}
#endif

	BString line;
	status_t err;
#ifndef HAIKU_TARGET_PLATFORM_BEOS
	err = ReceiveLine(line);
#else
	int32 tries = 200000;
		// no endless loop here
	while ((err = ReceiveLine(line)) == 0) {
		if (tries-- < 0)
			return B_ERROR;
	}
#endif

	if (err < 0) {
#ifndef HAIKU_TARGET_PLATFORM_BEOS
		close(fSocket);
#else
		closesocket(fSocket);
#endif
		fSocket = -1;
		error_msg << ": " << strerror(err);
		runner->ShowError(error_msg.String());
		return B_ERROR;
	}

	if (strncmp(line.String(), "+OK", 3) != 0) {
		if (line.Length() > 0) {
			error_msg << MDR_DIALECT_CHOICE(". The server said:\n",
				"サーバのメッセージです\n") << line.String();
		} else
			error_msg << ": No reply.\n";

		runner->ShowError(error_msg.String());
		return B_ERROR;
	}

	fLog = line;
	return B_OK;
}


status_t
POP3Protocol::Login(const char *uid, const char *password, int method)
{
	status_t err;

	BString error_msg;
	error_msg << MDR_DIALECT_CHOICE("Error while authenticating user ",
		"ユーザー認証中にエラーが発生しました ") << uid;

	if (method == 1) {	//APOP
		int32 index = fLog.FindFirst("<");
		if(index != B_ERROR) {
			runner->ReportProgress(0, 0, MDR_DIALECT_CHOICE(
				"Sending APOP authentication...", "APOP認証情報を送信中..."));
			int32 end = fLog.FindFirst(">",index);
			BString timestamp("");
			fLog.CopyInto(timestamp, index, end - index + 1);
			timestamp += password;
			char md5sum[33];
			MD5Digest((unsigned char*)timestamp.String(), md5sum);
			BString cmd = "APOP ";
			cmd += uid;
			cmd += " ";
			cmd += md5sum;
			cmd += CRLF;

			err = SendCommand(cmd.String());
			if (err != B_OK) {
				error_msg << MDR_DIALECT_CHOICE(". The server said:\n",
					"サーバのメッセージです\n") << fLog;
				runner->ShowError(error_msg.String());
				return err;
			}

			return B_OK;
		} else {
			error_msg << MDR_DIALECT_CHOICE(": The server does not support APOP.",
				"サーバはAPOPをサポートしていません");
			runner->ShowError(error_msg.String());
			return B_NOT_ALLOWED;
		}
	}
	runner->ReportProgress(0, 0, MDR_DIALECT_CHOICE("Sending username...",
		"ユーザーID送信中..."));

	BString cmd = "USER ";
	cmd += uid;
	cmd += CRLF;

	err = SendCommand(cmd.String());
	if (err != B_OK) {
		error_msg << MDR_DIALECT_CHOICE(". The server said:\n",
			"サーバのメッセージです\n") << fLog;
		runner->ShowError(error_msg.String());
		return err;
	}

	runner->ReportProgress(0, 0, MDR_DIALECT_CHOICE("Sending password...",
		"パスワード送信中..."));
	cmd = "PASS ";
	cmd += password;
	cmd += CRLF;

	err = SendCommand(cmd.String());
	if (err != B_OK) {
		error_msg << MDR_DIALECT_CHOICE(". The server said:\n",
			"サーバのメッセージです\n") << fLog;
		runner->ShowError(error_msg.String());
		return err;
	}

	return B_OK;
}


status_t
POP3Protocol::Stat()
{
	runner->ReportProgress(0, 0, MDR_DIALECT_CHOICE("Getting mailbox size...",
		"メールボックスのサイズを取得しています..."));

	if (SendCommand("STAT" CRLF) < B_OK)
		return B_ERROR;

	int32 messages,dropSize;
	if (sscanf(fLog.String(), "+OK %ld %ld", &messages, &dropSize) < 2)
		return B_ERROR;

	fNumMessages = messages;
	fMailDropSize = dropSize;

	return B_OK;
}


int32
POP3Protocol::Messages()
{
	if (fNumMessages < 0)
		Stat();

	return fNumMessages;
}


size_t
POP3Protocol::MailDropSize()
{
	if (fNumMessages < 0)
		Stat();

	return fMailDropSize;
}


status_t
POP3Protocol::Retrieve(int32 message, BPositionIO *write_to)
{
	status_t returnCode;
	BString cmd;
	cmd << "RETR " << message + 1 << CRLF;
	returnCode = RetrieveInternal(cmd.String(), message, write_to, true);
	runner->ReportProgress(0 /* bytes */, 1 /* messages */);

	if (returnCode == B_OK) { // Some debug code.
		int32 message_len = MessageSize(message);
 		write_to->Seek (0, SEEK_END);
		if (write_to->Position() != message_len) {
			printf ("POP3Protocol::Retrieve Note: message size is %d, was "
			"expecting %ld, for message #%ld.  Could be a transmission error "
			"or a bad POP server implementation (does it remove escape codes "
			"when it counts size?).\n",
			(int) write_to->Position(), message_len, message);
		}
	}

	return returnCode;
}


status_t
POP3Protocol::GetHeader(int32 message, BPositionIO *write_to)
{
	BString cmd;
	cmd << "TOP " << message + 1 << " 0" << CRLF;
	return RetrieveInternal(cmd.String(),message,write_to, false);
}


status_t
POP3Protocol::RetrieveInternal(const char *command, int32 message,
	BPositionIO *write_to, bool post_progress)
{
	const int bufSize = 1024 * 30;

	// To avoid waiting for the non-arrival of the next data packet, try to
	// receive only the message size, plus the 3 extra bytes for the ".\r\n"
	// after the message.  Of course, if we get it wrong (or it is a huge
	// message or has lines starting with escaped periods), it will then switch
	// back to receiving full buffers until the message is done.
	int amountToReceive = MessageSize (message) + 3;
	if (amountToReceive >= bufSize || amountToReceive <= 0)
		amountToReceive = bufSize - 1;

	BString bufBString; // Used for auto-dealloc on return feature.
	char *buf = bufBString.LockBuffer (bufSize);
	int amountInBuffer = 0;
	int amountReceived;
	int testIndex;
	char *testStr;
	bool cont = true;
	bool flushWholeBuffer = false;
	write_to->Seek(0,SEEK_SET);

	if (SendCommand(command) != B_OK)
		return B_ERROR;

	struct timeval tv;
	tv.tv_sec = POP3_RETRIEVAL_TIMEOUT / 1000000;
	tv.tv_usec = POP3_RETRIEVAL_TIMEOUT % 1000000;

	struct fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(fSocket, &readSet);

	while (cont) {
		int result = 0;

#ifdef USE_SSL
		if (fUseSSL && SSL_pending(fSSL))
			result = 1;
		else
#endif
		result = select(fSocket + 1, &readSet, NULL, NULL, &tv);
		if (result == 0) {
			// No data available, even after waiting a minute.
			fLog = "POP3 timeout - no data received after a long wait.";
			runner->Stop(true);
			return B_ERROR;
		}
		if (amountToReceive > bufSize - 1 - amountInBuffer)
			amountToReceive = bufSize - 1 - amountInBuffer;
#ifdef USE_SSL
		if (fUseSSL) {
			amountReceived = SSL_read(fSSL, buf + amountInBuffer,
				amountToReceive);
		} else
#endif
		amountReceived = recv(fSocket, buf + amountInBuffer, amountToReceive, 0);
		if (amountReceived < 0) {
			fLog = strerror(errno);
			return errno;
		}
		if (amountReceived == 0) {
			fLog = "POP3 data supposedly ready to receive but not received!";
			return B_ERROR;
		}

		amountToReceive = bufSize - 1; // For next time, read a full buffer.
		amountInBuffer += amountReceived;
		buf[amountInBuffer] = 0; // NUL stops tests past the end of buffer.

		// Look for lines starting with a period.  A single period by itself on
		// a line "\r\n.\r\n" marks the end of the message (thus the need for
		// at least five characters in the buffer for testing).  A period
		// "\r\n.Stuff" at the start of a line get deleted "\r\nStuff", since
		// POP adds one as an escape code to let you have message text with
		// lines starting with a period.  For convenience, assume that no
		// messages start with a period on the very first line, so we can
		// search for the previous line's "\r\n".

		for (testIndex = 0; testIndex <= amountInBuffer - 5; testIndex++) {
			testStr = buf + testIndex;
			if (testStr[0] == '\r' && testStr[1] == '\n' && testStr[2] == '.') {
				if (testStr[3] == '\r' && testStr[4] == '\n') {
					// Found the end of the message marker.  Ignore remaining data.
					if (amountInBuffer > testIndex + 5)
						printf ("POP3Protocol::RetrieveInternal Ignoring %d bytes "
							"of extra data past message end.\n",
							amountInBuffer - (testIndex + 5));
					amountInBuffer = testIndex + 2; // Don't include ".\r\n".
					buf[amountInBuffer] = 0;
					cont = false;
				} else {
					// Remove an extra period at the start of a line.
					// Inefficient, but it doesn't happen often that you have a
					// dot starting a line of text.  Of course, a file with a
					// lot of double period lines will get processed very
					// slowly.
					memmove (buf + testIndex + 2, buf + testIndex + 3,
						amountInBuffer - (testIndex + 3) + 1 /* for NUL at end */);
					amountInBuffer--;
					// Watch out for the end of buffer case, when the POP text
					// is "\r\n..X".  Don't want to leave the resulting
					// "\r\n.X" in the buffer (flush out the whole buffer),
					// since that will get mistakenly evaluated again in the
					// next loop and delete a character by mistake.
					if (testIndex >= amountInBuffer - 4 && testStr[2] == '.') {
						printf ("POP3Protocol::RetrieveInternal: Jackpot!  You have "
							"hit the rare situation with an escaped period at the "
							"end of the buffer.  Aren't you happy it decodes it "
							"correctly?\n");
						flushWholeBuffer = true;
					}
				}
			}
		}

		if (cont && !flushWholeBuffer) {
			// Dump out most of the buffer, but leave the last 4 characters for
			// comparison continuity, in case the line starting with a period
			// crosses a buffer boundary.
			if (amountInBuffer > 4) {
				write_to->Write(buf, amountInBuffer - 4);
				if (post_progress)
					runner->ReportProgress(amountInBuffer - 4,0);
				memmove (buf, buf + amountInBuffer - 4, 4);
				amountInBuffer = 4;
			}
		} else { // Dump everything - end of message or flushing the whole buffer.
			write_to->Write(buf, amountInBuffer);
			if (post_progress)
				runner->ReportProgress(amountInBuffer,0);
			amountInBuffer = 0;
		}
	}
	return B_OK;
}


status_t
POP3Protocol::UniqueIDs()
{
	status_t ret = B_OK;
	runner->ReportProgress(0, 0, MDR_DIALECT_CHOICE("Getting UniqueIDs...",
		"固有のIDを取得中..."));

	ret = SendCommand("UIDL" CRLF);
	if (ret != B_OK)
		return ret;

	BString result;
	int32 uid_offset;
	while (ReceiveLine(result) > 0) {
		if (result.ByteAt(0) == '.')
			break;

		uid_offset = result.FindFirst(' ') + 1;
		result.Remove(0,uid_offset);
		unique_ids->AddItem(result.String());
	}

	if (SendCommand("LIST" CRLF) != B_OK)
		return B_ERROR;

	int32 b;
	while (ReceiveLine(result) > 0) {
		if (result.ByteAt(0) == '.')
			break;

		b = result.FindLast(" ");
		if (b >= 0)
			b = atol(&(result.String()[b]));
		else
			b = 0;
		fSizes.AddItem((void *)(b));
	}

	return ret;
}


void
POP3Protocol::Delete(int32 num)
{
	BString cmd = "DELE ";
	cmd << (num+1) << CRLF;
	if (SendCommand(cmd.String()) != B_OK) {
		// Error
	}
#if DEBUG
	puts(fLog.String());
#endif
}


size_t
POP3Protocol::MessageSize(int32 index)
{
	return (size_t)fSizes.ItemAt(index);
}


int32
POP3Protocol::ReceiveLine(BString &line)
{
	int32 len = 0;
	bool flag = false;

	line = "";

	struct timeval tv;
	tv.tv_sec = POP3_RETRIEVAL_TIMEOUT / 1000000;
	tv.tv_usec = POP3_RETRIEVAL_TIMEOUT % 1000000;

	struct fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(fSocket, &readSet);

	int result;
#ifdef USE_SSL
	if (fUseSSL && SSL_pending(fSSL))
		result = 1;
	else
#endif
	result = select(fSocket + 1, &readSet, NULL, NULL, &tv);

	if (result > 0) {
		while (true) {
			// Hope there's an end of line out there else this gets stuck.
			int32 bytesReceived;
			uint8 c = 0;
#ifdef USE_SSL
			if (fUseSSL)
				bytesReceived = SSL_read(fSSL, (char*)&c, 1);
			else
#endif
			bytesReceived = recv(fSocket, &c, 1, 0);
			if (bytesReceived < 0)
				return errno;

			if (c == '\n' || bytesReceived == 0 /* EOF */)
				break;

			if (c == '\r') {
				flag = true;
			} else {
				if (flag) {
					len++;
					line += '\r';
					flag = false;
				}
				len++;
				line += (char)c;
			}
		}
	} else
		return errno;

	return len;
}


status_t
POP3Protocol::SendCommand(const char *cmd)
{
	if (fSocket < 0 || fSocket >= FD_SETSIZE)
		return B_FILE_ERROR;

	//printf(cmd);
	// Flush any accumulated garbage data before we send our command, so we
	// don't misinterrpret responses from previous commands (that got left over
	// due to bugs) as being from this command.

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000;
		/* very short timeout, hangs with 0 in R5 */

	struct fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(fSocket, &readSet);
	int result;
#ifdef USE_SSL
	if (fUseSSL && SSL_pending(fSSL))
		result = 1;
	else
#endif
	result = select(fSocket + 1, &readSet, NULL, NULL, &tv);

	if (result > 0) {
		int amountReceived;
		char tempString [1025];
#ifdef USE_SSL
		if (fUseSSL)
			amountReceived = SSL_read(fSSL, tempString, sizeof(tempString) - 1);
		else
#endif
		amountReceived = recv(fSocket, tempString, sizeof(tempString) - 1, 0);
		if (amountReceived < 0)
			return errno;

		tempString [amountReceived] = 0;
		printf ("POP3Protocol::SendCommand Bug!  Had to flush %d bytes: %s\n",
			amountReceived, tempString);
		//if (amountReceived == 0)
		//	break;
	}

#ifdef USE_SSL
	if (fUseSSL) {
		SSL_write(fSSL, cmd,::strlen(cmd));
	} else
#endif
	if (send(fSocket, cmd, ::strlen(cmd), 0) < 0) {
		fLog = strerror(errno);
		printf("POP3Protocol::SendCommand Send \"%s\" failed, code %d: %s\n",
			cmd, errno, fLog.String());
		return errno;
	}

	fLog = "";
	status_t err = B_OK;

	while (true) {
		int32 len = ReceiveLine(fLog);
		if (len <= 0 || fLog.ICompare("+OK", 3) == 0)
			break;

		if (fLog.ICompare("-ERR", 4) == 0) {
			printf("POP3Protocol::SendCommand \"%s\" got error message "
				"from server: %s\n", cmd, fLog.String());
			err = B_ERROR;
			break;
		} else {
			printf("POP3Protocol::SendCommand \"%s\" got nonsense message "
				"from server: %s\n", cmd, fLog.String());
			err = B_BAD_VALUE;
				// If it's not +OK, and it's not -ERR, then what the heck
				// is it? Presume an error
			break;
		}
	}
	return err;
}


void
POP3Protocol::MD5Digest(unsigned char *in, char *asciiDigest)
{
	unsigned char digest[16];

#ifdef USE_SSL
	MD5(in, ::strlen((char*)in), digest);
#else
	MD5_CTX context;

	MD5Init(&context);
	MD5Update(&context, in, ::strlen((char*)in));
	MD5Final(digest, &context);
#endif

	for (int i = 0;  i < 16;  i++) {
		sprintf(asciiDigest + 2 * i, "%02x", digest[i]);
	}

	return;
}


//	#pragma mark -


BMailFilter *
instantiate_mailfilter(BMessage *settings, BMailChainRunner *runner)
{
	return new POP3Protocol(settings,runner);
}


BView*
instantiate_config_panel(BMessage *settings, BMessage *)
{
	BMailProtocolConfigView *view = new BMailProtocolConfigView(
		B_MAIL_PROTOCOL_HAS_USERNAME | B_MAIL_PROTOCOL_HAS_AUTH_METHODS
		| B_MAIL_PROTOCOL_HAS_PASSWORD | B_MAIL_PROTOCOL_HAS_HOSTNAME
		| B_MAIL_PROTOCOL_CAN_LEAVE_MAIL_ON_SERVER
#if USE_SSL
		| B_MAIL_PROTOCOL_HAS_FLAVORS
#endif
		);
	view->AddAuthMethod("Plain text");
	view->AddAuthMethod("APOP");

#if USE_SSL
	view->AddFlavor("No encryption");
	view->AddFlavor("SSL");
#endif

	view->SetTo(settings);
	return view;
}

