/*
	
	DataTranslationsView.cpp
	
*/

#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include "DataTranslationsView.h"


DataTranslationsView::DataTranslationsView(BRect rect, const char *name, list_view_type type = B_SINGLE_SELECTION_LIST) :
	BListView(rect, name, B_SINGLE_SELECTION_LIST) 
{
	
	messagerunner = NULL;
	SetViewColor(217,217,217);
}

void DataTranslationsView::MessageReceived(BMessage *message) 
{

	uint32 type; 
	int32 count;
	
	switch (message->what) {
		case B_SIMPLE_DATA:
			message->GetInfo("refs", &type, &count); 
			if ( count>0 && type == B_REF_TYPE ) 
			{ 
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

void DataTranslationsView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	if ((transit == B_ENTERED_VIEW) && (msg != NULL))
	{
	 	BRect x( 0, 0, 110, 278);
	 	SetHighColor(220,0,0);
	 	SetPenSize(4);
	 	StrokeRect(x);
	 	SetHighColor(0,0,0);
	}
	if ((transit == B_EXITED_VIEW) && (msg != NULL))
	{
	 	Invalidate();
	}
	return;
}


DataTranslationsView::~DataTranslationsView() 
{
	if (messagerunner != NULL) delete messagerunner;
}