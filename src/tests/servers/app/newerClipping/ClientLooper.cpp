
#include <stdio.h>

#include <Message.h>
#include <MessageQueue.h>
#include <String.h>

#include "WindowLayer.h"

#include "ClientLooper.h"

#define SLOW_DRAWING 0

// constructor
ClientLooper::ClientLooper(const char* name, WindowLayer* serverWindow)
	: BLooper("", B_DISPLAY_PRIORITY),
	  fServerWindow(serverWindow),
	  fViewCount(0)
{
	BString clientName(name);
	clientName << " client";
	SetName(clientName.String());
}

// destructor
ClientLooper::~ClientLooper()
{
}

// MessageReceived
void
ClientLooper::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_UPDATE:

			fServerWindow->PostMessage(MSG_BEGIN_UPDATE);

			for (int32 i = 0; i < fViewCount; i++) {
				// the client is slow
//				snooze(40000L);
				// send the command to redraw a view
				BMessage command(MSG_DRAWING_COMMAND);
				command.AddInt32("token", i);
				fServerWindow->PostMessage(&command);
			}

			fServerWindow->PostMessage(MSG_END_UPDATE);

			break;
		case MSG_VIEWS_ADDED: {
			int32 count;
			if (message->FindInt32("count", &count) >= B_OK) {
				fViewCount += count;
			}
			break;
		}
		case MSG_VIEWS_REMOVED: {
			int32 count;
			if (message->FindInt32("count", &count) >= B_OK)
				fViewCount -= count;
			break;
		}
		default:
			BLooper::MessageReceived(message);
			break;
	}
}
