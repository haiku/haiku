#ifndef ZOIDBERG_MAIL_PROTOCOL_H
#define ZOIDBERG_MAIL_PROTOCOL_H
/* Protocol - the base class for protocol filters
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <OS.h>

#include <MailAddon.h>


class BHandler;
class BStringList;
class BMailChainRunner;

class BMailProtocol : public BMailFilter
{
  public:
	BMailProtocol(BMessage* settings, BMailChainRunner *runner);
	// Open a connection based on 'settings'.  'settings' will
	// contain a persistent uint32 ChainID field.  At most one
	// Protocol per ChainID will exist at a given time.
	// The constructor of Mail::Protocol initializes manifest.
	// It is your responsibility to fill in unique_ids, *and
	// to keep it updated* in the course of whatever nefarious
	// things your protocol does.
	
	virtual ~BMailProtocol();
	// Close the connection and clean up.   This will be cal-
	// led after FetchMessage() or FetchNewMessage() returns
	// B_TIMED_OUT or B_ERROR, or when the settings for this
	// Protocol are changed.
	
	virtual status_t GetMessage(
		const char* uid,
		BPositionIO** out_file, BMessage* out_headers,
		BPath* out_folder_location
	)=0;
	// Downloads the message with id uid, writing the message's
	// RFC2822 contents to *out_file and storing any headers it
	// wants to add other than those from the message itself into
	// out_headers.  It may store a path (if this type of account
	// supports folders) in *out_folder_location.
	//
	// Returns B_OK if the message is now available in out_file,
	// B_NAME_NOT_FOUND if there is no message with id 'uid' on
	// the server, or another error if the connection failed.
	//
	// B_OK will cause the message to be stored and processed.
	// B_NAME_NOT_FOUND will cause appropriate recovery to be
	// taken (if such exists) but not cause the connection to
	// be terminated.  Any other error will cause anything writen
	// to be discarded and and the connection closed.
	
	// OBS:
	// The Protocol may replace *out_file with a custom (read-
	// only) BPositionIO-derived object that preserves the il-
	// lusion that the message is writen to *out_file, but in
	// fact only reads from the server and writes to *out_file
	// on demand.  This BPositionIO must guarantee that any
	// data returned by Read() has also been writen to *out_-
	// file.  It must return a read error if reading from the
	// network or writing to *out_file fails.
	//
	// The mail_daemon will delete *out_file before invoking
	// FetchMessage() or FetchNewMessage() again.
	
	virtual status_t DeleteMessage(const char* uid)=0;
	// Removes the message from the server.  After this, it's
	// assumed (but not required) that GetMessage(uid,...)
	// et al will fail with B_NAME_NOT_FOUND.
	
			void CheckForDeletedMessages();
	// You can call this to trigger a sweep for deleted messages.
	// Automatically called at the beginning of the chain.
	
	//------MailFilter calls
	virtual status_t ProcessMailMessage
	(
		BPositionIO** io_message, BEntry* io_entry,
		BMessage* io_headers, BPath* io_folder, const char* io_uid
	);
	
  protected:
	BStringList *manifest, *unique_ids;
  	BMessage *settings;
  	
	BMailChainRunner *runner;	
  private:
  	inline void error_alert(const char *process, status_t error);
	virtual void _ReservedProtocol1();
	virtual void _ReservedProtocol2();
	virtual void _ReservedProtocol3();
	virtual void _ReservedProtocol4();
	virtual void _ReservedProtocol5();

	friend class DeletePass;
	
	BHandler *trash_monitor;
	BStringList *uids_on_disk;
	
	uint32 _reserved[3];
};

#endif // ZOIDBERG_MAIL_PROTOCOL_H
