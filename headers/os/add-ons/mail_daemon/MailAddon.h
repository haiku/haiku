#ifndef ZOIDBERG_MAIL_ADDON_H
#define ZOIDBERG_MAIL_ADDON_H
/* Filter - the base class for all mail filters
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


class BMessage;
class BView;
class BPositionIO;
class BEntry;
class BPath;
class BView;
class BString;
class BPositionIO;
class BEntry;

enum {
	B_MAIL_DISCARD = B_MAIL_ERROR_BASE + 8,
	//--- This terminates the chain and removes the message being processed
	// both from disk and from the server.
	B_MAIL_END_FETCH,
	//--- Terminates the current operation
	B_MAIL_END_CHAIN
	//--- This is for yikes errors, like an unreachable server.
};

class BMailStatusView;
class BMailChainRunner;

class BMailFilter
{
  public:
	BMailFilter(BMessage* settings);
	// How to support queueing messages until a time of the
	// day/week/month/year?  The settings will contain a
	// persistent ChainID field, the same for all Filters
	// on the same "chain".
	
	virtual ~BMailFilter();
	// This will be called when the settings for this Filter
	// are changed, or there are no new messages to consume
	// after settings->FindInt32("timeout") seconds.
	
	virtual status_t InitCheck(BString* out_message = NULL) = 0;
	// Returns B_OK if the Filter was constructed success-
	// fully.  Otherwise it returns an error code.  If it is
	// passed a valid BString*, it may add an error message
	// to the end of that BString iff it returns an error.
	// If it returns an error code then the MailFilter will
	// probably be deleted and the error shown to the user.
	
	virtual status_t ProcessMailMessage
	(
		BPositionIO** io_message, BEntry* io_entry,
		BMessage* io_headers, BPath* io_folder, const char *io_uid
	) = 0;
	// Filters a message.  On input and output, the arguments
	// are expected to be as below; however it is allowed for
	// the MailFilter to alter any of these values as nece-
	// ssary, so long as the constraints are as described when
	// the function returns:
	//
	//   * io_message - a PositionIO that contains the message
	//       data, pointing to the first byte of the message's
	//       header.  This can be swapped if, eg, the message
	//       is copied across volume boundries. When the chain
	//       begins this is a file in /tmp.
	//   * io_entry - The entry for the PositionIO above.
	//   * io_headers - a list of attributes that will be added
	//       to the message file.
	//   * io_folder - The message's "folder"---may be com-
	//       pletely unrelated to its on-disk Entry.
	//   * io_uid - The unique ID provided by the message's
	//       Protocol
	//
	// At most one Filter::ProcessMailMessage() for a given
	// chain (and thus ChainID) will be called at a time.

  private:
	virtual void _ReservedFilter1();
	virtual void _ReservedFilter2();
	virtual void _ReservedFilter3();
	virtual void _ReservedFilter4();
};

//
// The addon interface: export instantiate_mailfilter()
// and instantiate_mailconfig() to create a Filter addon
//

extern "C" _EXPORT BView* instantiate_config_panel(BMessage *settings,BMessage *metadata);
// return a view that configures the MailProtocol or MailFilter
// returned by the functions below.  BView::Archive(foo,true)
// produces this addon's settings, which are passed to the in-
// stantiate_* functions and stored persistently.  This function
// should gracefully handle empty and NULL settings.
// A note on the metadata argument: The metadata pointer is
// guaranteed to remain valid as long as this view exists. As it is
// a pointer to the chain's metadata, your view can save any
// chain-global settings to it in its Archive() function. Note that
// you must cache this pointer yourself! You will never get it again.
// Also note that it is possible for it to be NULL. 

extern "C" _EXPORT BMailFilter* instantiate_mailfilter(BMessage *settings,
	BMailChainRunner *runner);
// Return a MailProtocol or MailFilter ready to do its thing,
// based on settings produced by archiving your config panel.
// Note that a Mail::Protocol is a Mail::Filter, so use
// instantiate_mailfilter to start things up.

extern "C" _EXPORT status_t descriptive_name(BMessage *msg, char *buffer);
// the config panel will show this name in the chains filter
// list if this function returns B_OK.
// The buffer is as big as B_FILE_NAME_LENGTH.

// standard Filters:
//
// * Parser - does ParseRFC2822(io_message,io_headers)
// * Folder - stores the message in the specified folder,
//     optionally under io_folder, returns MD_HANDLED
// * HeaderFilter(regex,Yes_fiters,No_filters) -
//     Applies Nes_filters to messages that have a header
//     matching regex; applies No_filters otherwise.
// * CompatabilityFilter - Invokes the standard mail_dae-
//     mon filter ~/config/settings/add-ons/MailDaemon/Filter
//     on the message's Entry.
// * Producer - Reads outbound messages from disk and inserts
//     them into the queue.
// * SMTPSender - Sends the message, via the specified
//     SMTP server, to the people in header field
//    "MAIL:recipients", changes the the Entry's
//    "MAIL:flags" field to no longer pending, changes the
//    "MAIL:status" header field to "Sent", and adds a header
//     field "MAIL:when" with the time it was sent.
// * Dumper - returns MD_DISCARD
//
//
// Standard chain types:
//
//   Incoming Mail: Protocol - Parser - Notifier - Folder
//   Outgoing Mail: Producer - SMTPSender
//
// "chains" are lists of addons that appear in, or can be
// added to, the "Accounts" list in the config panel, a tree-
// view ordered by the chain type and the chain's AccountName().
// Their config views should be shown, one after the other,
// in the config panel.

#endif	/* ZOIDBERG_MAIL_ADDON_H */
