#ifndef ZOIDBERG_MAIL_REMOTESTORAGEPROTOCOL_H
#define ZOIDBERG_MAIL_REMOTESTORAGEPROTOCOL_H
/* RemoteStorageProtocol - the base class for protocol filters
**
** Copyright 2003 Dr. Zoidberg Enterprises. All rights reserved.
*/

#include <MailProtocol.h>
#include <StringList.h>

class BRemoteMailStorageProtocol : public BMailProtocol {
	public:
		BRemoteMailStorageProtocol(BMessage *settings, BMailChainRunner *runner);
		virtual ~BRemoteMailStorageProtocol();
		
		virtual status_t GetMessage(const char *mailbox, const char *message, BPositionIO **, BMessage *headers) = 0;
		virtual status_t AddMessage(const char *mailbox, BPositionIO *data, BString *id) = 0;
		
		virtual status_t DeleteMessage(const char *mailbox, const char *message) = 0;
		virtual status_t CopyMessage(const char *mailbox, const char *to_mailbox, BString *message) = 0;
		
		virtual status_t CreateMailbox(const char *mailbox) = 0;
		virtual status_t DeleteMailbox(const char *mailbox) = 0;
		
				void	 SyncMailbox(const char *mailbox);

		//----Mail::Protocol stuff
		virtual status_t GetMessage(
			const char* uid,
			BPositionIO** out_file, BMessage* out_headers,
			BPath* out_folder_location);
		virtual status_t DeleteMessage(const char* uid);
		
	//---Data members
		BStringList mailboxes;
		
	private:
		BHandler *handler;
};

#endif // ZOIDBERG_MAIL_REMOTESTORAGEPROTOCOL_H
