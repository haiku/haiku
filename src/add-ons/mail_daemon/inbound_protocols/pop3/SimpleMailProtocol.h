#ifndef ZOIDBERG_MAIL_SIMPLEPROTOCOL_H
#define ZOIDBERG_MAIL_SIMPLEPROTOCOL_H

#include <MailProtocol.h>

class SimpleMailProtocol : public BMailProtocol {
	public:
		SimpleMailProtocol(BMessage *settings, BMailChainRunner *runner);
			//---Constructor. Simply call this in yours, and most everything will be handled for you.
			
		virtual status_t Open(const char *server, int port, int protocol) = 0;
			//---server is an ASCII representation of the server that you are logging in to
			//---port is the remote port, -1 if you are to use default
			//---protocol is the protocol to use (defined by your add-on and useful for using, say, SSL) -1 is default
	
		virtual status_t Login(const char *uid, const char *password, int method) = 0;
			//---uid is the username provided
			//---likewise password
			//---method is the auth method to use, this works like protocol in Open
		
		virtual int32 Messages(void) = 0;
			//---return the number of messages waiting
		
		virtual status_t GetHeader(int32 message, BPositionIO *write_to) = 0;
			//---Retrieve the header of message <message> into <write_to>
		
		virtual status_t Retrieve(int32 message, BPositionIO *write_to) = 0;
			//---get message number <message>
			//---write your message to <write_to>
			
		virtual void Delete(int32 num) = 0;
			//---delete message number num
	
		virtual size_t MessageSize(int32 index) = 0;
			//---return the size in bytes of message number <index>
			
		virtual size_t MailDropSize(void) = 0;
			//---return the size of the entire maildrop in bytes
		
		virtual status_t UniqueIDs() = 0;
			// Fill the protected member unique_ids with strings containing
			// unique ids for all messages present on the server. This al-
			// lows comparison of remote and local message manifests, so
			// the local and remote contents can be kept in sync.
			//
			// The ID should be unique to this Chain; if that means
			// this Protocol must add account/server info to differ-
			// entiate it from other messages, then that info should
			// be added before returning the IDs and stripped from the
			// ID for use in GetMessage() et al, below.
			//
			// Returns B_OK if this was performed successfully, or another
			// error if the connection has failed.
		
		//---------These implement hooks from up above---------
		//---------not user-servicable-------------------------
		virtual status_t GetMessage(
			const char* uid,
			BPositionIO** out_file, BMessage* out_headers,
			BPath* out_folder_location
		);
		virtual status_t DeleteMessage(const char* uid);
		virtual status_t InitCheck(BString* out_message = NULL);
		virtual ~SimpleMailProtocol();

	protected:
		status_t Init();

	private:
		status_t error;
		int32 last_message;

		uint32 _reserved[5];
};

#endif // ZOIDBERG_MAIL_SIMPLEPROTOCOL_H
