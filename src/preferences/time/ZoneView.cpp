/*
	ZoneView.cpp
		by Mike Berg (inseculous)
		
		Status: 	mimics original Time Zone tab of Time Pref App
		Exceptions: 	doesn't calc "Time in" time.
		Issues: 	After experimenting with both Time Prefs, it seems the original doesn't
				 use the link file in the users settings file to get the current Timezone.
				 Need to find the call it uses to get its inital info so I can get exact duplication.
*/

#include <Button.h>

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <ListView.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <stdio.h>
#include <stdlib.h>
#include <StorageDefs.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include "TimeMessages.h"
#include "ZoneView.h"


/*=====> TZoneItem <=====*/
TZoneItem::TZoneItem(const char *text, const char *zone)
	:BStringItem(text),
	fZone(new BPath(zone))
{
}


TZoneItem::~TZoneItem()
{
	delete fZone;
}


const char *
TZoneItem::Zone() const
{
	return fZone->Leaf();
}


const char *
TZoneItem::Path() const
{
	return fZone->Path();
}


/*=====> TZoneView <=====*/

TZoneView::TZoneView(BRect frame)
	: BView(frame, B_EMPTY_STRING, B_FOLLOW_ALL, B_WILL_DRAW|B_NAVIGABLE_JUMP)
	, f_first(true)
{
	ReadTimeZoneLink();
	InitView();
}


TZoneView::~TZoneView()
{
}



void
TZoneView::AllAttached()
{
	BView::AllAttached();
}


void
TZoneView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());

	if (f_first) // stupid hack
	{
		f_regionpopup->SetTargetForItems(this);
		f_setzone->SetTarget(this);
		f_citylist->SetTarget(this);
		// update displays	
		BPath parent;
		f_currentzone.GetParent(&parent);
		int czone = FillCityList(parent.Path());
		if (czone> -1) {
			f_citylist->Select(czone);
			f_current->SetText( ((TZoneItem *)f_citylist->ItemAt(czone))->Text() );
		}
		f_first = false;
	}
	f_citylist->ScrollToSelection();
}


void
TZoneView::MessageReceived(BMessage *message)
{
	int32 change;
	switch(message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch(change) {
				case H_TM_CHANGED:
					UpdateDateTime(message);
					break;
				
				default:
					BView::MessageReceived(message);
					break;
			}
			break;
		case H_REGION_CHANGED:
			ChangeRegion(message);
			break;
		
		case H_SET_TIME_ZONE:
			SetTimeZone();
			break;
		
		case H_CITY_CHANGED:
			SetPreview();
			break;	
		
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
TZoneView::UpdateDateTime(BMessage *message)
{
	// only need hour and minute
	int32 hour, minute;

	if (message->FindInt32("hour", &hour) == B_OK
		&& message->FindInt32("minute", &minute) == B_OK) {
		if (f_hour != hour || f_minute != minute) {
			f_hour = hour;
			f_minute = minute;
			f_current->SetTo(hour, minute);

			if (f_citylist->CurrentSelection()> -1) {
				// do calc to get other zone time
				SetPreview();
			}
		}
	}
}


void
TZoneView::InitView()
{
	font_height finfo;
	be_plain_font->GetHeight(&finfo);
	float text_height = finfo.ascent +finfo.descent +finfo.leading;

	BRect bounds(Bounds().InsetByCopy(4, 2));
	BRect frame(bounds);
	
	// Zone menu
	f_regionpopup = new BPopUpMenu(B_EMPTY_STRING, true, true, B_ITEMS_IN_COLUMN);

	float widest = 0;
	BuildRegionMenu(&widest);

	frame.right = frame.left +widest +25;
	frame.bottom = frame.top +text_height -1;
	
	BMenuField *mField;
	mField= new BMenuField(frame, "regions", NULL, f_regionpopup, true);
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

	BMessage *citychange = new BMessage(H_CITY_CHANGED);
	f_citylist->SetSelectionMessage(citychange);
	
	BMessage *cityinvoke = new BMessage(H_SET_TIME_ZONE);
	f_citylist->SetInvocationMessage(cityinvoke);
	
	BScrollView *scrollList;
	scrollList = new BScrollView("scroll_list", f_citylist, 
			B_FOLLOW_LEFT|B_FOLLOW_TOP, 0, false, true);
	AddChild(scrollList);
	

	// Time Displays
	
	frame.OffsetBy(scrollList->Bounds().Width() +9, 3);
	frame.bottom = frame.top +text_height *2;
	frame.right = bounds.right -6;
	f_current = new TTZDisplay(frame, "current", 
			B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW,
			"Current time zone:", B_EMPTY_STRING);
	f_current->ResizeToPreferred();
	
	frame.OffsetBy(0, (text_height *3) +2);
	f_preview = new TTZDisplay(frame, "timein", 
			B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW,
			"Time in: ", B_EMPTY_STRING);
	f_preview->ResizeToPreferred();
	
	AddChild(f_current);
	AddChild(f_preview);
	
	// set button
	
	frame.Set(bounds.right -75, bounds.bottom -24, 
			bounds.right, bounds.bottom);
	frame.OffsetBy(-2, -2);
	f_setzone = new BButton(frame, "set", "Set", new BMessage(H_SET_TIME_ZONE));
	f_setzone->SetEnabled(false);
	AddChild(f_setzone);
}


void
TZoneView::BuildRegionMenu(float *widest)
{
	BPath path;
	// return if /etc directory is not found
	if (!(find_directory(B_BEOS_ETC_DIRECTORY, &path) == B_OK))
		return;

	path.Append("timezones");

	// get current region
	BPath region;
	f_currentzone.GetParent(&region);
	
	float width = 0;
	bool markit;
	BEntry entry;
	BMenuItem *item;
	BDirectory dir(path.Path());

	dir.Rewind();

	// walk timezones dir and add entries to menu
	BString itemtext;
	while (dir.GetNextEntry(&entry) == B_NO_ERROR) {
		if (entry.IsDirectory()) {
			path.SetTo(&entry);
			itemtext = path.Leaf();
			
			// skip Etc directory
			if (itemtext.Compare("Etc", 3) == 0)
			 	continue;
			 
			markit = itemtext.Compare(region.Leaf()) == 0;
			
			// add Ocean to oceans :)
			if (itemtext.Compare("Atlantic", 8) == 0
				 ||itemtext.Compare("Pacific", 7) == 0
				 ||itemtext.Compare("Indian", 6) == 0)
				itemtext.Append(" Ocean");

			itemtext = itemtext.ReplaceAll('_', ' ');		// underscores are spaces
			
			width = be_plain_font->StringWidth(itemtext.String());
			if (width > *widest)
				 *widest = width;

			BMessage *msg = new BMessage(H_REGION_CHANGED);
			msg->AddString("region", path.Path());

			item = new BMenuItem(itemtext.String(), msg);
			item->SetMarked(markit);
			f_regionpopup->AddItem(item);
		}
	}
}


int
TZoneView::FillCityList(const char *area)
{
	// clear list
	
	// MakeEmpty() doesn't free items so...
	int32 idx = f_citylist->CountItems();
	if (idx>= 1)
		for (; idx>= 0; idx--)
			delete f_citylist->RemoveItem(idx);
		
	//Enter time zones directory. Find subdir with name that matches string stored in area.
	//Enter subdirectory and count the items. For each item, add a StringItem to f_CityList
 	//Time zones directory 

	BPath path;

	// return if /etc directory is not found
	if (!(find_directory(B_BEOS_ETC_DIRECTORY, &path) == B_OK))
		return 0;

	path.Append("timezones");

	BPath Area(area);
 	BDirectory zoneDir(path.Path()); 
 	BDirectory cityDir;
 	BStringItem *city;
 	BString city_name;
 	BEntry entry;
	int index = -1; 
 	//locate subdirectory:

	if ( zoneDir.Contains(Area.Leaf(), B_DIRECTORY_NODE)) {
		cityDir.SetTo(&zoneDir, Area.Leaf());  
		//There is a subdir with a name that matches 'area'. That's the one!!
		
		
		//iterate over the items in the subdir and fill the listview with TZoneItems:	
		
		while(cityDir.GetNextEntry(&entry) == B_NO_ERROR) {
			if (!entry.IsDirectory()) {
				BPath zone(&entry);
				city_name = zone.Leaf();
				city_name.ReplaceAll("_IN", ", Indiana");
				city_name.ReplaceAll("__Calif", ", Calif");
				city_name.ReplaceAll("__", ", ");
				city_name.ReplaceAll("_", " ");
				city = new TZoneItem(city_name.String(), zone.Path());
				f_citylist->AddItem(city);
				if (strcmp(f_currentzone.Leaf(), zone.Leaf()) == 0)
					index = f_citylist->IndexOf(city);
			}
		}
	}
	return index;
}


void
TZoneView::ChangeRegion(BMessage *message)
{
	BString area;
	message->FindString("region", &area);

	FillCityList(area.String());
}


void
TZoneView::ReadTimeZoneLink()
{
	BEntry tzLink;

#if TARGET_PLATFORM_HAIKU
	extern status_t _kern_get_tzfilename(char *filename, size_t length, bool *_isGMT);
	
	char tzFileName[B_OS_PATH_LENGTH];
	bool isGMT;
	_kern_get_tzfilename(tzFileName, B_OS_PATH_LENGTH, &isGMT);
	tzLink.SetTo(tzFileName);
#else
	/* reads the timezone symlink from B_USER_SETTINGS_DIRECTORY
		currently this sets f_currentzone to the symlink value.
		this is wrong.  the original can get users timezone without 
		a timezone symlink present.  
		
	  defaults are set to different values to clue in on what error was returned
	  	GMT is set when the link is invalid
	  	EST is set when the settings dir can't be found **should never happen**	
	*/
	
 	
 	BPath path;
 	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {

 		path.Append("timezone");
		tzLink.SetTo(path.Path(), true);

		if (tzLink.InitCheck() != B_OK) {// link is broken
			tzLink.SetTo("/boot/beos/etc/timezones/Pacific/fiji");
			// do something like set to a default GMT???
		}
 	} else {
 		// set tzlink to a default
 		tzLink.SetTo("/boot/beos/etc/timezones/EST");
 	}
#endif
	// we need something in the current zone
	f_currentzone.SetTo(&tzLink);
}


void
TZoneView::SetPreview()
{
	// calc and display time based on users selection in city list
	int32 selection = f_citylist->CurrentSelection();
	if (selection>= 0) {
		TZoneItem *item = (TZoneItem *)f_citylist->ItemAt(selection);

		BString text;
		text = item->Text();

		time_t current;
		struct tm *ltime;
		
		// calc time to display and update
		SetTimeZone(item->Path());
		current = time(0);
		ltime = localtime(&current);
		SetTimeZone(f_currentzone.Path());
		
		f_preview->SetTo(ltime->tm_hour, ltime->tm_min);
		f_preview->SetText(text.String());
		
		f_setzone->SetEnabled((strcmp(f_current->Text(), text.String()) != 0));
	}
}


void
TZoneView::SetCurrent(const char *text)
{
	SetTimeZone(f_currentzone.Path());
	time_t current = time(0);
	struct tm *ltime = localtime(&current);
	
	f_current->SetTo(ltime->tm_hour, ltime->tm_min);
	f_current->SetText(text);
}


void
TZoneView::SetTimeZone()
{
	/*
		set time based on supplied timezone. How to do this?
			1) replace symlink, "timezone", in B_USER_SETTINGS_DIR with a link to the new timezone
			2) set TZ environment var
			3) call settz()
			4) call set_timezone from OS.h passing path to timezone file
	*/
	
	
	// update/create timezone symlink in B_USER_SETTINGS_DIRECTORY

	// get path to current link
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;
		
	path.Append("timezone");
	

	// build target for new link
	
	int32 selection = f_citylist->CurrentSelection();
	if (selection < 0)
	  	// nothing selected??
		return;
	
	BPath target( ((TZoneItem *)f_citylist->ItemAt(selection))->Path());
	
	// remove old
	BEntry entry(path.Path());
	if (entry.Exists())
		entry.Remove();
	
	// create new
	BDirectory dir(target.Path());
	if (dir.CreateSymLink(path.Path(), target.Path(), NULL) != B_OK)
		fprintf(stderr, "timezone not linked\n");

	// update environment
	
	char tz[B_PATH_NAME_LENGTH];
	sprintf(tz, "TZ=%s", target.Path());
	
	putenv( tz);
	tzset();

	// update display
	time_t current = time(0);
	struct tm *ltime = localtime(&current);
	
	
	char tza[B_PATH_NAME_LENGTH];
	sprintf(tza, "%s", target.Path());
	set_timezone(tza);
	
	// disable button
	f_setzone -> SetEnabled(false);

	time_t newtime = mktime(ltime);
	ltime = localtime(&newtime);
	stime(&newtime);

	f_hour = ltime->tm_hour;
	f_minute = ltime->tm_min;
	f_currentzone.SetTo(target.Path());
	SetCurrent(((TZoneItem *)f_citylist->ItemAt(selection))->Text());
}	
	

 


void
TZoneView::SetTimeZone(const char *zone)
{
	BString tz;
	
	tz << "TZ=" << zone;
	
	putenv(tz.String());
	tzset();
}


//---//
