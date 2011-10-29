/*
 * Copyright 2001-2004 Dr. Zoidberg Enterprises. All rights reserved.
 * Copyright 2007, 2010, Haiku Inc. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */

//! The main general purpose mail message class


#include <List.h>
#include <String.h>
#include <Directory.h>
#include <File.h>
#include <E-mail.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <netdb.h>
#include <NodeInfo.h>
#include <Messenger.h>
#include <Path.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <parsedate.h>

#include <MailMessage.h>
#include <MailAttachment.h>
#include <MailSettings.h>
#include <MailDaemon.h>
#include <mail_util.h>
#include <StringList.h>


//-------Change the following!----------------------
#define mime_boundary "----------Zoidberg-BeMail-temp--------"
#define mime_warning "This is a multipart message in MIME format."


BEmailMessage::BEmailMessage(BPositionIO *file, bool own, uint32 defaultCharSet)
	:
	BMailContainer (defaultCharSet),
	fData(NULL),
	_status(B_NO_ERROR),
	_bcc(NULL),
	_num_components(0),
	_body(NULL),
	_text_body(NULL)
{
	BMailSettings settings;
	_account_id = settings.DefaultOutboundAccount();

	if (own)
		fData = file;

	if (file != NULL)
		SetToRFC822(file,-1);
}


BEmailMessage::BEmailMessage(const entry_ref *ref, uint32 defaultCharSet)
	:
	BMailContainer(defaultCharSet),
	_bcc(NULL),
	_num_components(0),
	_body(NULL),
	_text_body(NULL)
{
	BMailSettings settings;
	_account_id = settings.DefaultOutboundAccount();

	fData = new BFile();
	_status = static_cast<BFile *>(fData)->SetTo(ref,B_READ_ONLY);

	if (_status == B_OK)
		SetToRFC822(fData,-1);
}


BEmailMessage::~BEmailMessage()
{
	free(_bcc);

	delete _body;
	delete fData;
}


status_t
BEmailMessage::InitCheck() const
{
	return _status;
}


BEmailMessage *
BEmailMessage::ReplyMessage(mail_reply_to_mode replyTo, bool accountFromMail,
	const char *quoteStyle)
{
	BEmailMessage *reply = new BEmailMessage;

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
		if (account)
			sender = account->ReturnAddress();
		extract_address(sender);

		BString cc;

		for (int32 i = list.CountItems(); i-- > 0;) {
			char *address = (char *)list.RemoveItem(0L);

			// add everything which is not the sender and not already in the list
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
	const char *messageID = _body ? _body->HeaderField("Message-Id") : NULL;
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


BEmailMessage *
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
	if (_text_body != NULL)
		header << _text_body->Text() << '\n';
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
			BMailComponent *cmpt = GetComponent(i);
			if (cmpt == _text_body || cmpt == NULL)
				continue;

			//---I am ashamed to have the written the code between here and the next comment
			// ... and you still managed to get it wrong ;-)), axeld.
			// we should really move this stuff into copy constructors
			// or something like that

			BMallocIO io;
			cmpt->RenderToRFC822(&io);
			BMailComponent *clone = cmpt->WhatIsThis();
			io.Seek(0, SEEK_SET);
			clone->SetToRFC822(&io, io.BufferLength(), true);
			message->AddComponent(clone);
		}
	}
	if (accountFromMail)
		message->SendViaAccountFrom(this);

	return message;
}


const char *
BEmailMessage::To()
{
	return HeaderField("To");
}


const char *
BEmailMessage::From()
{
	return HeaderField("From");
}


const char *
BEmailMessage::ReplyTo()
{
	return HeaderField("Reply-To");
}


const char *
BEmailMessage::CC()
{
	return HeaderField("Cc");
		// Note case of CC is "Cc" in our internal headers.
}


const char *
BEmailMessage::Subject()
{
	return HeaderField("Subject");
}


const char *
BEmailMessage::Date()
{
	return HeaderField("Date");
}


int
BEmailMessage::Priority()
{
	int priorityNumber;
	const char *priorityString;

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
BEmailMessage::SetSubject(const char *subject, uint32 charset,
	mail_encoding encoding)
{
	SetHeaderField("Subject", subject, charset, encoding);
}


void
BEmailMessage::SetReplyTo(const char *reply_to, uint32 charset,
	mail_encoding encoding)
{
	SetHeaderField("Reply-To", reply_to, charset, encoding);
}


void
BEmailMessage::SetFrom(const char *from, uint32 charset, mail_encoding encoding)
{
	SetHeaderField("From", from, charset, encoding);
}


void
BEmailMessage::SetTo(const char *to, uint32 charset, mail_encoding encoding)
{
	SetHeaderField("To", to, charset, encoding);
}


void
BEmailMessage::SetCC(const char *cc, uint32 charset, mail_encoding encoding)
{
	// For consistency with our header names, use Cc as the name.
	SetHeaderField("Cc", cc, charset, encoding);
}


void
BEmailMessage::SetBCC(const char *bcc)
{
	free(_bcc);
	_bcc = strdup(bcc);
}


void
BEmailMessage::SetPriority(int to)
{
	char tempString [20];

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
BEmailMessage::GetName(char *name, int32 maxLength) const
{
	if (name == NULL || maxLength <= 0)
		return B_BAD_VALUE;

	if (BFile *file = dynamic_cast<BFile *>(fData)) {
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
BEmailMessage::GetName(BString *name) const
{
	char *buffer = name->LockBuffer(B_FILE_NAME_LENGTH);
	status_t status = GetName(buffer,B_FILE_NAME_LENGTH);
	name->UnlockBuffer();

	return status;
}


void
BEmailMessage::SendViaAccountFrom(BEmailMessage *message)
{
	BString name;
	if (message->GetAccountName(name) < B_OK) {
		// just return the message with the default account
		return;
	}

	SendViaAccount(name);
}


void
BEmailMessage::SendViaAccount(const char *account_name)
{
	BMailAccounts accounts;
	BMailAccountSettings* account = accounts.AccountByName(account_name);
	if (!account)
		return;
	SendViaAccount(account->AccountID());
}


void
BEmailMessage::SendViaAccount(int32 account)
{
	_account_id = account;

	BMailAccounts accounts;
	BMailAccountSettings* accountSettings = accounts.AccountByID(_account_id);

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
	return _account_id;
}


status_t
BEmailMessage::GetAccountName(BString& accountName) const
{
	BFile *file = dynamic_cast<BFile *>(fData);
	if (file == NULL)
		return B_ERROR;

	int32 accountId;
	size_t read = file->ReadAttr(B_MAIL_ATTR_ACCOUNT, B_INT32_TYPE, 0,
		&accountId, sizeof(int32));
	if (read < sizeof(int32))
		return B_ERROR;

	BMailAccounts accounts;
	BMailAccountSettings* account =  accounts.AccountByID(accountId);
	if (account)
		accountName = account->Name();
	else
		accountName = "";

	return B_OK;
}


status_t
BEmailMessage::AddComponent(BMailComponent *component)
{
	status_t status = B_OK;

	if (_num_components == 0)
		_body = component;
	else if (_num_components == 1) {
		BMIMEMultipartMailContainer *container
			= new BMIMEMultipartMailContainer(
				mime_boundary, mime_warning, _charSetForTextDecoding);
		status = container->AddComponent(_body);
		if (status == B_OK)
			status = container->AddComponent(component);
		_body = container;
	} else {
		BMIMEMultipartMailContainer *container
			= dynamic_cast<BMIMEMultipartMailContainer *>(_body);
		if (container == NULL)
			return B_MISMATCHED_VALUES;

		status = container->AddComponent(component);
	}

	if (status == B_OK)
		_num_components++;
	return status;
}


status_t
BEmailMessage::RemoveComponent(BMailComponent */*component*/)
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


BMailComponent *
BEmailMessage::GetComponent(int32 i, bool parseNow)
{
	if (BMIMEMultipartMailContainer *container
			= dynamic_cast<BMIMEMultipartMailContainer *>(_body))
		return container->GetComponent(i, parseNow);

	if (i < _num_components)
		return _body;

	return NULL;
}


int32
BEmailMessage::CountComponents() const
{
	return _num_components;
}


void
BEmailMessage::Attach(entry_ref *ref, bool includeAttributes)
{
	if (includeAttributes)
		AddComponent(new BAttributedMailAttachment(ref));
	else
		AddComponent(new BSimpleMailAttachment(ref));
}


bool
BEmailMessage::IsComponentAttachment(int32 i)
{
	if ((i >= _num_components) || (_num_components == 0))
		return false;

	if (_num_components == 1)
		return _body->IsAttachment();

	BMIMEMultipartMailContainer *container
		= dynamic_cast<BMIMEMultipartMailContainer *>(_body);
	if (container == NULL)
		return false;

	BMailComponent *component = container->GetComponent(i);
	if (component == NULL)
		return false;
	return component->IsAttachment();
}


void
BEmailMessage::SetBodyTextTo(const char *text)
{
	if (_text_body == NULL) {
		_text_body = new BTextMailComponent;
		AddComponent(_text_body);
	}

	_text_body->SetText(text);
}


BTextMailComponent *
BEmailMessage::Body()
{
	if (_text_body == NULL)
		_text_body = RetrieveTextBody(_body);

	return _text_body;
}


const char *
BEmailMessage::BodyText()
{
	if (Body() == NULL)
		return NULL;

	return _text_body->Text();
}


status_t
BEmailMessage::SetBody(BTextMailComponent *body)
{
	if (_text_body != NULL) {
		return B_ERROR;
//	removing doesn't exist for now
//		RemoveComponent(_text_body);
//		delete _text_body;
	}
	_text_body = body;
	AddComponent(_text_body);

	return B_OK;
}


BTextMailComponent *
BEmailMessage::RetrieveTextBody(BMailComponent *component)
{
	BTextMailComponent *body = dynamic_cast<BTextMailComponent *>(component);
	if (body != NULL)
		return body;

	BMIMEMultipartMailContainer *container
		= dynamic_cast<BMIMEMultipartMailContainer *>(component);
	if (container != NULL) {
		for (int32 i = 0; i < container->CountComponents(); i++) {
			if ((component = container->GetComponent(i)) == NULL)
				continue;

			switch (component->ComponentType()) {
				case B_MAIL_PLAIN_TEXT_BODY:
					// AttributedAttachment returns the MIME type of its
					// contents, so we have to use dynamic_cast here
					body = dynamic_cast<BTextMailComponent *>(
						container->GetComponent(i));
					if (body != NULL)
						return body;
					break;

				case B_MAIL_MULTIPART_CONTAINER:
					body = RetrieveTextBody(container->GetComponent(i));
					if (body != NULL)
						return body;
					break;
			}
		}
	}
	return NULL;
}


status_t
BEmailMessage::SetToRFC822(BPositionIO *mail_file, size_t length,
	bool parse_now)
{
	if (BFile *file = dynamic_cast<BFile *>(mail_file)) {
		file->ReadAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE, 0, &_account_id,
			sizeof(_account_id));
	}

	mail_file->Seek(0,SEEK_END);
	length = mail_file->Position();
	mail_file->Seek(0,SEEK_SET);

	_status = BMailComponent::SetToRFC822(mail_file,length,parse_now);
	if (_status < B_OK)
		return _status;

	_body = WhatIsThis();

	mail_file->Seek(0,SEEK_SET);
	_status = _body->SetToRFC822(mail_file,length,parse_now);
	if (_status < B_OK)
		return _status;

	//------------Move headers that we use to us, everything else to _body
	const char *name;
	for (int32 i = 0; (name = _body->HeaderAt(i)) != NULL; i++) {
		if (strcasecmp(name,"Subject") != 0
			&& strcasecmp(name,"To") != 0
			&& strcasecmp(name,"From") != 0
			&& strcasecmp(name,"Reply-To") != 0
			&& strcasecmp(name,"Cc") != 0
			&& strcasecmp(name,"Priority") != 0
			&& strcasecmp(name,"X-Priority") != 0
			&& strcasecmp(name,"X-Msmail-Priority") != 0
			&& strcasecmp(name,"Date") != 0) {
			RemoveHeader(name);
		}
	}

	_body->RemoveHeader("Subject");
	_body->RemoveHeader("To");
	_body->RemoveHeader("From");
	_body->RemoveHeader("Reply-To");
	_body->RemoveHeader("Cc");
	_body->RemoveHeader("Priority");
	_body->RemoveHeader("X-Priority");
	_body->RemoveHeader("X-Msmail-Priority");
	_body->RemoveHeader("Date");

	_num_components = 1;
	if (BMIMEMultipartMailContainer *container
			= dynamic_cast<BMIMEMultipartMailContainer *>(_body))
		_num_components = container->CountComponents();

	return B_OK;
}


status_t
BEmailMessage::RenderToRFC822(BPositionIO *file)
{
	if (_body == NULL)
		return B_MAIL_INVALID_MAIL;

	// Do real rendering

	if (From() == NULL) {
		// set the "From:" string
		SendViaAccount(_account_id);
	}

	BList recipientList;
	get_address_list(recipientList, To(), extract_address);
	get_address_list(recipientList, CC(), extract_address);
	get_address_list(recipientList, _bcc, extract_address);

	BString recipients;
	for (int32 i = recipientList.CountItems(); i-- > 0;) {
		char *address = (char *)recipientList.RemoveItem(0L);

		recipients << '<' << address << '>';
		if (i)
			recipients << ',';

		free(address);
	}

	// add the date field
	int32 creationTime = time(NULL);
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

	err = _body->RenderToRFC822(file);
	if (err < B_OK)
		return err;

	// Set the message file's attributes.  Do this after the rest of the file
	// is filled in, in case the daemon attempts to send it before it is ready
	// (since the daemon may send it when it sees the status attribute getting
	// set to "Pending").

	if (BFile *attributed = dynamic_cast <BFile *>(file)) {
		BNodeInfo(attributed).SetType(B_MAIL_TYPE);

		attributed->WriteAttrString(B_MAIL_ATTR_RECIPIENTS,&recipients);

		BString attr;

		attr = To();
		attributed->WriteAttrString(B_MAIL_ATTR_TO,&attr);
		attr = CC();
		attributed->WriteAttrString(B_MAIL_ATTR_CC,&attr);
		attr = Subject();
		attributed->WriteAttrString(B_MAIL_ATTR_SUBJECT,&attr);
		attr = ReplyTo();
		attributed->WriteAttrString(B_MAIL_ATTR_REPLY,&attr);
		attr = From();
		attributed->WriteAttrString(B_MAIL_ATTR_FROM,&attr);
		if (Priority() != 3 /* Normal is 3 */) {
			sprintf (attr.LockBuffer (40), "%d", Priority());
			attr.UnlockBuffer(-1);
			attributed->WriteAttrString(B_MAIL_ATTR_PRIORITY,&attr);
		}
		attr = "Pending";
		attributed->WriteAttrString(B_MAIL_ATTR_STATUS, &attr);
		attr = "1.0";
		attributed->WriteAttrString(B_MAIL_ATTR_MIME, &attr);

		attributed->WriteAttr(B_MAIL_ATTR_ACCOUNT, B_INT32_TYPE, 0,
			&_account_id, sizeof(int32));

		attributed->WriteAttr(B_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, &creationTime,
			sizeof(int32));
		int32 flags = B_MAIL_PENDING | B_MAIL_SAVE;
		attributed->WriteAttr(B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, &flags,
			sizeof(int32));

		attributed->WriteAttr(B_MAIL_ATTR_ACCOUNT_ID, B_INT32_TYPE, 0,
			&_account_id, sizeof(int32));
	}

	return B_OK;
}


status_t
BEmailMessage::RenderTo(BDirectory *dir, BEntry *msg)
{
	time_t currentTime;
	char numericDateString [40];
	struct tm timeFields;
	BString worker;

	// Generate a file name for the outgoing message.  See also
	// FolderFilter::ProcessMailMessage which does something similar for
	// incoming messages.

	BString name = Subject();
	SubjectToThread (name); // Extract the core subject words.
	if (name.Length() <= 0)
		name = "No Subject";
	if (name[0] == '.')
		name.Prepend ("_"); // Avoid hidden files, starting with a dot.

	// Convert the date into a year-month-day fixed digit width format, so that
	// sorting by file name will give all the messages with the same subject in
	// order of date.
	time (&currentTime);
	localtime_r (&currentTime, &timeFields);
	sprintf (numericDateString, "%04d%02d%02d%02d%02d%02d",
		timeFields.tm_year + 1900,
		timeFields.tm_mon + 1,
		timeFields.tm_mday,
		timeFields.tm_hour,
		timeFields.tm_min,
		timeFields.tm_sec);
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
	while (name.FindFirst("  ") >= 0) // Remove multiple spaces.
		name.Replace("  " /* Old */, " " /* New */, 1024 /* Count */);

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
	if (status < B_OK)
		return status;

	if (msg != NULL)
		msg->SetTo(dir,worker.String());

	return RenderToRFC822(&file);
}


status_t
BEmailMessage::Send(bool sendNow)
{
	BMailAccounts accounts;
	BMailAccountSettings* account = accounts.AccountByID(_account_id);
	if (!account || !account->HasOutbound()) {
		account = accounts.AccountByID(
			BMailSettings().DefaultOutboundAccount());
		if (!account)
			return B_ERROR;
		SendViaAccount(account->AccountID());
	}

	BString path;
	if (account->OutboundSettings().Settings().FindString("path", &path)
			!= B_OK) {
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
		BMailSettings settings_file;
		if (settings_file.SendOnlyIfPPPUp()) {
			// TODO!
		}

		BMessenger daemon(B_MAIL_DAEMON_SIGNATURE);
		if (!daemon.IsValid())
			return B_MAIL_NO_DAEMON;

		BMessage msg('msnd');
		msg.AddInt32("account",_account_id);
		BPath path;
		message.GetPath(&path);
		msg.AddString("message_path",path.Path());
		daemon.SendMessage(&msg);
	}

	return status;
}


void BEmailMessage::_ReservedMessage1() {}
void BEmailMessage::_ReservedMessage2() {}
void BEmailMessage::_ReservedMessage3() {}
