#include <Button.h>
#include "NetListView.h"

// The NetListView only exists to override the SelectionChanged() function below
NetListView::NetListView(BRect frame,const char *name) : BListView(frame, name)
{
}

// Buttons are enabled or not depending on the selection in the list
// so each time you click in the list, this method is called to update the buttons
void NetListView::SelectionChanged(void)
{
	int32 index = CurrentSelection();

	BView *parent = Parent();
	
	if (index >= 0) {
		
		// There are 2 lists that are instances of this class, so we have to see in which list you are clicking
		if (strcmp(parent->Name(),"Interfaces_Scroller") == 0) {												  
			
			BButton *settings = dynamic_cast <BButton *> ((parent->Parent())->FindView("Settings"));
			BButton *clear 	  = dynamic_cast <BButton *> ((parent->Parent())->FindView("Clear"));
			
			settings->SetEnabled(true);
			clear->SetEnabled(true);
		}
		else if (strcmp(parent->Name(),"Configurations_Scroller") == 0) {
			
			BButton *restore = dynamic_cast <BButton *> ((parent->Parent())->FindView("Restore"));
			BButton *remove	 = dynamic_cast <BButton *> ((parent->Parent())->FindView("Delete"));
			
			restore ->SetEnabled(true);
			remove  ->SetEnabled(true);
		}
	}
	else {
		
		// There are 2 lists that are instances of this class, so we have to see in which list you are clicking
		if (strcmp(parent->Name(),"Interfaces_Scroller") == 0) {												  
		
			BButton *settings=dynamic_cast <BButton *> ((parent->Parent())->FindView("Settings"));
			BButton *clear=dynamic_cast <BButton *> ((parent->Parent())->FindView("Clear"));
			
			settings->SetEnabled(false);
			clear->SetEnabled(false);
		}
		else if (strcmp(parent->Name(),"Configurations_Scroller") == 0) {
		
			BButton *restore=dynamic_cast <BButton *> ((parent->Parent())->FindView("Restore"));
			BButton *remove=dynamic_cast <BButton *> ((parent->Parent())->FindView("Delete"));
			
			restore->SetEnabled(false);
			remove->SetEnabled(false);
		}
	}			
				
}

NetListItem::NetListItem(const InterfaceData &data)
 :	BStringItem(data.fPrettyName.String()),
 	fData(data)
{
}
