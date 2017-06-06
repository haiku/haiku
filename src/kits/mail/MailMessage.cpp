/*
 * Copyright 2007-2015, Haiku Inc. All Rights Reserved.
 * Copyright 2001-2004 Dr. Zoidberg Enterprises. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//! The main general purpose mail message class


#include <MailMessage.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#include <parsedate.h>

#include <Directory.h>
#include <E-mail.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <List.h>
#include <MailAttachment.h>
#include <MailDaemon.h>
#include <MailSettings.h>
#include <Messenger.h>
#include <netdb.h>
#include <NodeInfo.h>
#include <Path.h>
#include <String.h>
#include <StringList.h>

#include <MailPrivate.h>
#include <mail_util.h>


using namespace BPrivate;


//-------Change the following!----------------------
#define mime_boundary "----------Zoidberg-BeMail-temp--------"
#define mime_warning "This is a multipart message in MIME format."


BEmailMessage::BEmailMessage(BPositionIO* file, bool own, uint32 defaultCharSet)
	:
	BMailContainer(defaultCharSet),
	fData(NULL),
	fStatus(B_NO_ERROR),
	fBCC(NULL),
	fComponentCount(0),
	fBody(NULL),
	fTextBody(NULL)
{
	BMailSettings settings;
	fAccountID = settings.DefaultOutboundAccount();

	if (own)
		fData = file;

	if (file != NULL)
		SetToRFC822(file, ~0L);
}


BEmailMessage::BEmailMessage(const entry_ref* ref, uint32 defaultCharSet)
	:
	BMailContainer(defaultCharSet),
	fBCC(NULL),
	fComponentCount(0),
	fBody(NULL),
	fTextBody(NULL)
{
	BMailSettings settings;
	fAccountID = settings.DefaultOutboundAccount();

	fData = new BFile();
	fStatus = static_cast<BFile*>(fData)->SetTo(ref, B_READ_ONLY);

	if (fStatus == B_OK)
		SetToRFC822(fData, ~0L);
}


BEmailMessage::~BEmailMessage()
{
	free(fBCC);

	delete fBody;
	delete fData;
}


status_t
BEmailMessage::InitCheck() const
{
	return fStatus;
}


BEmailMessage*
BEmailMessage::ReplyMessage(mail_reply_to_mode replyTo, bool accountFromMail,
	const char* quoteStyle)
{
	BEmailMessage* reply = new BEmailMessage;

	// Set ReplyTo:

	if (replyTo == B_MAIL_REPLY_TO_ALL) {
		reply->SetTo(From());

		BList list;
		get_address_list(list, CC(), extract_address);
		get_address_list(list, To(), extract_address);

		// Filter out the sender
		BMailAccounts accounts;
		BMailAccountSettings* account = accounts.AccountByID(Account());
		BString sender;
		if (account != NULL)
			sender = account->ReturnAddress();
		extract_address(sender);

		BString cc;

		for (int32 i = list.CountItems(); i-- > 0;) {
			char* address = (char*)list.RemoveItem((int32)0);

			// Add everything which is not the sender and not already in the
			// list
			if (sender.ICompare(address) && cc.FindFirst(address) < 0) {
				if (cc.Length() > 0)
					cc << ", ";

				cc << address;
			}

			free(address);
		}

		if (cc.Length() > 0)
			reply->SetCC(cc.String());
	} else if (replyTo == B_MAIL_REPLY_TO_SENDER || ReplyTo() == NULL)
		reply->SetTo(From());
	else
		reply->SetTo(ReplyTo());

	// Set special "In-Reply-To:" header (used for threading)
	const char* messageID = fBody ? fBody->HeaderField("Message-Id") : NULL;
	if (messageID != NULL)
		reply->SetHeaderField("In-Reply-To", messageID);

	// quote body text
	reply->SetBodyTextTo(BodyText());
	if (quoteStyle)
		reply->Body()->Quote(quoteStyle);

	// Set the subject (and add a "Re:" if needed)
	BString string = Subject();
	if (string.ICompare("re:", 3) != 0)
		string.Prepend("Re: ");
	reply->SetSubject(string.String());

	// set the matching outbound chain
	if (accountFromMail)
		reply->SendViaAccountFrom(this);

	return reply;
}


BEmailMessage*
BEmailMessage::ForwardMessage(bool accountFromMail, bool includeAttachments)
{
	BString header = "------ Forwarded Message: ------\n";
	header << "To: " << To() << '\n';
	header << "From: " << From() << '\n';
	if (CC() != NULL) {
		// Can use CC rather than "Cc" since display only.
		header << "CC: " << CC() << '\n';
	}
	header << "Subject: " << Subject() << '\n';
	header << "Date: " << Date() << "\n\n";
	if (fTextBody != NULL)
		header << fTextBody->Text() << '\n';
	BEmailMessage *message = new BEmailMessage();
	message->SetBodyTextTo(header.String());

	// set the subject
	BString subject = Subject();
	if (subject.IFindFirst("fwd") == B_ERROR
		&& subject.IFindFirst("forward") == B_ERROR
		&& subject.FindFirst("FW") == B_ERROR)
		subject << " (fwd)";
	message->SetSubject(subject.String());

	if (includeAttachments) {
		for (int32 i = 0; i < CountComponents(); i++) {
			BMailComponent* component = GetComponent(i);
			if (component == fTextBody || component == NULL)
				continue;

			//---I am ashamed to have the written the code between here and the next comment
			// ... and you still managed to get it wrong ;-)), axeld.
			// we should really move this stuff into copy constructors
			// or something like that

			BMallocIO io;
			component->RenderToRFC822(&io);
			BMailComponent* clone = component->WhatIsThis();
			io.Seek(0, SEEK_SET);
			clone->SetToRFC822(&io, io.BufferLength(), true);
			message->AddComponent(clone);
		}
	}
	if (accountFromMail)
		message->SendViaAccountFrom(this);

	return message;
}


const char*
BEmailMessage::To() const
{
	return HeaderField("To");
}


const char*
BEmailMessage::From() const
{
	return HeaderField("From");
}


const char*
BEmailMessage::ReplyTo() const
{
	return HeaderField("Reply-To");
}


const char*
BEmailMessage::CC() const
{
	return HeaderField("Cc");
		// Note case of CC is "Cc" in our internal headers.
}


const char*
BEmailMessage::Subject() const
{
	return HeaderField("Subject");
}


time_t
BEmailMessage::Date() const
{
	const char* dateField = HeaderField("Date");
	if (dateField == NULL)
		return -1;

	return ParseDateWithTimeZone(dateField);
}


int
BEmailMessage::Priority() const
{
	int priorityNumber;
	const char* priorityString;

	/* The usual values are a number from 1 to 5, or one of three words:
	X-Priority: 1 and/or X-MSMail-Priority: High
	X-Priority: 3 and/or X-MSMail-Priority: Normal
	X-Priority: 5 and/or X-MSMail-Priority: Low
	Also plain Priority: is "normal", "urgent" or "non-urgent", see RFC 1327. */

	priorityString = HeaderField("Priority");
	if (priorityString == NULL)
		priorityString = HeaderField("X-Priority");
	if (priorityString == NULL)
		priorityString = HeaderField("X-Msmail-Priority");
	if (priorityString == NULL)
		return 3;
	priorityNumber = atoi (priorityString);
	if (priorityNumber != 0) {
		if (priorityNumber > 5)
			priorityNumber = 5;
		if (priorityNumber < 1)
			priorityNumber = 1;
		return priorityNumber;
	}
	if (strcasecmp (priorityString, "Low") == 0
		|| strcasecmp (priorityString, "non-urgent") == 0)
		return 5;
	if (strcasecmp (priorityString, "High") == 0
		|| strcasecmp (priorityString, "urgent") == 0)
		return 1;
	return 3;
}


void
BEmailMessage::SetSubject(const char* subject, uint32 charset,
	mail_encoding encoding)
{
	SetHeaderField("Subject", subject, charset, encoding);
}


void
BEmailMessage::SetReplyTo(const char* replyTo, uint32 charset,
	mail_encoding encoding)
{
	SetHeaderField("Reply-To", replyTo, charset, encoding);
}


void
BEmailMessage::SetFrom(const char* from, uint32 charset, mail_encoding encoding)
{
	SetHeaderField("From", from, charset, encoding);
}


void
BEmailMessage::SetTo(const char* to, uint32 charset, mail_encoding encoding)
{
	SetHeaderField("To", to, charset, encoding);
}


void
BEmailMessage::SetCC(const char* cc, uint32 charset, mail_encoding encoding)
{
	// For consistency with our header names, use Cc as the name.
	SetHeaderField("Cc", cc, charset, encoding);
}


void
BEmailMessage::SetBCC(const char* bcc)
{
	free(fBCC);
	fBCC = strdup(bcc);
}


void
BEmailMessage::SetPriority(int to)
{
	char tempString[20];

	if (to < 1)
		to = 1;
	if (to > 5)
		to = 5;
	sprintf (tempString, "%d", to);
	SetHeaderField("X-Priority", tempString);
	if (to <= 2) {
		SetHeaderField("Priority", "urgent");
		SetHeaderField("X-Msmail-Priority", "High");
	} else if (to >= 4) {
		SetHeaderField("Priority", "non-urgent");
		SetHeaderField("X-Msmail-Priority", "Low");
	} else {
		SetHeaderField("Priority", "normal");
		SetHeaderField("X-Msmail-Priority", "Normal");
	}
}


status_t
BEmailMessage::GetName(char* name, int32 maxLength) const
{
	if (name == NULL || maxLength <= 0)
		return B_BAD_VALUE;

	if (BFile* file = dynamic_cast<BFile*>(fData)) {
		status_t status = file->ReadAttr(B_MAIL_ATTR_NAME, B_STRING_TYPE, 0,
			name, maxLength);
		name[maxLength - 1] = '\0';

		return status >= 0 ? B_OK : status;
	}
	// TODO: look at From header?  But usually there is
	// a file since only the BeMail GUI calls this.
	return B_ERROR;
}


status_t
BEmailMessage::GetName(BString* name) const
{
	char* buffer = name->LockBuffer(B_FILE_NAME_LENGTH);
	status_t status = GetName(buffer, B_FILE_NAME_LENGTH);
	name->UnlockBuffer();

	return status;
}


void
BEmailMessage::SendViaAccountFrom(BEmailMessage* message)
{
	BString name;
	if (message->GetAccountName(name) < B_OK) {
		// just return the message with the default account
		return;
	}

	SendViaAccount(name);
}


void
BEmailMessage::SendViaAccount(const char* accountName)
{
	BMailAccounts accounts;
	BMailAccountSettings* account = accounts.AccountByName(accountName);
	if (account != NULL)
		SendViaAccount(account->AccountID());
}


void
BEmailMessage::SendViaAccount(int32 account)
{
	fAccountID = account;

	BMailAccounts accounts;
	BMailAccountSettings* accountSettings = accounts.AccountByID(fAccountID);

	BString from;
	if (accountSettings) {
		from << '\"' << accountSettings->RealName() << "\" <"
			<< accountSettings->ReturnAddress() << '>';
	}
	SetFrom(from);
}


int32
BEmailMessage::Account() const
{
	return fAccountID;
}


status_t
BEmailMessage::GetAccountName(BString& accountName) const
{
	BFile* file = dynamic_cast<BFile*>(fData);
	if (file == NULL)
		return B_ERROR;

	int32 accountID;
	size_t read = file->ReadAttr(B_MAIL_ATTR_ACCOUNT, B_INT32_TYPE, 0,
		&accountID, sizeof(int32));
	if (read < sizeof(int32))
		return B_ERROR;

	BMailAccounts accounts;
	BMailAccountSettings* account =  accounts.AccountByID(accountID);
	if (account != NULL)
		accountName = account->Name();
	else
		accountName = "";

	return B_OK;
}


status_t
BEmailMessage::AddComponent(BMailComponent* component)
{
	status_t status = B_OK;

	if (fComponentCount == 0)
		fBody = component;
	else if (fComponentCount == 1) {
		BMIMEMultipartMailContainer *container
			= new BMIMEMultipartMailContainer(
				mime_boundary, mime_warning, _charSetForTextDecoding);
		status = container->AddComponent(fBody);
		if (status == B_OK)
			status = container->AddComponent(component);
		fBody = container;
	} else {
		BMIMEMultipartMailContainer* container
			= dynamic_cast<BMIMEMultipartMailContainer*>(fBody);
		if (container == NULL)
			return B_MISMATCHED_VALUES;

		status = container->AddComponent(component);
	}

	if (status == B_OK)
		fComponentCount++;
	return status;
}


status_t
BEmailMessage::RemoveComponent(BMailComponent* /*component*/)
{
	// not yet implemented
	// BeMail/Enclosures.cpp:169: contains a warning about this fact
	return B_ERROR;
}


status_t
BEmailMessage::RemoveComponent(int32 /*index*/)
{
	// not yet implemented
	return B_ERROR;
}


BMailComponent*
BEmailMessage::GetComponent(int32 i, bool parseNow)
{
	if (BMIMEMultipartMailContainer* container
			= dynamic_cast<BMIMEMultipartMailContainer*>(fBody))
		return container->GetComponent(i, parseNow);

	if (i < fComponentCount)
		return fBody;

	return NULL;
}


int32
BEmailMessage::CountComponents() const
{
	return fComponentCount;
}


void
BEmailMessage::Attach(entry_ref* ref, bool includeAttributes)
{
	if (includeAttributes)
		AddComponent(new BAttributedMailAttachment(ref));
	else
		AddComponent(new BSimpleMailAttachment(ref));
}


bool
BEmailMessage::IsComponentAttachment(int32 i)
{
	if ((i >= fComponentCount) || (fComponentCount == 0))
		return false;

	if (fComponentCount == 1)
		return fBody->IsAttachment();

	BMIMEMultipartMailContainer* container
		= dynamic_cast<BMIMEMultipartMailContainer*>(fBody);
	if (container == NULL)
		return false;

	BMailComponent* component = container->GetComponent(i);
	if (component == NULL)
		return false;

	return component->IsAttachment();
}


void
BEmailMessage::SetBodyTextTo(const char* text)
{
	if (fTextBody == NULL) {
		fTextBody = new BTextMailComponent;
		AddComponent(fTextBody);
	}

	fTextBody->SetText(text);
}


BTextMailComponent*
BEmailMessage::Body()
{
	if (fTextBody == NULL)
		fTextBody = _RetrieveTextBody(fBody);

	return fTextBody;
}


const char*
BEmailMessage::BodyText()
{
	if (Body() == NULL)
		return NULL;

	return fTextBody->Text();
}


status_t
BEmailMessage::SetBody(BTextMailComponent* body)
{
	if (fTextBody != NULL) {
		return B_ERROR;
//	removing doesn't exist for now
//		RemoveComponent(fTextBody);
//		delete fTextBody;
	}
	fTextBody = body;
	AddComponent(fTextBody);

	return B_OK;
}


BTextMailComponent*
BEmailMessage::_RetrieveTextBody(BMailComponent* component)
{
	BTextMailComponent* body = dynamic_cast<BTextMailComponent*>(component);
	if (body != NULL)
		return body;

	BMIMEMultipartMailContainer* container
		= dynamic_cast<BMIMEMultipartMailContainer*>(component);
	if (container != NULL) {
		for (int32 i = 0; i < container->CountComponents(); i++) {
			if ((component = container->GetComponent(i)) == NULL)
				continue;

			switch (component->ComponentType()) {
				case B_MAIL_PLAIN_TEXT_BODY:
					// AttributedAttachment returns the MIME type of its
					// contents, so we have to use dynamic_cast here
					body = dynamic_cast<BTextMailComponent*>(
						container->GetComponent(i));
					if (body != NULL)
						return body;
					break;

				case B_MAIL_MULTIPART_CONTAINER:
					body = _RetrieveTextBody(container->GetComponent(i));
					if (body != NULL)
						return body;
					break;
			}
		}
	}
	return NULL;
}


status_t
BEmailMessage::SetToRFC822(BPositionIO* mailFile, size_t length,
	bool parseNow)
{
	if (BFile* file = dynamic_cast<BFile*>(mailFile)) {
		file->ReadAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE, 0, &fAccountID,
			sizeof(fAccountID));
	}

	mailFile->Seek(0, SEEK_END);
	length = mailFile->Position();
	mailFile->Seek(0, SEEK_SET);

	fStatus = BMailComponent::SetToRFC822(mailFile, length, parseNow);
	if (fStatus < B_OK)
		return fStatus;

	fBody = WhatIsThis();

	mailFile->Seek(0, SEEK_SET);
	fStatus = fBody->SetToRFC822(mailFile, length, parseNow);
	if (fStatus < B_OK)
		return fStatus;

	// Move headers that we use to us, everything else to fBody
	const char* name;
	for (int32 i = 0; (name = fBody->HeaderAt(i)) != NULL; i++) {
		if (strcasecmp(name, "Subject") != 0
			&& strcasecmp(name, "To") != 0
			&& strcasecmp(name, "From") != 0
			&& strcasecmp(name, "Reply-To") != 0
			&& strcasecmp(name, "Cc") != 0
			&& strcasecmp(name, "Priority") != 0
			&& strcasecmp(name, "X-Priority") != 0
			&& strcasecmp(name, "X-Msmail-Priority") != 0
			&& strcasecmp(name, "Date") != 0) {
			RemoveHeader(name);
		}
	}

	fBody->RemoveHeader("Subject");
	fBody->RemoveHeader("To");
	fBody->RemoveHeader("From");
	fBody->RemoveHeader("Reply-To");
	fBody->RemoveHeader("Cc");
	fBody->RemoveHeader("Priority");
	fBody->RemoveHeader("X-Priority");
	fBody->RemoveHeader("X-Msmail-Priority");
	fBody->RemoveHeader("Date");

	fComponentCount = 1;
	if (BMIMEMultipartMailContainer* container
			= dynamic_cast<BMIMEMultipartMailContainer*>(fBody))
		fComponentCount = container->CountComponents();

	return B_OK;
}


status_t
BEmailMessage::RenderToRFC822(BPositionIO* file)
{
	if (fBody == NULL)
		return B_MAIL_INVALID_MAIL;

	// Do real rendering

	if (From() == NULL) {
		// set the "From:" string
		SendViaAccount(fAccountID);
	}

	BList recipientList;
	get_address_list(recipientList, To(), extract_address);
	get_address_list(recipientList, CC(), extract_address);
	get_address_list(recipientList, fBCC, extract_address);

	BString recipients;
	for (int32 i = recipientList.CountItems(); i-- > 0;) {
		char *address = (char *)recipientList.RemoveItem((int32)0);

		recipients << '<' << address << '>';
		if (i)
			recipients << ',';

		free(address);
	}

	// add the date field
	time_t creationTime = time(NULL);
	{
		char date[128];
		struct tm tm;
		localtime_r(&creationTime, &tm);

		size_t length = strftime(date, sizeof(date),
			"%a, %d %b %Y %H:%M:%S", &tm);

		// GMT offsets are full hours, yes, but you never know :-)
		snprintf(date + length, sizeof(date) - length, " %+03d%02d",
			tm.tm_gmtoff / 3600, (tm.tm_gmtoff / 60) % 60);

		SetHeaderField("Date", date);
	}

	// add a message-id

	// empirical evidence indicates message id must be enclosed in
	// angle brackets and there must be an "at" symbol in it
	BString messageID;
	messageID << "<";
	messageID << system_time();
	messageID << "-BeMail@";

	char host[255];
	if (gethostname(host, sizeof(host)) < 0 || !host[0])
		strcpy(host, "zoidberg");

	messageID << host;
	messageID << ">";

	SetHeaderField("Message-Id", messageID.String());

	status_t err = BMailComponent::RenderToRFC822(file);
	if (err < B_OK)
		return err;

	file->Seek(-2, SEEK_CUR);
		// Remove division between headers

	err = fBody->RenderToRFC822(file);
	if (err < B_OK)
		return err;

	// Set the message file's attributes.  Do this after the rest of the file
	// is filled in, in case the daemon attempts to send it before it is ready
	// (since the daemon may send it when it sees the status attribute getting
	// set to "Pending").

	if (BFile* attributed = dynamic_cast <BFile*>(file)) {
		BNodeInfo(attributed).SetType(B_MAIL_TYPE);

		attributed->WriteAttrString(B_MAIL_ATTR_RECIPIENTS,&recipients);

		BString attr;

		attr = To();
		attributed->WriteAttrString(B_MAIL_ATTR_TO, &attr);
		attr = CC();
		attributed->WriteAttrString(B_MAIL_ATTR_CC, &attr);
		attr = Subject();
		attributed->WriteAttrString(B_MAIL_ATTR_SUBJECT, &attr);
		attr = ReplyTo();
		attributed->WriteAttrString(B_MAIL_ATTR_REPLY, &attr);
		attr = From();
		attributed->WriteAttrString(B_MAIL_ATTR_FROM, &attr);
		if (Priority() != 3 /* Normal is 3 */) {
			sprintf(attr.LockBuffer(40), "%d", Priority());
			attr.UnlockBuffer(-1);
			attributed->WriteAttrString(B_MAIL_ATTR_PRIORITY, &attr);
		}
		attr = "Pending";
		attributed->WriteAttrString(B_MAIL_ATTR_STATUS, &attr);
		attr = "1.0";
		attributed->WriteAttrString(B_MAIL_ATTR_MIME, &attr);

		attributed->WriteAttr(B_MAIL_ATTR_ACCOUNT, B_INT32_TYPE, 0,
			&fAccountID, sizeof(int32));

		attributed->WriteAttr(B_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, &creationTime,
			sizeof(int32));
		int32 flags = B_MAIL_PENDING | B_MAIL_SAVE;
		attributed->WriteAttr(B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, &flags,
			sizeof(int32));

		attributed->WriteAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE, 0,
			&fAccountID, sizeof(int32));
	}

	return B_OK;
}


status_t
BEmailMessage::RenderTo(BDirectory* dir, BEntry* msg)
{
	time_t currentTime;
	char numericDateString[40];
	struct tm timeFields;
	BString worker;

	// Generate a file name for the outgoing message.  See also
	// FolderFilter::ProcessMailMessage which does something similar for
	// incoming messages.

	BString name = Subject();
	SubjectToThread(name);
		// Extract the core subject words.
	if (name.Length() <= 0)
		name = "No Subject";
	if (name[0] == '.') {
		// Avoid hidden files, starting with a dot.
		name.Prepend("_");
	}

	// Convert the date into a year-month-day fixed digit width format, so that
	// sorting by file name will give all the messages with the same subject in
	// order of date.
	time (&currentTime);
	localtime_r (&currentTime, &timeFields);
	sprintf (numericDateString, "%04d%02d%02d%02d%02d%02d",
		timeFields.tm_year + 1900, timeFields.tm_mon + 1, timeFields.tm_mday,
		timeFields.tm_hour, timeFields.tm_min, timeFields.tm_sec);
	name << " " << numericDateString;

	worker = From();
	extract_address_name(worker);
	name << " " << worker;

	name.Truncate(222);	// reserve space for the uniquer

	// Get rid of annoying characters which are hard to use in the shell.
	name.ReplaceAll('/','_');
	name.ReplaceAll('\'','_');
	name.ReplaceAll('"','_');
	name.ReplaceAll('!','_');
	name.ReplaceAll('<','_');
	name.ReplaceAll('>','_');

	// Remove multiple spaces.
	while (name.FindFirst("  ") >= 0)
		name.Replace("  ", " ", 1024);

	int32 uniquer = time(NULL);
	worker = name;

	int32 tries = 30;
	bool exists;
	while ((exists = dir->Contains(worker.String())) && --tries > 0) {
		srand(rand());
		uniquer += (rand() >> 16) - 16384;

		worker = name;
		worker << ' ' << uniquer;
	}

	if (exists)
		printf("could not create mail! (should be: %s)\n", worker.String());

	BFile file;
	status_t status = dir->CreateFile(worker.String(), &file);
	if (status != B_OK)
		return status;

	if (msg != NULL)
		msg->SetTo(dir,worker.String());

	return RenderToRFC822(&file);
}


status_t
BEmailMessage::Send(bool sendNow)
{
	BMailAccounts accounts;
	BMailAccountSettings* account = accounts.AccountByID(fAccountID);
	if (account == NULL || !account->HasOutbound()) {
		account = accounts.AccountByID(
			BMailSettings().DefaultOutboundAccount());
		if (!account)
			return B_ERROR;
		SendViaAccount(account->AccountID());
	}

	BString path;
	if (account->OutboundSettings().FindString("path", &path) != B_OK) {
		BPath defaultMailOutPath;
		if (find_directory(B_USER_DIRECTORY, &defaultMailOutPath) != B_OK
			|| defaultMailOutPath.Append("mail/out") != B_OK)
			path = "/boot/home/mail/out";
		else
			path = defaultMailOutPath.Path();
	}

	create_directory(path.String(), 0777);
	BDirectory directory(path.String());

	BEntry message;

	status_t status = RenderTo(&directory, &message);
	if (status >= B_OK && sendNow) {
		// TODO: check whether or not the internet connection is available
		BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
		if (!daemon.IsValid())
			return B_MAIL_NO_DAEMON;

		BMessage msg(kMsgSendMessages);
		msg.AddInt32("account", fAccountID);
		BPath path;
		message.GetPath(&path);
		msg.AddString("message_path", path.Path());
		daemon.SendMessage(&msg);
	}

	return status;
}


void BEmailMessage::_ReservedMessage1() {}
void BEmailMessage::_ReservedMessage2() {}
void BEmailMessage::_ReservedMessage3() {}
