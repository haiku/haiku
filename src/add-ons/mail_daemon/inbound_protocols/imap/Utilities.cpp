/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Utilities.h"


BString
MailboxToFolderName(const BString& mailbox, const BString& separator)
{
	if (separator == "/")
		return mailbox;

	BString name = mailbox;
	name.ReplaceAll('/', '_');
	name.ReplaceAll(separator.String(), "/");

	return name;
}
