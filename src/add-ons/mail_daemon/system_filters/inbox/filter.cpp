/* Inbox - places the incoming mail to their destination folder
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <FileConfigView.h>

#include <Directory.h>
#include <String.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <E-mail.h>
#include <Path.h>
#include <Roster.h>
#include <CheckBox.h>
#include <TextControl.h>
#include <StringView.h>

#include <stdio.h>
#include <stdlib.h>
#include <parsedate.h>
#include <unistd.h>

#include <MailAddon.h>
#include <MailSettings.h>
#include <NodeMessage.h>
#include <ChainRunner.h>
#include <status.h>
#include <mail_util.h>

#include <MDRLanguage.h>

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


class FolderFilter : public BMailFilter
{
	BString dest_string;
	BDirectory destination;
	int32 chain_id;
	BMailChainRunner *runner;
	int fNumberOfFilesSaved;
	int size_limit; // Messages larger than this many bytes get partially downloaded.  -1 for always do full download.

  public:
	FolderFilter(BMessage*,BMailChainRunner *);
	virtual ~FolderFilter();
	virtual status_t InitCheck(BString *err);
	virtual status_t ProcessMailMessage
	(
		BPositionIO** io_message, BEntry* io_entry,
		BMessage* io_headers, BPath* io_folder, const char* io_uid
	);
};


FolderFilter::FolderFilter(BMessage* msg,BMailChainRunner *therunner)
	: BMailFilter(msg),
	chain_id(msg->FindInt32("chain")), runner(therunner), size_limit(-1)
{
	fNumberOfFilesSaved = 0;
	dest_string = runner->Chain()->MetaData()->FindString("path");
	create_directory(dest_string.String(),0777);
	destination = dest_string.String();
	
	if (msg->FindInt32("size_limit",(long *)&size_limit) != B_OK)
		size_limit = -1;
}


FolderFilter::~FolderFilter ()
{
	// Save the disk cache to the actual disk so mail data won't get lost if a
	// crash happens soon after mail has been received or sent.  Mostly put
	// here because of unexpected mail daemon activity during debugging of
	// kernel crashing software.

	if (fNumberOfFilesSaved > 0)
		sync ();
}


status_t FolderFilter::InitCheck(BString* err)
{
	
	status_t ret = destination.InitCheck();

	if (ret==B_OK) return B_OK;
	else
	{
		if (err) *err
		<< "FolderFilter failed: destination '" << dest_string
		<< "' not found (" << strerror(ret) << ").";
		return ret;
	}
}

status_t FolderFilter::ProcessMailMessage(BPositionIO**io, BEntry* e, BMessage* out_headers, BPath*loc, const char* io_uid)
{
	time_t			dateAsTime;
	const time_t   *datePntr;
	ssize_t			dateSize;
	char			numericDateString [40];
	bool			tempBool;
	struct tm   	timeFields;
	BString			worker;

	BDirectory		dir;
	
	BPath path = dest_string.String();
	if (out_headers->HasString("DESTINATION")) {
		const char *string;
		out_headers->FindString("DESTINATION",&string);
		if (string[0] == '/')
			path = string;
		else
			path.Append(string);
	} else if (loc != NULL && loc->Path() != NULL && strcmp(loc->Path(),"") != 0) // --- Don't append folder names to overridden paths
		path.Append(loc->Path());

	create_directory(path.Path(),0777);
	dir.SetTo(path.Path());

	BNode node(e);
	status_t err = 0;
	bool haveReadWholeMessage = false;
	// "ENTIRE_MESSAGE" really means the user has double clicked on a partial
	// message and now it should be fully read, and then displayed to the user.
	if ((out_headers->FindBool("ENTIRE_MESSAGE", &tempBool) == B_OK && tempBool)
		|| !out_headers->HasInt32("SIZE")
		|| (size_limit < 0 || size_limit >= out_headers->FindInt32("SIZE"))) {
		err = (*io)->Seek(0,SEEK_END); // Force protocol to read the whole message.
		if (err < 0)
		{
			BString error;
			MDR_DIALECT_CHOICE (
				error << "Unable to read whole message from server, ignoring it.  "
					<< "Subject \"" << out_headers->FindString("Subject")
					<< "\", save to dir \"" << path.Path() <<
					"\", error code: " << err << " " << strerror(err);
			,
				error << out_headers->FindString("Subject") << " のメッセージを " <<
					path.Path() << "に保存中にエラーが発生しました" << strerror(err);
			)
			runner->ShowError(error.String());
			return B_MAIL_END_FETCH; // Stop reading further mail messages.
		}
		haveReadWholeMessage = true;
	}

	BNodeInfo info(&node);
	node.Sync();
	off_t size;
	node.GetSize(&size);
	// Note - sometimes the actual message size is a few bytes more than the
	// registered size, so use >= when testing.  And sometimes the message is
	// actually slightly smaller, due to POP server errors (some count double
	// dots correctly, some don't, if it causes problems, you'll get a partial
	// message and waste time downloading it twice).
	if (haveReadWholeMessage ||
		(out_headers->HasInt32("SIZE") && size >= out_headers->FindInt32("SIZE"))) {
		info.SetType(B_MAIL_TYPE);
		// A little fixup for incorrect registered sizes, so that the full
		// length attribute gets written correctly later.
		if (out_headers->HasInt32("SIZE"))
			out_headers->ReplaceInt32("SIZE", size);
		haveReadWholeMessage = true;
	} else // Don't have the whole message.
		info.SetType("text/x-partial-email");

	BMessage attributes;

	attributes.AddString("MAIL:unique_id",io_uid);
	attributes.AddString("MAIL:account",BMailChain(chain_id).Name());
	attributes.AddInt32("MAIL:chain",chain_id);

	size_t length = (*io)->Position();
	length -= out_headers->FindInt32(B_MAIL_ATTR_HEADER);
	if (attributes.ReplaceInt32(B_MAIL_ATTR_CONTENT,length) != B_OK)
		attributes.AddInt32(B_MAIL_ATTR_CONTENT,length);

	const char *buf;
	time_t when;
	for (int i = 0; gDefaultFields[i].rfc_name; ++i)
	{
		out_headers->FindString(gDefaultFields[i].rfc_name,&buf);
		if (buf == NULL)
			continue;

		switch (gDefaultFields[i].attr_type){
		case B_STRING_TYPE:
			attributes.AddString(gDefaultFields[i].attr_name, buf);
			break;

		case B_TIME_TYPE:
			when = ParseDateWithTimeZone (buf);
			if (when == -1)
				when = time (NULL); // Use current time if it's undecodable.
			attributes.AddData(B_MAIL_ATTR_WHEN, B_TIME_TYPE, &when, sizeof(when));
			break;
		}
	}
	
	if (out_headers->HasInt32("SIZE")) {
		size_t size = out_headers->FindInt32("SIZE");
		attributes.AddData("MAIL:fullsize",B_SIZE_T_TYPE,&size,sizeof(size_t));
	}
		
	// add "New" status, if the status hasn't been set already
	if (attributes.FindString(B_MAIL_ATTR_STATUS,&buf) < B_OK)
		attributes.AddString(B_MAIL_ATTR_STATUS,"New");

	node << attributes;

	// Move the message file out of the temporary directory, else it gets
	// deleted.  Partial messages have already been moved, so don't move them.
	if (out_headers->FindBool("ENTIRE_MESSAGE", &tempBool) != B_OK
	|| tempBool == false) {
		err = B_OK;
		if (!dir.Contains(e))
			err = e->MoveTo(&dir);
		if (err != B_OK)
		{
			BString error;
			MDR_DIALECT_CHOICE (
				error << "An error occurred while moving the message " <<
					out_headers->FindString("Subject") << " to " << path.Path() <<
					": " << strerror(err);
				,
				error << out_headers->FindString("Subject") << " のメッセージを " <<
					path.Path() << "に保存中にエラーが発生しました" << strerror(err);
			)
			runner->ShowError(error.String());
			return err;
		}
	}

	// Generate a file name for the incoming message.  See also
	// Message::RenderTo which does a similar thing for outgoing messages.

	BString name = attributes.FindString("MAIL:subject");
	SubjectToThread (name); // Extract the core subject words.
	if (name.Length() <= 0)
		name = "No Subject";
	if (name[0] == '.')
		name.Prepend ("_"); // Avoid hidden files, starting with a dot.

	// Convert the date into a year-month-day fixed digit width format, so that
	// sorting by file name will give all the messages with the same subject in
	// order of date.
	dateAsTime = 0;
	if (attributes.FindData(B_MAIL_ATTR_WHEN, B_TIME_TYPE,
		(const void **) &datePntr, &dateSize) == B_OK)
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

	worker = attributes.FindString("MAIL:from");
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
	int32 tries = 20;
	while ((err = e->Rename(worker.String())) == B_FILE_EXISTS && --tries > 0) {
		srand(rand());
		uniquer += (rand() >> 16) - 16384;

		worker = name;
		worker << ' ' << uniquer;
	}
	if (err < B_OK)
		printf("FolderFilter::ProcessMailMessage: could not rename mail (%s)! "
			"(should be: %s)\n",strerror(err),worker.String());

	fNumberOfFilesSaved++;
	if (out_headers->FindBool("ENTIRE_MESSAGE")) {
		entry_ref ref;
		e->GetRef(&ref);
		be_roster->Launch(&ref);
	}
		
	return B_OK;
}


BMailFilter* instantiate_mailfilter(BMessage* settings, BMailChainRunner *run)
{
	return new FolderFilter(settings,run);
}

class FolderConfig : public BView {
	public:
		FolderConfig(BMessage *settings, BMessage *meta_data) :  BView(BRect(0,0,50,50),"folder_config",B_FOLLOW_ALL_SIDES,0) {

			const char *partial_text = MDR_DIALECT_CHOICE (
				"Partially download messages larger than",
				"部分ダウンロードする");

			fPathView = new BMailFileConfigView(MDR_DIALECT_CHOICE ("Location:","受信箱："),
												"path",true,"/boot/home/mail/in");
			fPathView->SetTo(settings,meta_data);
			fPathView->ResizeToPreferred();
			
			BRect r(fPathView->Frame());
			r.OffsetBy(0,r.Height() + 5);
			fPartialBox = new BCheckBox(r, "size_if", partial_text, new BMessage('SIZF'));
			fPartialBox->ResizeToPreferred();
			
			r = fPartialBox->Frame();
			r.OffsetBy(17,r.Height() + 1);
			r.right = r.left + be_plain_font->StringWidth("0000") + 10;
			fSizeBox = new BTextControl(r, "size", "", "", NULL);
			
			r.OffsetBy(r.Width() + 5,0);
			fBytesLabel = new BStringView(r,"kb", "KB");
			AddChild(fBytesLabel);
			fSizeBox->SetDivider(0);
			if (settings->HasInt32("size_limit")) {
				BString kb;
				kb << int32(settings->FindInt32("size_limit")/1024);
				fSizeBox->SetText(kb.String());
				fPartialBox->SetValue(B_CONTROL_ON);
			} else
				fSizeBox->SetEnabled(false);
			AddChild(fPathView);
			SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
			AddChild(fPartialBox);
			AddChild(fSizeBox);
			ResizeToPreferred();
		}
		void MessageReceived(BMessage *msg) {
			if (msg->what != 'SIZF')
				return BView::MessageReceived(msg);
			fSizeBox->SetEnabled(fPartialBox->Value());
		}
		void AttachedToWindow() {
			fPartialBox->SetTarget(this);
			fPartialBox->ResizeToPreferred();
		}
		void GetPreferredSize(float *width, float *height) {
			fPathView->GetPreferredSize(width,height);
//			*height = fPartialBox->Frame().bottom + 5;
			*height = fBytesLabel->Frame().bottom + 5;
			*width = MAX(fBytesLabel->Frame().right + 5, *width);
		}
		status_t Archive(BMessage *into, bool) const {
			into->MakeEmpty();
			fPathView->Archive(into);
			if (fPartialBox->Value())
				into->AddInt32("size_limit",atoi(fSizeBox->Text()) * 1024);
			return B_OK;
		}
			
			
	private:
		BMailFileConfigView *fPathView;
		BTextControl *fSizeBox;
		BCheckBox *fPartialBox;
		BStringView *fBytesLabel;
};

BView* instantiate_config_panel(BMessage *settings, BMessage *meta_data)
{

	return new FolderConfig(settings,meta_data);
}

