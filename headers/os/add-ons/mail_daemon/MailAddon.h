/* Filter - the base class for all mail filters
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/
#ifndef ZOIDBERG_MAIL_ADDON_H
#define ZOIDBERG_MAIL_ADDON_H


#include "MailProtocol.h"
#include "MailSettings.h"


class BView;

//
// The addon interface: export instantiate_mailfilter()
// and instantiate_mailconfig() to create a Filter addon
//

extern "C" _EXPORT InboundProtocol* instantiate_inbound_protocol(
	BMailAccountSettings* settings);
extern "C" _EXPORT OutboundProtocol* instantiate_outbound_protocol(
	BMailAccountSettings* settings);
extern "C" _EXPORT BView* instantiate_config_panel(MailAddonSettings&,
	BMailAccountSettings&);
// return a view that configures the MailProtocol
// returned by the functions below.  BView::Archive(foo,true)
// produces this addon's settings, which are passed to the in-
// stantiate_* functions and stored persistently.  This function
// should gracefully handle empty and NULL settings.

extern "C" _EXPORT BView* instantiate_filter_config_panel(AddonSettings&);
extern "C" _EXPORT MailFilter* instantiate_mailfilter(MailProtocol& protocol,
	AddonSettings* settings);

extern "C" _EXPORT BString descriptive_name();
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
