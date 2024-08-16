#include "TFindPanelLooper.h"


#include <utility>
#include <iostream>


#include <fs_attr.h>


#include <Message.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <Thread.h>


#include "QueryPoseView.h"
#include "TFindPanel.h"


namespace BPrivate {

TFindPanelLooper::TFindPanelLooper(BQueryPoseView* queryPoseView)
	:
	BLooper(),
	fQueryPoseView(queryPoseView)
{
}


TFindPanelLooper::~TFindPanelLooper()
{
}


void
TFindPanelLooper::MessageReceived(BMessage* message) {
	switch (message->what) {
		case kUpdatePoseView:
		{
			static int count = 1;
			if (!(IsMessageWaiting() || (snooze(150000) && IsMessageWaiting()))) {
				BMessenger messenger(fQueryPoseView);
				messenger.SendMessage(message);
				// std::cout<<"Sent Message"<<std::endl;
			} else {
				BMessageQueue* queue = MessageQueue();
				std::cout<<"Length:"<<queue->CountMessages()<<std::endl;
				std::cout<<"Skipped: "<<count++<<std::endl;
			}
			break;
		}
	
		default:
			_inherited::MessageReceived(message);
			break;
	}
}
}