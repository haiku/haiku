// ClipboardHandler.cpp

#include <Message.h>

#include "ClipboardHandler.h"

// constructor
ClipboardHandler::ClipboardHandler()
				: BHandler()
{
}

// destructor
ClipboardHandler::~ClipboardHandler()
{
}

// MessageReceived
void
ClipboardHandler::MessageReceived(BMessage *message)
{
	switch (message->what) {
		default:
			BHandler::MessageReceived(message);
			break;
	}
}

