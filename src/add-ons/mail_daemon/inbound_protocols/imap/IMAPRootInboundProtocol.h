/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_ROOT_INBOUND_PROTOCOL_H
#define IMAP_ROOT_INBOUND_PROTOCOL_H


#include "IMAPInboundProtocol.h"

#include "MailAddon.h"


typedef BObjectList<InboundProtocolThread> ProtocolThreadList;


/*! Hold the main INBOX mailbox and manage all the other other mailboxes. */
class IMAPRootInboundProtocol : public IMAPInboundProtocol {
public:
								IMAPRootInboundProtocol(
									BMailAccountSettings* settings);
								~IMAPRootInboundProtocol();

			//! thread safe interface
			status_t			Connect(const char* server,
									const char* username, const char* password,
									bool useSSL = true, int32 port = -1);
			status_t			Disconnect();

			void				SetStopNow();

			status_t			SyncMessages();
			status_t			FetchBody(const entry_ref& ref);
			status_t			MarkMessageAsRead(const entry_ref& ref,
									read_flags flag = B_READ);

			status_t			DeleteMessage(const entry_ref& ref);
			status_t			AppendMessage(const entry_ref& ref);

			status_t			DeleteMessage(node_ref& node);

private:
			void				_ShutdownChilds();
			InboundProtocolThread*	_FindThreadFor(const entry_ref& ref);

			ProtocolThreadList	fProtocolThreadList;
};


#endif // IMAP_ROOT_INBOUND_PROTOCOL_H
