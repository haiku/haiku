#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ListView.h>
#include <ScrollView.h>
#include <String.h>

#include "NetworkSetupAddOn.h"

class ServicesAddOn : public NetworkSetupAddOn {
public:
					ServicesAddOn(image_id addon_image);
	BView* 			CreateView(BRect* bounds);
	const char* 	Name() { return "Internet services"; };
};


NetworkSetupAddOn*
get_nth_addon(image_id image, int index)
{
	if (index != 0)
		return NULL;
	
	return new ServicesAddOn(image);
}

// #pragma mark -
ServicesAddOn::ServicesAddOn(image_id image)
	:
	NetworkSetupAddOn(image)
{
}


BView*
ServicesAddOn::CreateView(BRect* bounds)
{	
	BRect r = *bounds;
	if (r.Width() < 100 || r.Height() < 100)
		r.Set(0, 0, 100, 100);
	
	BRect rlv = r.InsetByCopy(2, 2);
	rlv.right -= B_V_SCROLL_BAR_WIDTH;
	BListView* lv = new BListView(rlv, "inetd_services",
						B_SINGLE_SELECTION_LIST, B_FOLLOW_ALL_SIDES);
										
	BScrollView* sv = new BScrollView( "inetd_services_scrollview", lv,
				B_FOLLOW_ALL_SIDES,	B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	FILE *f = fopen("/etc/inetd.conf", "r");
	if (f) {
		char line[1024], *l;
		char *token;
		
		while (fgets(line, sizeof(line), f)) {
			l = line;
			if (! *l)
				continue;

			if (line[0] == '#') {
				if (line[1] == ' ' || line[1] == '\t' ||
					line[1] == '\0' || line[1] == '\n' ||
					line[1] == '\r')
					// skip comments
					continue;
				l++;	// jump the disable/comment service mark
			}
			
			BString label;			
			token = strtok(l, " \t");	// service name
			label << token;
			token = strtok(NULL, " \t");	// type
			label << " (" << token << ")";
			token = strtok(NULL, " \t");	// protocol
			label << " " << token;
			
			lv->AddItem(new BStringItem(label.String()));
		}		
		fclose(f);
	}	

	*bounds = r;
	return sv;
}
	
/*
	BDirectory	folder;
	BEntry		entry;
	
	folder.SetTo ( "/boot/beos/etc/bubblejet" );
	if ( folder.InitCheck() != B_OK )
		return;
		
	while ( folder.GetNextEntry ( &entry ) != B_ENTRY_NOT_FOUND )
		{
		char	name[B_FILE_NAME_LENGTH];
		if ( entry.GetName(name) == B_NO_ERROR )
			m_model_list->AddItem (new BStringItem(name));
		};

	m_model_list->SetSelectionMessage(new BMessage(MODEL_MSG));
	m_model_list->SetInvocationMessage(new BMessage(OK_MSG));
*/
