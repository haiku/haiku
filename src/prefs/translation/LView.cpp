#include "LView.h"
#include "Common.h"

#ifndef _APPLICATION_H
#include <Application.h>
#endif

LView::LView(BRect rect, const char *name, list_view_type type = B_SINGLE_SELECTION_LIST) :
	BListView(rect, name, B_SINGLE_SELECTION_LIST) {
	
	message_from_tracker = NULL;
	messagerunner = NULL;
	SetViewColor(217,217,217);
}



void LView::MessageReceived(BMessage *message) {
	
	uint32 type; 
	int32 count;
	
	switch (message->what) {
		case B_SIMPLE_DATA:
			message->GetInfo("refs", &type, &count); 
			if ( count>0 && type == B_REF_TYPE ) 
			{ 
				message->what = B_REFS_RECEIVED;
				be_app->PostMessage(message);
			}
			break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


LView::~LView() 
{
	if (messagerunner != NULL) delete messagerunner;
}