// MIMEManager.cpp

#include <Message.h>

#include "MIMEManager.h"

// constructor
MIMEManager::MIMEManager()
		   : BLooper("main_mime")
{
}

// destructor
MIMEManager::~MIMEManager()
{
}

// MessageReceived
void
MIMEManager::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			BLooper::MessageReceived(message);
			break;
	}
}

