/*
	
	DataTranslationsView.cpp
	
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include "DataTranslationsView.h"


DataTranslationsView::DataTranslationsView(BRect rect, const char *name,
	list_view_type type = B_SINGLE_SELECTION_LIST)
	: BListView(rect, name, B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES) 
{	
}

DataTranslationsView::~DataTranslationsView() 
{
}

void
DataTranslationsView::MessageReceived(BMessage *message) 
{
	uint32 type; 
	int32 count;
	
	switch (message->what) {
		case B_SIMPLE_DATA:
			// Tell the application object that a
			// file has been dropped on this view
			message->GetInfo("refs", &type, &count); 
			if (count > 0 && type == B_REF_TYPE) {
				message->what = B_REFS_RECEIVED;
				be_app->PostMessage(message);
				Invalidate();
			}
			break;
			
		default:
			BView::MessageReceived(message);
			break;
	}
}

void
DataTranslationsView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	if (msg && transit == B_ENTERED_VIEW) {
		// Draw a red box around the inside of the view
		// to tell the user that this view accepts drops
		SetHighColor(220,0,0);
	 	SetPenSize(4);
		StrokeRect(Bounds());
	 	SetHighColor(0,0,0);
		 	
	} else if (msg && transit == B_EXITED_VIEW)
		Invalidate();
}


