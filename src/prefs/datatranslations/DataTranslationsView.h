/*
 *	
 *	DataTranslationsView.h
 *  
 *  by Oliver Siebenmarck	
*/

#ifndef DATA_TRANSLATIONS_VIEW_H
#define DATA_TRANSLATIONS_VIEW_H

#include <interface/View.h>
#include <interface/ListView.h>
#include <app/MessageRunner.h>


class DataTranslationsView : public BListView {
	public:
		DataTranslationsView(BRect rect,
			const char *name, list_view_type type = B_SINGLE_SELECTION_LIST );
			
		~DataTranslationsView();
		void MessageReceived(BMessage *message);
		virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *msg);
		
	private:
	
				
		BMessageRunner *messagerunner;
};

#endif
