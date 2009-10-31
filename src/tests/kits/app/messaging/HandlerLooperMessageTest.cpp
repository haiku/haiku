#include <Handler.h>
#include <Looper.h>
#include <Message.h>

#include <stdio.h>

const int32 kMessage = 'MeXX';

class Looper : public BLooper {
public:
	Looper();
	virtual void MessageReceived(BMessage *message);
};


class Handler : public BHandler {
public:
	Handler();
	virtual void MessageReceived(BMessage *message);
};

int main()
{
	Looper *looper = new Looper();
	Handler *handler = new Handler();
	looper->AddHandler(new Handler());
	looper->AddHandler(new Handler());
	looper->AddHandler(handler);
	looper->AddHandler(new Handler());
	looper->AddHandler(new Handler());
	
	looper->Run();
	
	looper->PostMessage(new BMessage(kMessage), handler);
	
	snooze(1000000);
	
	looper->Lock();
	looper->Quit();
}


Looper::Looper()
	:
	BLooper("looper")
{
}


void
Looper::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMessage:
			printf("Message received by Looper\n");
			break;
		default:
			BLooper::MessageReceived(message);
			break;
	}
}


Handler::Handler()
	:
	BHandler("handler")
{
}


void
Handler::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			BHandler::MessageReceived(message);
			break;
	}
}
