#ifndef LVIEW_H
#define LVIEW_H

#include <interface/View.h>
#include <interface/ListView.h>
#include <app/MessageRunner.h>

#include "Common.h"

class LView : public BListView {
	public:
		LView(BRect rect,
			const char *name, list_view_type type = B_SINGLE_SELECTION_LIST );
			
		~LView();
		void MessageReceived(BMessage *message);
		

	private:
	
		const BMessage *message_from_tracker;
		
		BMessageRunner *messagerunner;
};

#endif
