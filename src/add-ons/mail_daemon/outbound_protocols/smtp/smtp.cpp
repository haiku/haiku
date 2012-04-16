/*
 * Copyright 2007-2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2002 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 *
 * Distributed under the terms of the MIT License.
 */


//!	Implementation of the SMTP protocol


#include "smtp.h"

#include <map>

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <Alert.h>
#include <Catalog.h>
#include <DataIO.h>
#include <Entry.h>
#include <File.h>
#include <MenuField.h>
#include <Message.h>
#include <Path.h>
#include <TextControl.h>

#include <crypt.h>
#include <mail_encoding.h>
#include <MailSettings.h>
#include <NodeMessage.h>
#include <ProtocolConfigView.h>

#ifdef USE_SSL
#	include <openssl/md5.h>
#else
#	include "md5.h"
#endif


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "smtp"


#define CRLF "\r\n"
#define SMTP_RESPONSE_SIZE 8192

//#define DEBUG
#ifdef DEBUG
#	define D(x) x
#	define bug printf
#else
#	define D(x) ;
#endif

using namespace std;

/*
** Function: md5_hmac
** taken from the file rfc2104.txt
** written by Martin Schaaf <mascha@ma-scha.de>
*/
void
MD5Hmac(unsigned char *digest, const unsigned char* text, int text_len,
	const unsigned char* key, int key_len)
{
	MD5_CTX context;
	unsigned char k_ipad[64];
		// inner padding - key XORd with ipad
	unsigned char k_opad[64];
		// outer padding - key XORd with opad
	int i;

	/* start out by storing key in pads */
	memset(k_ipad, 0, sizeof k_ipad);
	memset(k_opad, 0, sizeof k_opad);
	if (key_len > 64) {
		/* if key is longer than 64 bytes reset it to key=MD5(key) */
		MD5_CTX tctx;

		MD5_Init(&tctx);
		MD5_Update(&tctx, (unsigned char*)key, key_len);
		MD5_Final(k_ipad, &tctx);
		MD5_Final(k_opad, &tctx);
	} else {
		memcpy(k_ipad, key, key_len);
		memcpy(k_opad, key, key_len);
	}

	/*
	 * the HMAC_MD5 transform looks like:
	 *
	 * MD5(K XOR opad, MD5(K XOR ipad, text))
	 *
	 * where K is an n byte key
	 * ipad is the byte 0x36 repeated 64 times
	 * opad is the byte 0x5c repeated 64 times
	 * and text is the data being protected
	 */

	/* XOR key with ipad and opad values */
	for (i = 0; i < 64; i++) {
		k_ipad[i] ^= 0x36;
		k_opad[i] ^= 0x5c;
	}

	/*
	 * perform inner MD5
	 */
	MD5_Init(&context);		      /* init context for 1st
					       * pass */
	MD5_Update(&context, k_ipad, 64);     /* start with inner pad */
	MD5_Update(&context, (unsigned char*)text, text_len); /* then text of datagram */
	MD5_Final(digest, &context);	      /* finish up 1st pass */
	/*
	 * perform outer MD5
	 */
	MD5_Init(&context);		      /* init context for 2nd
					       * pass */
	MD5_Update(&context, k_opad, 64);     /* start with outer pad */
	MD5_Update(&context, digest, 16);     /* then results of 1st
					       * hash */
	MD5_Final(digest, &context);	      /* finish up 2nd pass */
}


void
MD5HexHmac(char *hexdigest, const unsigned char* text, int text_len,
	const unsigned char* key, int key_len)
{
	unsigned char digest[16];
	int i;
	unsigned char c;

	MD5Hmac(digest, text, text_len, key, key_len);
  	for (i = 0;  i < 16;  i++) {
  		c = digest[i];
		*hexdigest++ = (c > 0x9F ? 'a'-10 : '0')+(c>>4);
		*hexdigest++ = ((c&0x0F) > 9 ? 'a'-10 : '0')+(c&0x0F);
  	}
  	*hexdigest = '\0';
}


/*
** Function: MD5Sum
** generates an MD5-sum from the given string
*/
void
MD5Sum (char* sum, unsigned char *text, int text_len) {
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, text, text_len);
	MD5_Final((unsigned char*)sum, &context);
	sum[16] = '\0';
}

/*
** Function: MD5Digest
** generates an MD5-digest from the given string
*/
void MD5Digest (char* hexdigest, unsigned char *text, int text_len) {
	int i;
	unsigned char digest[17];
	unsigned char c;

	MD5Sum((char*)digest, text, text_len);

  	for (i = 0;  i < 16;  i++) {
  		c = digest[i];
		*hexdigest++ = (c > 0x9F ? 'a'-10 : '0')+(c>>4);
		*hexdigest++ = ((c&0x0F) > 9 ? 'a'-10 : '0')+(c&0x0F);
  	}
  	*hexdigest = '\0';
}

/*
** Function: SplitChallengeIntoMap
** splits a challenge-string into the given map (see RFC-2831)
*/
// :
static bool
SplitChallengeIntoMap(BString str, map<BString,BString>& m)
{
	m.clear();
	const char* key;
	const char* val;
	char* s = (char*)str.String();
	while(*s != 0) {
		while(isspace(*s))
			s++;
		key = s;
		while(isalpha(*s))
			s++;
		if (*s != '=')
			return false;
		*s++ = '\0';
		while(isspace(*s))
			s++;
		if (*s=='"') {
			val = ++s;
			while(*s!='"') {
				if (*s == 0)
					return false;
				s++;
			}
			*s++ = '\0';
		} else {
			val = s;
			while(*s!=0 && *s!=',' && !isspace(*s))
				s++;
			if (*s != 0)
				*s++ = '\0';
		}
		m[key] = val;
		while(isspace(*s))
			s++;
		if (*s != ',')
			return false;
		s++;
	}
	return true;
}


// Authentication types recognized. Not all methods are implemented.
enum AuthType {
	LOGIN		= 1,
	PLAIN		= 1 << 2,
	CRAM_MD5	= 1 << 3,
	DIGEST_MD5	= 1 << 4
};


SMTPProtocol::SMTPProtocol(BMailAccountSettings* settings)
	:
	OutboundProtocol(settings),
	fAuthType(0)
{
	fSettingsMessage = settings->OutboundSettings().Settings();
}


SMTPProtocol::~SMTPProtocol()
{

}


status_t
SMTPProtocol::Connect()
{
	BString error_msg;
	int32 authMethod = fSettingsMessage.FindInt32("auth_method");

	status_t status = B_ERROR;

	if (authMethod == 2) {
		// POP3 authentication is handled here instead of SMTPProtocol::Login()
		// because some servers obviously don't like establishing the connection
		// to the SMTP server first...
		status_t status = _POP3Authentication();
		if (status < B_OK) {
			error_msg << B_TRANSLATE("POP3 authentication failed. The server "
				"said:\n") << fLog;
			ShowError(error_msg.String());
			return status;
		}
	}

	status = Open(fSettingsMessage.FindString("server"),
		fSettingsMessage.FindInt32("port"), authMethod == 1);
	if (status < B_OK) {
		error_msg << B_TRANSLATE("Error while opening connection to %serv");
		error_msg.ReplaceFirst("%serv", fSettingsMessage.FindString("server"));

		if (fSettingsMessage.FindInt32("port") > 0)
			error_msg << ":" << fSettingsMessage.FindInt32("port");

		// << strerror(err) - BNetEndpoint sucks, we can't use this;
		if (fLog.Length() > 0)
			error_msg << B_TRANSLATE(". The server says:\n") << fLog;
		else
			error_msg << B_TRANSLATE(": Connection refused or host not found.");

		ShowError(error_msg.String());

		return status;
	}

	const char* password = get_passwd(&fSettingsMessage, "cpasswd");
	status = Login(fSettingsMessage.FindString("username"), password);
	delete[] password;

	if (status != B_OK) {
		//-----This is a really cool kind of error message. How can we make it work for POP3?
		error_msg << B_TRANSLATE("Error while logging in to %serv")
			<< B_TRANSLATE(". The server said:\n") << fLog;

		error_msg.ReplaceFirst("%serv", fSettingsMessage.FindString("server"));

		ShowError(error_msg.String());
	}
	return B_OK;
}


void
SMTPProtocol::Disconnect()
{
	Close();
}


//! Process EMail to be sent
status_t
SMTPProtocol::SendMessages(const std::vector<entry_ref>& mails,
	size_t totalBytes)
{
	status_t status = Connect();
	if (status != B_OK)
		return status;

	for (unsigned int i = 0; i < mails.size(); i++) {
		status = _SendMessage(mails[i]);

		if (status != B_OK) {
			BString error;
			error << "An error occurred while sending the message " <<
				mails[i].name << ":\n" << fLog;
			ShowError(error.String());

			ResetProgress();
			break;
		}
		off_t size = 0;
		const entry_ref& ref = mails[i];
		BNode(&ref).GetSize(&size);
		ReportProgress(size, 1);
	}

	Disconnect();
	return B_ERROR;
}


//! Opens connection to server
status_t
SMTPProtocol::Open(const char *address, int port, bool esmtp)
{
	ReportProgress(0, 0, B_TRANSLATE("Connecting to server" B_UTF8_ELLIPSIS));

	#ifdef USE_SSL
		use_ssl = (fSettingsMessage.FindInt32("flavor") == 1);
		use_STARTTLS = (fSettingsMessage.FindInt32("flavor") == 2);
		ssl = NULL;
		ctx = NULL;
	#endif

	if (port <= 0)
	#ifdef USE_SSL
		port = use_ssl ? 465 : 25;
	#else
		port = 25;
	#endif

	uint32 hostIP = inet_addr(address);  // first see if we can parse it as a numeric address
	if ((hostIP == 0)||(hostIP == (uint32)-1)) {
		struct hostent * he = gethostbyname(address);
		hostIP = he ? *((uint32*)he->h_addr) : 0;
	}

	if (hostIP == 0)
		return EHOSTUNREACH;

	fSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fSocket < 0)
		return errno;

	struct sockaddr_in saAddr;
	memset(&saAddr, 0, sizeof(saAddr));
	saAddr.sin_family = AF_INET;
	saAddr.sin_port = htons(port);
	saAddr.sin_addr.s_addr = hostIP;
	int result = connect(fSocket, (struct sockaddr *)&saAddr,
		sizeof(saAddr));
	if (result < 0) {
		close(fSocket);
		fSocket = -1;
		return errno;
	}

#ifdef USE_SSL
	if (use_ssl) {
		SSL_library_init();
    	SSL_load_error_strings();
    	RAND_seed(this,sizeof(SMTPProtocol));
    	/*--- Because we're an add-on loaded at an unpredictable time, all
    	      the memory addresses and things contained in ourself are
    	      esssentially random. */

    	ctx = SSL_CTX_new(SSLv23_method());
    	ssl = SSL_new(ctx);
    	sbio=BIO_new_socket(fSocket,BIO_NOCLOSE);
    	SSL_set_bio(ssl,sbio,sbio);

    	if (SSL_connect(ssl) <= 0) {
    		BString error;
			error << "Could not connect to SMTP server "
				<< fSettingsMessage.FindString("server");
			if (port != 465)
				error << ":" << port;
			error << ". (SSL connection error)";
			ShowError(error.String());
			SSL_CTX_free(ctx);
			close(fSocket);
			fSocket = -1;
			return B_ERROR;
		}
	}
#endif	// USE_SSL

	BString line;
	ReceiveResponse(line);

	char localhost[255];
	gethostname(localhost,255);

	if (localhost[0] == 0)
		strcpy(localhost,"namethisbebox");

	char *cmd = new char[::strlen(localhost)+8];
	if (!esmtp)
		::sprintf(cmd,"HELO %s"CRLF, localhost);
	else
		::sprintf(cmd,"EHLO %s"CRLF, localhost);

	if (SendCommand(cmd) != B_OK) {
		delete[] cmd;
		return B_ERROR;
	}

#ifdef USE_SSL
	// Check for STARTTLS
	if (use_STARTTLS) {
		const char *res = fLog.String();
		char *p;

		SSL_library_init();
		RAND_seed(this,sizeof(SMTPProtocol));
		::sprintf(cmd, "STARTTLS"CRLF);

		if ((p = ::strstr(res, "STARTTLS")) != NULL) {
			// Server advertises STARTTLS support
			if (SendCommand(cmd) != B_OK) {
				delete[] cmd;
				return B_ERROR;
			}

			// We should start TLS negotiation
			use_ssl = true;
			ctx = SSL_CTX_new(TLSv1_method());
    		ssl = SSL_new(ctx);
    		sbio = BIO_new_socket(fSocket,BIO_NOCLOSE);
    		BIO_set_nbio(sbio, 0);
    		SSL_set_bio(ssl, sbio, sbio);
    		SSL_set_connect_state(ssl);
    		if(SSL_do_handshake(ssl) != 1)
    			return B_ERROR;

    		// Should send EHLO command again
    		if(!esmtp)
    			::sprintf(cmd, "HELO %s"CRLF, localhost);
    		else
    			::sprintf(cmd, "EHLO %s"CRLF, localhost);

    		if (SendCommand(cmd) != B_OK) {
    			delete[] cmd;
    			return B_ERROR;
    		}
		}
	}
#endif	// USE_SSL

	delete[] cmd;

	// Check auth type
	if (esmtp) {
		const char *res = fLog.String();
		char *p;
		if ((p = ::strstr(res, "250-AUTH")) != NULL) {
			if(::strstr(p, "LOGIN"))
				fAuthType |= LOGIN;
			if(::strstr(p, "PLAIN"))
				fAuthType |= PLAIN;
			if(::strstr(p, "CRAM-MD5"))
				fAuthType |= CRAM_MD5;
			if(::strstr(p, "DIGEST-MD5")) {
				fAuthType |= DIGEST_MD5;
				fServerName = address;
			}
		}
	}
	return B_OK;
}


status_t
SMTPProtocol::_SendMessage(const entry_ref& mail)
{
	// open read write to be able to manipulate in MessageReadyToSend hook
	BFile file(&mail, B_READ_WRITE);
	status_t status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage header;
	file >> header;

	const char *from = header.FindString("MAIL:from");
	const char *to = header.FindString("MAIL:recipients");
	if (!to)
		to = header.FindString("MAIL:to");

	if (to == NULL || from == NULL) {
		fLog = "Invalid message headers";
		return B_ERROR;
	}

	NotifyMessageReadyToSend(mail, &file);
	status = Send(to, from, &file);
	if (status != B_OK)
		return status;
	NotifyMessageSent(mail, &file);
	return B_OK;
}


status_t
SMTPProtocol::_POP3Authentication()
{
	const entry_ref& entry = fAccountSettings.InboundPath();
	if (strcmp(entry.name, "POP3") != 0)
		return B_ERROR;

	status_t (*pop3_smtp_auth)(BMailAccountSettings*);

	BPath path(&entry);
	image_id image = load_add_on(path.Path());
	if (image < 0)
		return B_ERROR;
	if (get_image_symbol(image, "pop3_smtp_auth",
		B_SYMBOL_TYPE_TEXT, (void **)&pop3_smtp_auth) != B_OK) {
		unload_add_on(image);
		image = -1;
		return B_ERROR;
	}
	status_t status = (*pop3_smtp_auth)(&fAccountSettings);
	unload_add_on(image);
	return status;
}


status_t
SMTPProtocol::Login(const char *_login, const char *password)
{
	if (fAuthType == 0)
		return B_OK;

	const char *login = _login;
	char hex_digest[33];
	BString out;

	int32 loginlen = ::strlen(login);
	int32 passlen = ::strlen(password);

	if (fAuthType & DIGEST_MD5) {
		//******* DIGEST-MD5 Authentication ( tested. works fine [with Cyrus SASL] )
		// this implements only the subpart of DIGEST-MD5 which is
		// required for authentication to SMTP-servers. Integrity-
		// and confidentiality-protection are not implemented, as
		// they are provided by the use of OpenSSL.
		SendCommand("AUTH DIGEST-MD5"CRLF);
		const char *res = fLog.String();

		if (strncmp(res, "334", 3) != 0)
			return B_ERROR;
		int32 baselen = ::strlen(&res[4]);
		char *base = new char[baselen+1];
		baselen = ::decode_base64(base, &res[4], baselen);
		base[baselen] = '\0';

		D(bug("base: %s\n", base));

		map<BString,BString> challengeMap;
		SplitChallengeIntoMap(base, challengeMap);

		delete[] base;

		BString rawResponse = BString("username=") << '"' << login << '"';
		rawResponse << ",realm=" << '"' << challengeMap["realm"] << '"';
		rawResponse << ",nonce=" << '"' << challengeMap["nonce"] << '"';
		rawResponse << ",nc=00000001";
		char temp[33];
		for( int i=0; i<32; ++i)
			temp[i] = 1+(rand()%254);
		temp[32] = '\0';
		BString rawCnonce(temp);
		BString cnonce;
		char* cnoncePtr = cnonce.LockBuffer(rawCnonce.Length()*2);
		baselen = ::encode_base64(cnoncePtr, rawCnonce.String(), rawCnonce.Length(), true /* headerMode */);
		cnoncePtr[baselen] = '\0';
		cnonce.UnlockBuffer(baselen);
		rawResponse << ",cnonce=" << '"' << cnonce << '"';
		rawResponse << ",qop=auth";
		BString digestUriValue	= BString("smtp/") << fServerName;
		rawResponse << ",digest-uri=" << '"' << digestUriValue << '"';
		char sum[17], hex_digest2[33];
		BString a1,a2,kd;
		BString t1 = BString(login) << ":"
				<< challengeMap["realm"] << ":"
				<< password;
		MD5Sum(sum, (unsigned char*)t1.String(), t1.Length());
		a1 << sum << ":" << challengeMap["nonce"] << ":" << cnonce;
		MD5Digest(hex_digest, (unsigned char*)a1.String(), a1.Length());
		a2 << "AUTHENTICATE:" << digestUriValue;
		MD5Digest(hex_digest2, (unsigned char*)a2.String(), a2.Length());
		kd << hex_digest << ':' << challengeMap["nonce"]
		   << ":" << "00000001" << ':' << cnonce << ':' << "auth"
		   << ':' << hex_digest2;
		MD5Digest(hex_digest, (unsigned char*)kd.String(), kd.Length());

		rawResponse << ",response=" << hex_digest;
		BString postResponse;
		char *resp = postResponse.LockBuffer(rawResponse.Length() * 2 + 10);
		baselen = ::encode_base64(resp, rawResponse.String(), rawResponse.Length(), true /* headerMode */);
		resp[baselen] = 0;
		postResponse.UnlockBuffer();
		postResponse.Append(CRLF);

		SendCommand(postResponse.String());

		res = fLog.String();
		if (atol(res) >= 500)
			return B_ERROR;
		// actually, we are supposed to check the rspauth sent back
		// by the SMTP-server, but that isn't strictly required,
		// so we skip that for now.
		SendCommand(CRLF);	// finish off authentication
		res = fLog.String();
		if (atol(res) < 500)
			return B_OK;
	}
	if (fAuthType & CRAM_MD5) {
		//******* CRAM-MD5 Authentication ( tested. works fine [with Cyrus SASL] )
		SendCommand("AUTH CRAM-MD5"CRLF);
		const char *res = fLog.String();

		if (strncmp(res, "334", 3) != 0)
			return B_ERROR;
		int32 baselen = ::strlen(&res[4]);
		char *base = new char[baselen+1];
		baselen = ::decode_base64(base, &res[4], baselen);
		base[baselen] = '\0';

		D(bug("base: %s\n", base));

		::MD5HexHmac(hex_digest, (const unsigned char *)base, (int)baselen,
			(const unsigned char *)password, (int)passlen);

		D(bug("%s\n%s\n", base, hex_digest));

		delete[] base;

		BString preResponse, postResponse;
		preResponse = login;
		preResponse << " " << hex_digest << CRLF;
		char *resp = postResponse.LockBuffer(preResponse.Length() * 2 + 10);
		baselen = ::encode_base64(resp, preResponse.String(), preResponse.Length(), true /* headerMode */);
		resp[baselen] = 0;
		postResponse.UnlockBuffer();
		postResponse.Append(CRLF);

		SendCommand(postResponse.String());

		res = fLog.String();
		if (atol(res) < 500)
			return B_OK;
	}
	if (fAuthType & LOGIN) {
		//******* LOGIN Authentication ( tested. works fine)
		ssize_t encodedsize; // required by our base64 implementation

		SendCommand("AUTH LOGIN"CRLF);
		const char *res = fLog.String();

		if (strncmp(res, "334", 3) != 0)
			return B_ERROR;

		// Send login name as base64
		char *login64 = new char[loginlen*3 + 6];
		encodedsize = ::encode_base64(login64, (char *)login, loginlen, true /* headerMode */);
		login64[encodedsize] = 0;
		strcat (login64, CRLF);
		SendCommand(login64);
		delete[] login64;

		res = fLog.String();
		if (strncmp(res,"334",3) != 0)
			return B_ERROR;

		// Send password as base64
		login64 = new char[passlen*3 + 6];
		encodedsize = ::encode_base64(login64, (char *)password, passlen, true /* headerMode */);
		login64[encodedsize] = 0;
		strcat (login64, CRLF);
		SendCommand(login64);
		delete[] login64;

		res = fLog.String();
		if (atol(res) < 500)
			return B_OK;
	}
	if (fAuthType & PLAIN) {
		//******* PLAIN Authentication ( tested. works fine [with Cyrus SASL] )
		// format is:
		// 	authenticateID + \0 + username + \0 + password
		// 	(where authenticateID is always empty !?!)
		BString preResponse, postResponse;
		char *stringPntr;
		ssize_t encodedLength;
		stringPntr = preResponse.LockBuffer(loginlen + passlen + 3);
			// +3 to make room for the two \0-chars between the tokens and
			// the final delimiter added by sprintf().
		sprintf (stringPntr, "%c%s%c%s", 0, login, 0, password);
		preResponse.UnlockBuffer(loginlen + passlen + 2);
			// +2 in order to leave out the final delimiter (which is not part
			// of the string).
		stringPntr = postResponse.LockBuffer(preResponse.Length() * 3);
		encodedLength = ::encode_base64(stringPntr, preResponse.String(),
			preResponse.Length(), true /* headerMode */);
		stringPntr[encodedLength] = 0;
		postResponse.UnlockBuffer();
		postResponse.Prepend("AUTH PLAIN ");
		postResponse << CRLF;

		SendCommand(postResponse.String());

		const char *res = fLog.String();
		if (atol(res) < 500)
			return B_OK;
	}
	return B_ERROR;
}


void
SMTPProtocol::Close()
{

	BString cmd = "QUIT";
	cmd += CRLF;

	if (SendCommand(cmd.String()) != B_OK) {
		// Error
	}

#ifdef USE_SSL
        if (use_ssl)  {
                if (ssl)
                        SSL_shutdown(ssl);
                if (ctx)
                        SSL_CTX_free(ctx);
        }
#endif

	close(fSocket);
}


status_t
SMTPProtocol::Send(const char* to, const char* from, BPositionIO *message)
{
	BString cmd = from;
	cmd.Remove(0, cmd.FindFirst("\" <") + 2);
	cmd.Prepend("MAIL FROM: ");
	cmd += CRLF;
	if (SendCommand(cmd.String()) != B_OK)
		return B_ERROR;

	int32 len = strlen(to);
	BString addr("");
	for (int32 i = 0;i < len;i++) {
		char c = to[i];
		if (c != ',')
			addr += (char)c;
		if (c == ','||i == len-1) {
			if(addr.Length() == 0)
				continue;
			cmd = "RCPT TO: ";
			cmd << addr.String() << CRLF;
			if (SendCommand(cmd.String()) != B_OK)
				return B_ERROR;

			addr ="";
		}
	}

	cmd = "DATA";
	cmd += CRLF;
	if (SendCommand(cmd.String()) != B_OK)
		return B_ERROR;

	// Send the message data.  Convert lines starting with a period to start
	// with two periods and so on.  The actual sequence is CR LF Period.  The
	// SMTP server will remove the periods.  Of course, the POP server may then
	// add some of its own, but the POP client should take care of them.

	ssize_t amountRead;
	ssize_t amountToRead;
	ssize_t amountUnread;
	ssize_t bufferLen = 0;
	const int bufferMax = 2000;
	bool foundCRLFPeriod;
	int i;
	bool messageEndedWithCRLF = false;

	message->Seek(0, SEEK_END);
	amountUnread = message->Position();
	message->Seek(0, SEEK_SET);
	char *data = new char[bufferMax];

	while (true) {
		// Fill the buffer if it is getting low, but not every time, to avoid
		// small reads.
		if (bufferLen < bufferMax / 2) {
			amountToRead = bufferMax - bufferLen;
			if (amountToRead > amountUnread)
				amountToRead = amountUnread;
			if (amountToRead > 0) {
				amountRead = message->Read (data + bufferLen, amountToRead);
				if (amountRead <= 0 || amountRead > amountToRead)
					amountUnread = 0; // Just stop reading when an error happens.
				else {
					amountUnread -= amountRead;
					bufferLen += amountRead;
				}
			}
		}

		// Look for the next CRLFPeriod triple.
		foundCRLFPeriod = false;
		for (i = 0; i <= bufferLen - 3; i++) {
			if (data[i] == '\r' && data[i+1] == '\n' && data[i+2] == '.') {
				foundCRLFPeriod = true;
				// Send data up to the CRLF, and include the period too.
#ifdef USE_SSL
				if (use_ssl) {
					if (SSL_write(ssl,data,i + 3) < 0) {
						amountUnread = 0; // Stop when an error happens.
						bufferLen = 0;
						break;
					}
				} else
#endif
				if (send (fSocket,data, i + 3,0) < 0) {
					amountUnread = 0; // Stop when an error happens.
					bufferLen = 0;
					break;
				}
				ReportProgress (i + 2 /* Don't include the double period here */,0);
				// Move the data over in the buffer, but leave the period there
				// so it gets sent a second time.
				memmove(data, data + (i + 2), bufferLen - (i + 2));
				bufferLen -= i + 2;
				break;
			}
		}

		if (!foundCRLFPeriod) {
			if (amountUnread <= 0) { // No more data, all we have is in the buffer.
				if (bufferLen > 0) {
                            #ifdef USE_SSL
                                    if (use_ssl)
                                            SSL_write(ssl,data,bufferLen);
                                    else
                            #endif
					send (fSocket,data, bufferLen,0);
					ReportProgress (bufferLen,0);
					if (bufferLen >= 2)
						messageEndedWithCRLF = (data[bufferLen-2] == '\r' &&
							data[bufferLen-1] == '\n');
				}
				break; // Finished!
			}

			// Send most of the buffer, except a few characters to overlap with
			// the next read, in case the CRLFPeriod is split between reads.
			if (bufferLen > 3) {
                        #ifdef USE_SSL
                            if (use_ssl) {
                                    if (SSL_write(ssl,data,bufferLen - 3) < 0)
                                        break;
                            } else
                        #endif
				if (send (fSocket,data, bufferLen - 3,0) < 0)
					break; // Stop when an error happens.
				ReportProgress (bufferLen - 3,0);
				memmove (data, data + bufferLen - 3, 3);
				bufferLen = 3;
			}
		}
	}
	delete [] data;

	if (messageEndedWithCRLF)
		cmd = "."CRLF; // The standard says don't add extra CRLF.
	else
		cmd = CRLF"."CRLF;

	if (SendCommand(cmd.String()) != B_OK)
		return B_ERROR;

	return B_OK;
}


//! Receives response from server.
int32
SMTPProtocol::ReceiveResponse(BString &out)
{
	out = "";
	int32 len = 0,r;
	char buf[SMTP_RESPONSE_SIZE];
	bigtime_t timeout = 1000000*180; // timeout 180 secs
	bool gotCode = false;
	int32 errCode;
	BString searchStr = "";

	struct timeval tv;
	struct fd_set fds;

	tv.tv_sec = long(timeout / 1e6);
	tv.tv_usec = long(timeout-(tv.tv_sec * 1e6));

	/* Initialize (clear) the socket mask. */
	FD_ZERO(&fds);

	/* Set the socket in the mask. */
	FD_SET(fSocket, &fds);
        int result = -1;
#ifdef USE_SSL
        if ((use_ssl) && (SSL_pending(ssl)))
            result = 1;
        else
#endif
            result = select(1 + fSocket, &fds, NULL, NULL, &tv);
	if (result < 0)
		return errno;

	if (result > 0) {
		while (1) {
                 #ifdef USE_SSL
			if (use_ssl)
				r = SSL_read(ssl,buf,SMTP_RESPONSE_SIZE - 1);
			else
		  #endif
			r = recv(fSocket,buf, SMTP_RESPONSE_SIZE - 1,0);
			if (r <= 0)
				break;

			if (!gotCode)
			{
				if (buf[3] == ' ' || buf[3] == '-')
				{
					errCode = atol(buf);
					gotCode = true;
					searchStr << errCode << ' ';
				}
			}

			len += r;
			out.Append(buf, r);

			if (strstr(buf, CRLF) && (out.FindFirst(searchStr) != B_ERROR))
				break;
		}
	} else
		fLog = "SMTP socket timeout.";

	D(bug("S:%s\n", out.String()));
	return len;
}


// Sends SMTP command. Result kept in fLog

status_t
SMTPProtocol::SendCommand(const char *cmd)
{
	D(bug("C:%s\n", cmd));

#ifdef USE_SSL
	if (use_ssl && ssl) {
		if (SSL_write(ssl,cmd,::strlen(cmd)) < 0)
                    return B_ERROR;
	} else
#endif
	if (send(fSocket,cmd, ::strlen(cmd),0) < 0)
		return B_ERROR;
	fLog = "";

	// Receive
	while (1) {
		int32 len = ReceiveResponse(fLog);

		if (len <= 0) {
			D(bug("SMTP: len == %ld\n", len));
			return B_ERROR;
		}

		if (fLog.Length() > 4 && (fLog[3] == ' ' || fLog[3] == '-'))
		{
			int32 num = atol(fLog.String());
			D(bug("ReplyNumber: %ld\n", num));
			if (num >= 500)
				return B_ERROR;

			break;
		}
	}

	return B_OK;
}


// #pragma mark -


OutboundProtocol*
instantiate_outbound_protocol(BMailAccountSettings* settings)
{
	return new SMTPProtocol(settings);
}
