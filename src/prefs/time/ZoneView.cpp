/*
	TZoneView.cpp
*/

#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <ListView.h> 
#include <MenuField.h>  
#include <MenuItem.h>
#include <MenuBar.h>
#include <Path.h> 
#include <PopUpMenu.h> 
#include <ScrollView.h>
#include <String.h>
#include <View.h>

#include <stdio.h>

#include "TimeMessages.h"
#include "ZoneView.h"


/*=====> TZoneItem <=====*/

TZoneItem::TZoneItem(const char *text, const char *zonedata)
	: BStringItem(text)
{ 
	f_zonedata = new BPath(zonedata); 
}


TZoneItem::~TZoneItem()
{
	delete f_zonedata;
}


const char *
TZoneItem::GetZone() const
{
	return f_zonedata->Leaf();
}


/*=====> TZoneView <=====*/

TZoneView::TZoneView(BRect frame)
	: BView(frame, "", B_FOLLOW_ALL, 
	  B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP|B_PULSE_NEEDED)
	, f_citylist(NULL)
{
	InitView();
}


TZoneView::~TZoneView()
{
}


void
TZoneView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
TZoneView::MessageReceived(BMessage *message)
{
	int32 change;
	switch(message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch(change) {
				case OB_TM_CHANGED:
					UpdateDateTime(message);
				break;
				
				default:
					BView::MessageReceived(message);
				break;
			}
		break;
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TZoneView::Draw(BRect updaterect)
{
	f_currentzone->Draw(updaterect);
	f_timeinzone->Draw(updaterect);
}


void
TZoneView::UpdateDateTime(BMessage *message)
{
	int32 hour, minute, second;

	if (message->FindInt32("hour", &hour) == B_OK
		&& message->FindInt32("minute", &minute) == B_OK
		&& message->FindInt32("second", &second) == B_OK)
	{
		f_currentzone->SetTo(hour, minute, second);

		// do calc to get other zone time
		f_timeinzone->SetTo(hour, minute, second);
	}
}


void
TZoneView:: InitView()
{
	font_height finfo;
	be_plain_font->GetHeight(&finfo);
	float text_height = finfo.ascent +finfo.descent +finfo.leading;

	GetCurrentRegion();

	BRect bounds(Bounds().InsetByCopy(4, 2));
	BRect frame(bounds);
	
	// Zone menu
	f_zonepopup = new BPopUpMenu("", true, true, B_ITEMS_IN_COLUMN);
	
	float widest = 0;
	CreateZoneMenu(&widest);

	frame.right = frame.left +widest +25;
	frame.bottom = frame.top +text_height -1;
	
	BMenuField *mField;
	mField= new BMenuField(frame, "mfield", NULL, f_zonepopup, true);
	mField->MenuBar()->SetBorder(B_BORDER_CONTENTS);
	mField->MenuBar()->ResizeToPreferred();
	AddChild(mField);


	// City Listing
	
	frame = mField->MenuBar()->Frame();
	frame.top += frame.bottom +1;
	frame.right = mField->Bounds().Width() -B_V_SCROLL_BAR_WIDTH +2;
	frame.bottom = bounds.bottom;
	frame.OffsetBy(2, 0); 
	frame.InsetBy(3, 5);
	
	f_citylist = new BListView(frame, "cities", B_SINGLE_SELECTION_LIST, 
			B_FOLLOW_LEFT|B_FOLLOW_TOP,
			B_WILL_DRAW|B_NAVIGABLE|B_FRAME_EVENTS);

	BMessage *citychange = new BMessage(TIME_ZONE_CHANGED);
	f_citylist->SetSelectionMessage(citychange);
	
	BScrollView *scrollList;
	scrollList = new BScrollView("scroll_list", f_citylist, 
			B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true);
	AddChild(scrollList);
	
	
	// Time Displays
	
	frame.OffsetBy(scrollList->Bounds().Width() +9, 3);
	frame.bottom = frame.top +text_height *2;
	frame.right = bounds.right -6;
	f_currentzone = new TTZDisplay(frame, "current", 
			B_FOLLOW_LEFT|B_FOLLOW_TOP, 0,
			"Current time zone:", B_EMPTY_STRING);
	f_currentzone->ResizeToPreferred();
	
	frame.OffsetBy(0, (text_height *3) +2);
	f_timeinzone = new TTZDisplay(frame, "timein", 
			B_FOLLOW_LEFT|B_FOLLOW_TOP, 0,
			"Time in: ", B_EMPTY_STRING);
	f_timeinzone->ResizeToPreferred();
	
	AddChild(f_currentzone);
	AddChild(f_timeinzone);
	
	
	// set button
	
	frame.Set(bounds.right -75, bounds.bottom -24, 
			bounds.right, bounds.bottom);
	frame.OffsetBy(-2, -2);
	f_settimezone = new BButton(frame, "set", "Set", new BMessage(SET_TIME_ZONE));
	f_settimezone->SetEnabled(false);
	AddChild(f_settimezone);
	
	// update displays	
	int currentzone = FillZoneList(f_RegionStr.Leaf());
	f_citylist->Select(currentzone);
	f_citylist->ScrollToSelection();
	f_citylist->Invalidate();
	f_currentzone->SetText( ((TZoneItem *)f_citylist->ItemAt(currentzone))->Text() );
}


void
TZoneView::CreateZoneMenu(float *widest)
{
	BPath path;
	// return if /etc directory is not found
	if (!(find_directory(B_BEOS_ETC_DIRECTORY, &path) == B_OK))
		return;

	path.Append("timezones");

	float width = 0;
	bool markitem;
	BEntry entry;
	BMenuItem *zoneItem;
	BDirectory dir(path.Path());

	dir.Rewind();
	// walk timezones dir and add entries to menu
	BString itemtext;
	while (dir.GetNextEntry(&entry) == B_NO_ERROR) {
		if (entry.IsDirectory()) {
			path.SetTo(&entry);
			itemtext = path.Leaf();
			if (itemtext.Compare("Etc", 3) == 0)
			 continue;
			 
			// add Ocean to oceans :)
			if ((itemtext.Compare("Atlantic", 8) == 0)
			 ||(itemtext.Compare("Pacific", 7) == 0)
			 ||(itemtext.Compare("Indian", 6) == 0))
				itemtext.Append(" Ocean");

			markitem = itemtext.Compare(f_RegionStr.Leaf()) == 0;
			itemtext = itemtext.ReplaceAll('_', ' ');
			
			width = be_plain_font->StringWidth(itemtext.String());
			if (width> *widest) *widest = width;

			BMessage *zoneMessage = new BMessage(REGION_CHANGED);
			zoneMessage->AddString("region", path.Leaf());

			zoneItem = new BMenuItem(itemtext.String(), zoneMessage);
			zoneItem->SetMarked(markitem);
			f_zonepopup->AddItem(zoneItem);
		}
	}
}


void
TZoneView::ChangeRegion(BMessage *message)
{
	const char *area;
	message->FindString("region", &area);

	FillZoneList(area);
}


int
TZoneView::FillZoneList(const char *area)
{
	//clear listview from previous items
	f_citylist->MakeEmpty();

	//Enter time zones directory. Find subdir with name that matches string stored in area.
	//Enter subdirectory and count the items. For each item, add a StringItem to f_CityList
 	//Time zones directory 

	BPath path;
	// return if /etc directory is not found
	if (!(find_directory(B_BEOS_ETC_DIRECTORY, &path) == B_OK))
		return 0;

	path.Append("timezones");

 	BDirectory zoneDir(path.Path()); 
 	BDirectory cityDir;
 	BStringItem *city;
 	BString city_name;
 	entry_ref ref;
 	
	int index = 0; // index of current setting
 	//locate subdirectory:
	if ( zoneDir.Contains(area, B_DIRECTORY_NODE)) {
		cityDir.SetTo(&zoneDir, area);  //There is a subdir with a name that matches 'area'. That's the one!!
		//iterate over the items in the subdir and fill the listview with TZoneItems:	
		while(cityDir.GetNextRef(&ref) == B_NO_ERROR) {
			city_name = ref.name;
			city_name.ReplaceAll("__", ", ");
			city_name.ReplaceAll("_", " ");
			city = new TZoneItem(city_name.String(), ref.name);
			f_citylist->AddItem(city);
			
			if (strcmp(f_ZoneStr.Leaf(), ref.name) == 0)
				index = f_citylist->IndexOf(city);
		}
	}
	printf("%d\n", index);
	return index;
}


void
TZoneView::GetCurrentTZ()
{
 	BPath path;
 	BEntry tzLink;
 	
 	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
 		path.Append("timezone");
		tzLink.SetTo(path.Path(), true);
		if (!tzLink.InitCheck()) {// link is broken
			// do something like set to a default GMT???
//			return;
		}
 	} else {
 		// set tzlink to default
 	}
 				
	// we need something in the current zone
	f_ZoneStr.SetTo(&tzLink);
}


void
TZoneView::GetCurrentRegion()
{
	GetCurrentTZ();
	
	f_ZoneStr.GetParent(&f_RegionStr);
}


void
TZoneView::NewTimeZone()
{
	BString text;
	int32 selection = f_citylist->CurrentSelection();
	text = ((TZoneItem *)f_citylist->ItemAt(selection))->Text();
	f_timeinzone->SetText(text.String());
	f_settimezone->SetEnabled(true);
}


void
TZoneView::SetTimeZone()
{
	/*
		set time based on supplied timezone. How to do this?
		replace symlink, "timezone", in B_USER_SETTINGS_DIR with a link to the new timezone
	*/
	
	// get path to current link
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;
		
	path.Append("timezone");
		
	// build target from selection
	BPath target;
	target.SetTo(f_RegionStr.Path());
	
	int32 selection = f_citylist->CurrentSelection();
	target.Append(((TZoneItem *)f_citylist->ItemAt(selection))->GetZone());
	
	// create dir object to create link and remove old
	BDirectory dir(target.Path());
	BEntry entry(path.Path());
	if (entry.Exists())
		entry.Remove();
		
	if (dir.CreateSymLink(path.Path(), target.Path(), NULL) != B_OK)
		printf("timezone not linked\n"); 

	// disable button	
	f_settimezone -> SetEnabled(false);
	
	/*
		Still doesn't change runtime timezone setting.
	*/
} 
