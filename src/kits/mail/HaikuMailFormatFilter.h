/*
 * Copyright 2011-2013, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_LISTENER_H
#define IMAP_LISTENER_H


#include <MailFilter.h>
#include <String.h>


class HaikuMailFormatFilter : public BMailFilter {
public:
								HaikuMailFormatFilter(BMailProtocol& protocol,
									const BMailAccountSettings& settings);

	virtual BString				DescriptiveName() const;

			BMailFilterAction	HeaderFetched(entry_ref& ref, BFile& file,
									BMessage& attributes);
			void				BodyFetched(const entry_ref& ref, BFile& file,
									BMessage& attributes);

			void				MessageSent(const entry_ref& ref, BFile& file);

private:
			void				_RemoveExtraWhitespace(BString& name);
			void				_RemoveLeadingDots(BString& name);
			BString				_ExtractName(const BString& from);
			status_t			_SetType(BMessage& attributes,
									const char* mimeType);

private:
			int32				fAccountID;
			BString				fAccountName;
			BString				fOutboundDirectory;
};


#endif // IMAP_LISTENER_H
