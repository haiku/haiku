/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "HaikuMailFormatFilter.h"

#include <Directory.h>

#include <E-mail.h>
#include <NodeInfo.h>

#include <mail_util.h>
#include <NodeMessage.h>


struct mail_header_field
{
	const char *rfc_name;

	const char *attr_name;
	type_code attr_type;
	// currently either B_STRING_TYPE and B_TIME_TYPE
};


static const mail_header_field gDefaultFields[] =
{
	{ "To",				B_MAIL_ATTR_TO,			B_STRING_TYPE },
	{ "From",         	B_MAIL_ATTR_FROM,		B_STRING_TYPE },
	{ "Cc",				B_MAIL_ATTR_CC,			B_STRING_TYPE },
	{ "Date",         	B_MAIL_ATTR_WHEN,		B_TIME_TYPE   },
	{ "Delivery-Date",	B_MAIL_ATTR_WHEN,		B_TIME_TYPE   },
	{ "Reply-To",     	B_MAIL_ATTR_REPLY,		B_STRING_TYPE },
	{ "Subject",      	B_MAIL_ATTR_SUBJECT,	B_STRING_TYPE },
	{ "X-Priority",		B_MAIL_ATTR_PRIORITY,	B_STRING_TYPE },	// Priorities with prefered
	{ "Priority",		B_MAIL_ATTR_PRIORITY,	B_STRING_TYPE },	// one first - the numeric
	{ "X-Msmail-Priority", B_MAIL_ATTR_PRIORITY, B_STRING_TYPE },	// one (has more levels).
	{ "Mime-Version",	B_MAIL_ATTR_MIME,		B_STRING_TYPE },
	{ "STATUS",       	B_MAIL_ATTR_STATUS,		B_STRING_TYPE },
	{ "THREAD",       	"MAIL:thread",			B_STRING_TYPE }, //---Not supposed to be used for this (we add it in Parser), but why not?
	{ "NAME",       	B_MAIL_ATTR_NAME,		B_STRING_TYPE },
	{ NULL,				NULL,					0 }
};


HaikuMailFormatFilter::HaikuMailFormatFilter(MailProtocol& protocol,
	BMailAccountSettings* settings)
	:
	MailFilter(protocol, NULL),

	fAccountId(settings->AccountID())
{
	const BMessage* outboundSettings = &settings->OutboundSettings().Settings();
	if (outboundSettings->FindString("destination", &fOutboundDirectory)
		!= B_OK) {
		fOutboundDirectory = default_sent_directory();
	}
}


void
HaikuMailFormatFilter::HeaderFetched(const entry_ref& ref, BFile* file)
{
	file->Seek(0, SEEK_SET);

	BMessage attributes;
	// TODO attributes.AddInt32(B_MAIL_ATTR_CONTENT, length);
	attributes.AddInt32("MAIL:account", fAccountId);

	BString header;
	off_t size;
	if (file->GetSize(&size) == B_OK) {
		char* buffer = header.LockBuffer(size);
		file->Read(buffer, size);
		header.UnlockBuffer(size);
	}

	for (int i = 0; gDefaultFields[i].rfc_name; ++i) {
		BString target;
		status_t status = extract_from_header(header,
			gDefaultFields[i].rfc_name, target);
		if (status != B_OK)
			continue;
		switch (gDefaultFields[i].attr_type){
			case B_STRING_TYPE:
				attributes.AddString(gDefaultFields[i].attr_name, target);
				break;

			case B_TIME_TYPE:
			{
				time_t when;
				when = ParseDateWithTimeZone(target);
				if (when == -1)
					when = time(NULL); // Use current time if it's undecodable.
				attributes.AddData(B_MAIL_ATTR_WHEN, B_TIME_TYPE, &when,
					sizeof(when));
				break;
			}
		}
	}

	(*file) << attributes;


	// Generate a file name for the incoming message.  See also
	// Message::RenderTo which does a similar thing for outgoing messages.

	BString name = attributes.FindString("MAIL:subject");
	SubjectToThread(name); // Extract the core subject words.
	if (name.Length() <= 0)
		name = "No Subject";
	if (name[0] == '.')
		name.Prepend ("_"); // Avoid hidden files, starting with a dot.

	// Convert the date into a year-month-day fixed digit width format, so that
	// sorting by file name will give all the messages with the same subject in
	// order of date.
	time_t dateAsTime = 0;
	const time_t* datePntr;
	ssize_t dateSize;
	char numericDateString [40];
	struct tm timeFields;

	if (attributes.FindData(B_MAIL_ATTR_WHEN, B_TIME_TYPE,
		(const void**)&datePntr, &dateSize) == B_OK)
		dateAsTime = *datePntr;
	localtime_r(&dateAsTime, &timeFields);
	sprintf(numericDateString, "%04d%02d%02d%02d%02d%02d",
		timeFields.tm_year + 1900,
		timeFields.tm_mon + 1,
		timeFields.tm_mday,
		timeFields.tm_hour,
		timeFields.tm_min,
		timeFields.tm_sec);
	name << " " << numericDateString;

	BString worker = attributes.FindString("MAIL:from");
	extract_address_name(worker);
	name << " " << worker;

	name.Truncate(222);	// reserve space for the uniquer

	// Get rid of annoying characters which are hard to use in the shell.
	name.ReplaceAll('/', '_');
	name.ReplaceAll('\'', '_');
	name.ReplaceAll('"', '_');
	name.ReplaceAll('!', '_');
	name.ReplaceAll('<', '_');
	name.ReplaceAll('>', '_');
	while (name.FindFirst("  ") >= 0) // Remove multiple spaces.
		name.Replace("  " /* Old */, " " /* New */, 1024 /* Count */);

	worker = name;
	int32 identicalNumber = 1;
	status_t status = _SetFileName(ref, worker);
	while (status == B_FILE_EXISTS) {
		identicalNumber++;

		worker = name;
		worker << "_" << identicalNumber;
		status = _SetFileName(ref, worker);
	}
	if (status < B_OK)
		printf("FolderFilter::ProcessMailMessage: could not rename mail (%s)! "
			"(should be: %s)\n",strerror(status), worker.String());
	else {
		entry_ref to(ref.device, ref.directory, worker);
		fMailProtocol.FileRenamed(ref, to);
	}

	BNodeInfo info(file);
	info.SetType(B_PARTIAL_MAIL_TYPE);
}


void
HaikuMailFormatFilter::BodyFetched(const entry_ref& ref, BFile* file)
{
	BNodeInfo info(file);
	info.SetType(B_MAIL_TYPE);
}


void
HaikuMailFormatFilter::MessageSent(const entry_ref& ref, BFile* file)
{
	mail_flags flags = B_MAIL_SENT;
	file->WriteAttr(B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, &flags, sizeof(int32));
	file->WriteAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, "Sent", 5);

	if (fOutboundDirectory == "")
		return;
	create_directory(fOutboundDirectory, 755);
	BDirectory dir(fOutboundDirectory);
	fMailProtocol.Looper()->TriggerFileMove(ref, dir);
}


status_t
HaikuMailFormatFilter::_SetFileName(const entry_ref& ref, const BString& name)
{
	BEntry entry(&ref);
	return entry.Rename(name);
}
