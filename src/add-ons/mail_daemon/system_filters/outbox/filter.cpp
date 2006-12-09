/* Outbox - scans outgoing mail in a specific folder
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Directory.h>
#include <String.h>
#include <Entry.h>
#include <NodeInfo.h>
#include <Path.h>
#include <E-mail.h>

#include <stdio.h>

#include <MailAddon.h>
#include <NodeMessage.h>
#include <ChainRunner.h>
#include <status.h>
#include <FileConfigView.h>
#include <StringList.h>

#include <MDRLanguage.h>

class StatusChanger : public BMailChainCallback {
	public:
		StatusChanger(const char * entry);
		void Callback(status_t result);
		
	private:
		const char * to_change;
};

class DiskProducer : public BMailFilter
{	
	BMailChainRunner *runner;
	status_t init;
	
  public:
	DiskProducer(BMessage*,BMailChainRunner*);
	virtual status_t InitCheck(BString *err);
	virtual status_t ProcessMailMessage
	(
		BPositionIO** io_message, BEntry* io_entry,
		BMessage* io_headers, BPath* io_folder, const char* io_uid
	);
};

DiskProducer::DiskProducer(BMessage* msg,BMailChainRunner*status)
	: BMailFilter(msg), runner(status), init(B_OK)
{}

status_t DiskProducer::InitCheck(BString* err)
{
	if (init != B_OK)
		return init;
	
	return B_OK;
}

status_t DiskProducer::ProcessMailMessage(BPositionIO**io, BEntry* e, BMessage* out_headers, BPath*, const char* io_uid)
{
	e->Remove();
	
	BFile *file = new BFile(io_uid,B_READ_WRITE);
	
	e->SetTo(io_uid);
	*file >> *out_headers;
	*io = file;
	
	runner->RegisterMessageCallback(new StatusChanger(io_uid));
		
	return B_OK;
}

StatusChanger::StatusChanger(const char * entry)
	: to_change(entry)
{
}

void StatusChanger::Callback(status_t result) {
	BNode node(to_change);
	
	if (result == B_OK) {
		mail_flags flags = B_MAIL_SENT;
		
		node.WriteAttr(B_MAIL_ATTR_FLAGS,B_INT32_TYPE,0,&flags,4);
		node.WriteAttr(B_MAIL_ATTR_STATUS,B_STRING_TYPE,0,"Sent",5);
	} else {
		node.WriteAttr(B_MAIL_ATTR_STATUS,B_STRING_TYPE,0,"Error",6);
	}
}
		

BMailFilter* instantiate_mailfilter(BMessage* settings, BMailChainRunner *runner)
{
	return new DiskProducer(settings,runner);
}


BView* instantiate_config_panel(BMessage *settings,BMessage *metadata)
{
	BMailFileConfigView *view = new BMailFileConfigView(MDR_DIALECT_CHOICE ("Location:","送信箱："),"path",true,"/boot/home/mail/out");
	view->SetTo(settings,metadata);

	return view;
}

