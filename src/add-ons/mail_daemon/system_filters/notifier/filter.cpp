/* New Mail Notification - notifies incoming e-mail
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Message.h>
#include <String.h>
#include <Alert.h>
#include <Beep.h>
#include <Application.h>

#include <MailAddon.h>
#include <ChainRunner.h>
#include <status.h>

#include <MDRLanguage.h>

#include "ConfigView.h"

class NotifyFilter;

class NotifyCallback : public BMailChainCallback {
	public:
		NotifyCallback (int32 notification_method, BMailChainRunner *us,NotifyFilter *ref2);
		virtual void Callback(status_t result);
		
		uint32 num_messages;
	private:
		BMailChainRunner *chainrunner;
		int32 strategy;
		NotifyFilter *parent;
};

class NotifyFilter : public BMailFilter
{
  public:
	NotifyFilter(BMessage*,BMailChainRunner*);
	virtual status_t InitCheck(BString *err);
	virtual status_t ProcessMailMessage
	(
		BPositionIO** io_message, BEntry* io_entry,
		BMessage* io_headers, BPath* io_folder, const char* io_uid
	);
	
  private:
  	friend class NotifyCallback;
  	
  	NotifyCallback *callback;
  	BMailChainRunner *_runner;
  	int32 strategy;
};


NotifyFilter::NotifyFilter(BMessage* msg,BMailChainRunner *runner)
	: BMailFilter(msg), _runner(runner), callback(NULL)
{
	strategy = msg->FindInt32("notification_method");
}

status_t NotifyFilter::InitCheck(BString* err)
{
	return B_OK;
}

status_t NotifyFilter::ProcessMailMessage(BPositionIO**, BEntry*, BMessage*headers, BPath*, const char*)
{
	if (callback == NULL) {
		callback = new NotifyCallback(strategy,_runner,this);
		_runner->RegisterProcessCallback(callback);
	}
	
	if (!headers->FindBool("ENTIRE_MESSAGE"))	
		callback->num_messages ++;
	
	return B_OK;
}

NotifyCallback::NotifyCallback (int32 notification_method, BMailChainRunner *us,NotifyFilter *ref2) : 
	strategy(notification_method),
	chainrunner(us),
	num_messages(0), parent(ref2)
{
}

void NotifyCallback::Callback(status_t result) {
	parent->callback = NULL;
	
	if (num_messages == 0)
		return;
	
	if (strategy & do_beep)
		system_beep("New E-mail");
		
	if (strategy & alert) {
		BString text;
		MDR_DIALECT_CHOICE (
		text << "You have " << num_messages << " new message" << ((num_messages != 1) ? "s" : "")
		<< " for " << chainrunner->Chain()->Name() << ".",

		text << chainrunner->Chain()->Name() << "より\n" << num_messages << " 通のメッセージが届きました");
		
		BAlert *alert = new BAlert(MDR_DIALECT_CHOICE ("New Messages","新着メッセージ"), text.String(), "OK", NULL, NULL, B_WIDTH_AS_USUAL);
		alert->SetFeel(B_NORMAL_WINDOW_FEEL);
		alert->Go(NULL);
	}
	
	if (strategy & blink_leds)
		be_app->PostMessage('mblk');
	
	if (strategy & one_central_beep)
		be_app->PostMessage('mcbp');
	
	if (strategy & big_doozy_alert) {
		BMessage msg('numg');
		msg.AddInt32("num_messages",num_messages);
		msg.AddString("chain_name",chainrunner->Chain()->Name());
		msg.AddInt32("chain_id",chainrunner->Chain()->ID());
		
		be_app->PostMessage(&msg);
	}
	
	if (strategy & log_window) {
		BString message;
		message << num_messages << " new message" << ((num_messages != 1) ? "s" : "");
		chainrunner->ShowMessage(message.String());
	}
}

BMailFilter* instantiate_mailfilter(BMessage* settings, BMailChainRunner *runner)
{
	return new NotifyFilter(settings,runner);
}

