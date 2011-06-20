/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
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

#include <arpa/inet.h>
#include <sys/select.h>

#if USE_SSL
#	include <openssl/ssl.h>
#	include <openssl/rand.h>
#	include <openssl/md5.h>
#else
#	include "md5.h"
#endif

#include <Alert.h>
#include <Catalog.h>
#include <Debug.h>
#include <Directory.h>
#include <fs_attr.h>
#include <Path.h>
#include <String.h>
#include <VolumeRoster.h>
#include <Query.h>

#include "crypt.h"
#include "MailSettings.h"
#include "MessageIO.h"


#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "pop3"


#define POP3_RETRIEVAL_TIMEOUT 60000000
#define CRLF	"\r\n"


POP3Protocol::POP3Protocol(BMailAccountSettings* settings)
	:
	InboundProtocol(settings),
	fNumMessages(-1),
	fMailDropSize(0)
{
	printf("POP3Protocol::POP3Protocol(BMailAccountSettings* settings)\n");
	fSettings = fAccountSettings.InboundSettings().Settings();
#ifdef USE_SSL
	fUseSSL = (fSettings.FindInt32("flavor") == 1);
	fSSL = NULL;
	fSSLContext = NULL;
#endif

	if (fSettings.FindString("destination", &fDestinationDir) != B_OK)
		fDestinationDir = "/boot/home/mail/in";

	create_directory(fDestinationDir, 0777);

	fFetchBodyLimit = 0;
	if (fSettings.HasInt32("partial_download_limit"))
		fFetchBodyLimit = fSettings.FindInt32("partial_download_limit");
}


POP3Protocol::~POP3Protocol()
{
	Disconnect();
}


status_t
POP3Protocol::Connect()
{
	status_t error = Open(fSettings.FindString("server"), fSettings.FindInt32("port"),
				fSettings.FindInt32("flavor"));
	if (error != B_OK)
		return error;

	char* password = get_passwd(&fSettings, "cpasswd");

	error = Login(fSettings.FindString("username"), password,
		fSettings.FindInt32("auth_method"));
	delete[] password;

	return error;
}


status_t
POP3Protocol::Disconnect()
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
		if (fSSLBio)
			BIO_free(fSSLBio);

		fSSL = NULL;
		fSSLContext = NULL;
		fSSLBio = NULL;
	}
#endif

	close(fSocket);
	fSocket = -1;
	return B_OK;
}


status_t
POP3Protocol::SyncMessages()
{
	bool leaveOnServer;
	if (fSettings.FindBool("leave_mail_on_server", &leaveOnServer) != B_OK)
		leaveOnServer = true;

	// create directory if not exist
	create_directory(fDestinationDir, 0777);

	printf("POP3Protocol::SyncMessages()\n");
	_ReadManifest();

	SetTotalItems(2);
	ReportProgress(0, 1, B_TRANSLATE("Connect to server" B_UTF8_ELLIPSIS));
	status_t error = Connect();
	if (error < B_OK) {
		ResetProgress();
		return error;
	}

	ReportProgress(0, 1, B_TRANSLATE("Getting UniqueIDs" B_UTF8_ELLIPSIS));
	error = _UniqueIDs();
	if (error < B_OK) {
		ResetProgress();
		return error;
	}

	BStringList toDownload;
	fManifest.NotHere(fUniqueIDs, &toDownload);

	int32 numMessages = toDownload.CountItems();
	if (numMessages == 0) {
		CheckForDeletedMessages();
		ResetProgress();
		return B_OK;
	}

	ResetProgress();
	SetTotalItems(toDownload.CountItems());

	printf("POP3: Messages to download: %i\n", (int)toDownload.CountItems());
	for (int32 i = 0; i < toDownload.CountItems(); i++) {
		const char* uid = toDownload.ItemAt(i);
		int32 toRetrieve = fUniqueIDs.IndexOf(uid);

		if (toRetrieve < 0) {
			// should not happen!
			error = B_NAME_NOT_FOUND;
			printf("POP3: uid %s index %i not found in fUniqueIDs!\n", uid,
				(int)toRetrieve);
			continue;
		}

		BPath path(fDestinationDir);
		BString fileName = "Downloading file... uid: ";
		fileName += uid;
		fileName.ReplaceAll("/", "_SLASH_");
		path.Append(fileName);
		BEntry entry(path.Path());
		BFile file(&entry, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		error = file.InitCheck();
		if (error != B_OK) {
			printf("POP3: Can't create file %s\n ", path.Path());
			break;
		}
		BMailMessageIO mailIO(this, &file, toRetrieve);

		entry_ref ref;
		entry.GetRef(&ref);

		// the ref becomes invalid after renaming the file thus we already
		// write the status here
		MarkMessageAsRead(ref, B_UNREAD);

		int32 size = MessageSize(toRetrieve);
		if (fFetchBodyLimit < 0 || size <= fFetchBodyLimit) {
			error = mailIO.Seek(0, SEEK_END);
			if (error < 0) {
				printf("POP3: Failed to download body %s\n ", uid);
				break;
			}
			NotifyHeaderFetched(ref, &file);
			NotifyBodyFetched(ref, &file);

			if (!leaveOnServer)
				Delete(toRetrieve);
		} else {
			int32 dummy;
			error = mailIO.ReadAt(0, &dummy, 1);
			if (error < 0) {
				printf("POP3: Failed to download header %s\n ", uid);
				break;
			}
			NotifyHeaderFetched(ref, &file);
		}
		ReportProgress(0, 1);

		if (file.WriteAttr("MAIL:unique_id", B_STRING_TYPE, 0, uid,
			strlen(uid)) < 0) {
			error = B_ERROR;
		}

		file.WriteAttr("MAIL:size", B_INT32_TYPE, 0, &size, sizeof(int32));

		// save manifest in case we get disturbed
		fManifest += uid;
		_WriteManifest();
	}

	ResetProgress();

	CheckForDeletedMessages();
	Disconnect();
	return error;
}


status_t
POP3Protocol::FetchBody(const entry_ref& ref)
{
	ResetProgress("Fetch body");
	SetTotalItems(1);

	status_t error = Connect();
	if (error < B_OK)
		return error;

	error = _UniqueIDs();
	if (error < B_OK)
		return error;

	BFile file(&ref, B_READ_WRITE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	char uidString[256];
	BNode node(&ref);
	if (node.ReadAttr("MAIL:unique_id", B_STRING_TYPE, 0, uidString, 256) < 0)
		return B_ERROR;

	int32 toRetrieve = fUniqueIDs.IndexOf(uidString);
	if (toRetrieve < 0)
		return B_NAME_NOT_FOUND;

	bool leaveOnServer;
	if (fSettings.FindBool("leave_mail_on_server", &leaveOnServer) != B_OK)
		leaveOnServer = true;

	// TODO: get rid of this BMailMessageIO!
	BMailMessageIO io(this, &file, toRetrieve);
	// read body
	status = io.Seek(0, SEEK_END);
	if (status < 0)
		return status;

	NotifyBodyFetched(ref, &file);

	if (!leaveOnServer)
		Delete(toRetrieve);

	ReportProgress(0, 1);
	ResetProgress();

	Disconnect();
	return B_OK;
}


status_t
POP3Protocol::DeleteMessage(const entry_ref& ref)
{
	status_t error = Connect();
	if (error < B_OK)
		return error;

	error = _UniqueIDs();
	if (error < B_OK)
		return error;

#if DEBUG
	printf("DeleteMessage: ID is %d\n", (int)unique_ids->IndexOf(uid));
		// What should we use for int32 instead of %d?
#endif
	char uidString[256];
	BNode node(&ref);
	if (node.ReadAttr("MAIL:unique_id", B_STRING_TYPE, 0, uidString, 256) < 0)
		return B_ERROR;
	Delete(fUniqueIDs.IndexOf(uidString));

	Disconnect();
	return B_OK;
}


status_t
POP3Protocol::Open(const char *server, int port, int)
{
	ReportProgress(0, 0, B_TRANSLATE("Connecting to POP3 server"
		B_UTF8_ELLIPSIS));

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
	error_msg << B_TRANSLATE("Error while connecting to server ") << server;
	if (port != 110)
		error_msg << ":" << port;

	uint32 hostIP = inet_addr(server);
		// first see if we can parse it as a numeric address
	if (hostIP == 0 || hostIP == ~0UL) {
		struct hostent * he = gethostbyname(server);
		hostIP = he ? *((uint32*)he->h_addr) : 0;
	}

	if (hostIP == 0) {
		error_msg << B_TRANSLATE(": Connection refused or host not found");
		ShowError(error_msg.String());

		return B_NAME_NOT_FOUND;
	}

	fSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fSocket >= 0) {
		struct sockaddr_in saAddr;
		memset(&saAddr, 0, sizeof(saAddr));
		saAddr.sin_family = AF_INET;
		saAddr.sin_port = htons(port);
		saAddr.sin_addr.s_addr = hostIP;
		int result = connect(fSocket, (struct sockaddr *) &saAddr,
			sizeof(saAddr));
		if (result < 0) {
			close(fSocket);
			fSocket = -1;
			error_msg << ": " << strerror(errno);
			ShowError(error_msg.String());
			return errno;
		}
	} else {
		error_msg << B_TRANSLATE(": Could not allocate socket.");
		ShowError(error_msg.String());
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
				<< fSettings.FindString("server");
			if (port != 995)
				error << ":" << port;
			error << ". (SSL connection error)";
			ShowError(error.String());
			SSL_CTX_free(fSSLContext);
			close(fSocket);
			return B_ERROR;
		}
	}
#endif

	BString line;
	status_t err = ReceiveLine(line);

	if (err < 0) {
		close(fSocket);
		fSocket = -1;
		error_msg << ": " << strerror(err);
		ShowError(error_msg.String());
		return B_ERROR;
	}

	if (strncmp(line.String(), "+OK", 3) != 0) {
		if (line.Length() > 0) {
			error_msg << B_TRANSLATE(". The server said:\n")
				<< line.String();
		} else
			error_msg << B_TRANSLATE(": No reply.\n");

		ShowError(error_msg.String());
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
	error_msg << B_TRANSLATE("Error while authenticating user ") << uid;

	if (method == 1) {	//APOP
		int32 index = fLog.FindFirst("<");
		if(index != B_ERROR) {
			ReportProgress(0, 0, B_TRANSLATE("Sending APOP authentication"
				B_UTF8_ELLIPSIS));
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
				error_msg << B_TRANSLATE(". The server said:\n") << fLog;
				ShowError(error_msg.String());
				return err;
			}

			return B_OK;
		} else {
			error_msg << B_TRANSLATE(": The server does not support APOP.");
			ShowError(error_msg.String());
			return B_NOT_ALLOWED;
		}
	}
	ReportProgress(0, 0, B_TRANSLATE("Sending username" B_UTF8_ELLIPSIS));

	BString cmd = "USER ";
	cmd += uid;
	cmd += CRLF;

	err = SendCommand(cmd.String());
	if (err != B_OK) {
		error_msg << B_TRANSLATE(". The server said:\n") << fLog;
		ShowError(error_msg.String());
		return err;
	}

	ReportProgress(0, 0, B_TRANSLATE("Sending password" B_UTF8_ELLIPSIS));
	cmd = "PASS ";
	cmd += password;
	cmd += CRLF;

	err = SendCommand(cmd.String());
	if (err != B_OK) {
		error_msg << B_TRANSLATE(". The server said:\n") << fLog;
		ShowError(error_msg.String());
		return err;
	}

	return B_OK;
}


status_t
POP3Protocol::Stat()
{
	ReportProgress(0, 0, B_TRANSLATE("Getting mailbox size" B_UTF8_ELLIPSIS));

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


void
POP3Protocol::CheckForDeletedMessages()
{
	{
		//---Delete things from the manifest no longer on the server
		BStringList temp;
		fManifest.NotThere(fUniqueIDs, &temp);
		fManifest -= temp;
	}

	if (!fSettings.FindBool("delete_remote_when_local")
		|| fManifest.CountItems() == 0)
		return;

	BStringList to_delete;

	BStringList query_contents;
	BVolumeRoster volumes;
	BVolume volume;

	while (volumes.GetNextVolume(&volume) == B_OK) {
		BQuery fido;
		entry_ref entry;

		fido.SetVolume(&volume);
		fido.PushAttr(B_MAIL_ATTR_ACCOUNT_ID);
		fido.PushInt32(fAccountSettings.AccountID());
		fido.PushOp(B_EQ);

		fido.Fetch();

		BString uid;
		while (fido.GetNextRef(&entry) == B_OK) {
			BNode(&entry).ReadAttrString("MAIL:unique_id", &uid);
			query_contents.AddItem(uid.String());
		}
	}
	query_contents.NotHere(fManifest, &to_delete);

	for (int32 i = 0; i < to_delete.CountItems(); i++) {
		printf("delete mail on server uid %s\n", to_delete[i]);
		Delete(fUniqueIDs.IndexOf(to_delete[i]));
	}

	//*(unique_ids) -= to_delete; --- This line causes bad things to
	// happen (POP3 client uses the wrong indices to retrieve
	// messages).  Without it, bad things don't happen.
	fManifest -= to_delete;
}


status_t
POP3Protocol::Retrieve(int32 message, BPositionIO *write_to)
{
	status_t returnCode;
	BString cmd;
	cmd << "RETR " << message + 1 << CRLF;
	returnCode = RetrieveInternal(cmd.String(), message, write_to, true);
	ReportProgress(0 /* bytes */, 1 /* messages */);

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
						printf ("POP3Protocol::RetrieveInternal: Jackpot!  "
							"You have hit the rare situation with an escaped "
							"period at the end of the buffer.  Aren't you happy"
							"it decodes it correctly?\n");
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
					ReportProgress(amountInBuffer - 4,0);
				memmove (buf, buf + amountInBuffer - 4, 4);
				amountInBuffer = 4;
			}
		} else { // Dump everything - end of message or flushing the whole buffer.
			write_to->Write(buf, amountInBuffer);
			if (post_progress)
				ReportProgress(amountInBuffer,0);
			amountInBuffer = 0;
		}
	}
	return B_OK;
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
	/* The mail is just marked as deleted and removed from the server when
	sending the QUIT command. Because of that the message number stays the same
	and we keep the uid in the uid list. */
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


status_t
POP3Protocol::_UniqueIDs()
{
	fUniqueIDs.MakeEmpty();

	status_t ret = B_OK;

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
		fUniqueIDs.AddItem(result.String());
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
POP3Protocol::_ReadManifest()
{
	fManifest.MakeEmpty();
	BString attr_name = "MAIL:";
	attr_name << fAccountSettings.AccountID() << ":manifest";
	//--- In case someone puts multiple accounts in the same directory

	BNode node(fDestinationDir);
	if (node.InitCheck() != B_OK)
		return;

	// We already have a directory so we can try to read metadata
	// from it. Note that it is normal for this directory not to
	// be found on the first run as it will be later created by
	// the INBOX system filter.
	attr_info info;
	/*status_t status = node.GetAttrInfo(attr_name.String(), &info);
	printf("read manifest3 status %i\n", (int)status);
	status = node.GetAttrInfo(attr_name.String(), &info);
	printf("read manifest3 status2 %i\n", (int)status);*/
	if (node.GetAttrInfo(attr_name.String(), &info) != B_OK)
		return;

	void* flatmanifest = malloc(info.size);
	node.ReadAttr(attr_name.String(), fManifest.TypeCode(), 0,
		flatmanifest, info.size);
	fManifest.Unflatten(fManifest.TypeCode(), flatmanifest, info.size);
	free(flatmanifest);
}


void
POP3Protocol::_WriteManifest()
{
	BString attr_name = "MAIL:";
	attr_name << fAccountSettings.AccountID() << ":manifest";
		//--- In case someone puts multiple accounts in the same directory
	BNode node(fDestinationDir);
	if (node.InitCheck() != B_OK) {
		ShowError("Error while saving account manifest: cannot use "
			"destination directory.");
		return;
	}

	node.RemoveAttr(attr_name.String());
	ssize_t manifestsize = fManifest.FlattenedSize();
	void* flatmanifest = malloc(manifestsize);
	fManifest.Flatten(flatmanifest, manifestsize);
	status_t err = node.WriteAttr(attr_name.String(),
		fManifest.TypeCode(), 0, flatmanifest, manifestsize);
	if (err < 0) {
		BString error = "Error while saving account manifest: ";
		error << strerror(err);
			printf("moep error\n");
		ShowError(error.String());
	}

	free(flatmanifest);
}


//	#pragma mark -


InboundProtocol*
instantiate_inbound_protocol(BMailAccountSettings* settings)
{
	return new POP3Protocol(settings);
}


status_t
pop3_smtp_auth(BMailAccountSettings* settings)
{
	POP3Protocol protocol(settings);
	protocol.Connect();
	protocol.Disconnect();
	return B_OK;
}
