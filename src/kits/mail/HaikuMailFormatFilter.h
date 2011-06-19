/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_LISTENER_H
#define IMAP_LISTENER_H


#include "MailProtocol.h"

#include <String.h>


class HaikuMailFormatFilter : public MailFilter {
public:
								HaikuMailFormatFilter(MailProtocol& protocol,
									BMailAccountSettings* settings);

			void				HeaderFetched(const entry_ref& ref,
									BFile* file);
			void				BodyFetched(const entry_ref& ref, BFile* file);

			void				MessageSent(const entry_ref& ref,
									BFile* file);
private:
			status_t			_SetFileName(const entry_ref& ref,
									const BString& name);
			BString				_ExtractName(const BString& from);

private:
			int32				fAccountID;
			BString				fAccountName;
			BString				fOutboundDirectory;
};


#endif // IMAP_LISTENER_H
