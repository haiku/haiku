/*
	TZoneView.cpp
*/

#include <AppDefs.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Handler.h>
#include <ListItem.h> 
#include <ListView.h> 
#include <MenuField.h>  
#include <MenuItem.h>
#include <MenuBar.h>
#include <Path.h> 
#include <PopUpMenu.h> 
#include <ScrollView.h>
#include <String.h>
#include <StringView.h>
#include <View.h>

#include <stdio.h>
#include <time.h>

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
	return f_zonedata->Path();
}


/*=====> TZoneView <=====*/

TZoneView::TZoneView(BRect frame)
	: BView(frame,"",B_FOLLOW_ALL, 
			B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE_JUMP)
	, f_citylist(NULL)
{
	InitView();
} //TZoneView::TZoneView()


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
	switch(message->what)
	{
		case B_OBSERVER_NOTICE_CHANGE:
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &change);
			switch (change)
			{
				case OB_TIME_UPDATE:
				{
					UpdateDateTime(message);
					break;
				}
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
	frame.bottom = frame.top +text_height;
	
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
								B_FOLLOW_LEFT|B_FOLLOW_TOP, 
								0, false, true);
	AddChild(scrollList);
	
	
	// Time displays
	BStringView *displaytext;

	BString labels = "Current time zone:";
	frame.OffsetBy(scrollList->Bounds().Width() +9, 2);
	frame.bottom = frame.top +text_height;
	displaytext = new BStringView(frame, "current", labels.String());
	AddChild(displaytext);
	
	float oldleft = frame.left;
	float width = be_plain_font->StringWidth("00:00 PM") +8;
	
	frame.OffsetBy(0, text_height +2);
	frame.right = bounds.right -(width +4);
	f_curtext = new BStringView(frame, "currenttext", "Current time display");

	frame.right = bounds.right;
	frame.left = frame.right -width;
	
	f_curtime = new BStringView(frame, "currenttime", "cTIME");
	f_curtime->SetAlignment(B_ALIGN_RIGHT);
	AddChild(f_curtext);
	AddChild(f_curtime);	
	
	labels = "Time in:";
	frame.OffsetTo(oldleft, frame.top +text_height *2);
	displaytext = new BStringView(frame, "timein", labels.String());
	AddChild(displaytext);
	
	frame.OffsetBy(0, text_height +2);
	frame.right = bounds.right - (width -2);
	frame.bottom = frame.top +text_height;
	f_seltext = new BStringView(frame, "selectedtext", "Selected time display");
	
	frame.right = bounds.right;
	frame.left = bounds.right -width;
	f_seltime = new BStringView(frame, "selectedtime", "sTIME");
	f_seltime->SetAlignment(B_ALIGN_RIGHT);
	AddChild(f_seltext);
	AddChild(f_seltime);
	
	
	// set button
	
	frame.Set(bounds.right -75, bounds.bottom -24, 
			bounds.right, bounds.bottom);
	frame.OffsetBy(-2, -2);
	f_settimezone = new BButton(frame, "set", "Set", new BMessage(SET_TIME_ZONE));
	f_settimezone->SetEnabled(false);
	AddChild(f_settimezone);
	
	
	// update displays	
	
	FillZoneList(f_RegionStr.Leaf());
	UpdateTimeDisplay(OB_CURRENT_TIME);	
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
	while (dir.GetNextEntry(&entry) == B_NO_ERROR)
	{
		if (entry.IsDirectory())
		{
			path.SetTo(&entry);
			itemtext = path.Leaf();
			if (itemtext.Compare("Etc", 3) == 0)
			 continue;
			 
			// add Ocean to oceans :)
			
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


void
TZoneView::FillZoneList(const char *area)
{
	//clear listview from previous items
	f_citylist->MakeEmpty();

	//Enter time zones directory. Find subdir with name that matches string stored in area.
	//Enter subdirectory and count the items. For each item, add a StringItem to f_CityList
 	//Time zones directory 
 	BDirectory zoneDir("/boot/beos/etc/timezones/"); 
 	BDirectory cityDir;
 	BStringItem *city;
 	BString city_name;
 	entry_ref ref;
 	
	
 	//locate subdirectory:
	if ( zoneDir.Contains(area, B_DIRECTORY_NODE) )
	{
		cityDir.SetTo(&zoneDir, area);  //There is a subdir with a name that matches 'area'. That's the one!!
		//iterate over the items in the subdir and fill the listview with TZoneItems:	
		while(cityDir.GetNextRef(&ref) == B_NO_ERROR)
		{
			city_name = ref.name;
			city_name.ReplaceAll("__", ", ");
			city_name.ReplaceAll("_", " ");
			city = new TZoneItem(city_name.String(), ref.name);
			f_citylist->AddItem(city);
		}
	}
}


void
TZoneView::UpdateDateTime(BMessage *message)
{
	struct tm *ltime;
	if (message->FindPointer("_tm_", (void **)&ltime) == B_OK)
	{
		if (f_hour != ltime->tm_hour || f_minutes != ltime->tm_min)
		{
			f_hour = ltime->tm_hour;
			f_minutes = ltime->tm_min;
			Update();
		}
	}
}


void
TZoneView::Update()
{
	BString text;
	if (f_hour -12 < 10)
		text << "0";
	text << f_hour-12 << ":";

	if (f_minutes < 10)
		text << "0";
	text << f_minutes << " ";
	
	if (f_hour> 12)
		text << "PM";
	else
		text << "AM";
	f_curtime->SetText(text.String());
}


void
TZoneView::GetCurrentTZ()
{

 	BPath path;
 	BEntry tzLink;
 	
 	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK)
 	{
 		path.Append("timezone");
		tzLink.SetTo(path.Path(), true);
		if (!tzLink.InitCheck()) // link is broken
		{
			// do something like set to a default GMT???
//			return;
		}
 	}
 	else
 	{
 		// set tzlink to default
 	}
 				
	// we need something in the current zone
	f_ZoneStr.SetTo(&tzLink);
 	
}//TZoneView::getCurrentTz()


void
TZoneView::GetCurrentRegion()
{
	GetCurrentTZ();
	
	f_ZoneStr.GetParent(&f_RegionStr);
}


void
TZoneView::NewTimeZone()
{
	UpdateTimeDisplay(OB_SELECTION_TIME);
}//TZoneView::newTimeZone()


void
TZoneView::SetTimeZone()
{
	/*
		set time based on supplied timezone. How to do this?
		replace symlink, "timezone", in B_USER_SETTINGS_DIR with a link to the new timezone
	*/
	
	f_settimezone -> SetEnabled(false);
} //TZoneView::setTimeZone()


void
TZoneView::UpdateTimeDisplay(OB_TIME_TARGET target)
{
	switch (target)
	{
		case OB_CURRENT_TIME:
		{
			BString text(f_ZoneStr.Leaf());
			text.ReplaceAll("_", " ");
			f_curtext->SetText(text.String());
			Draw(Bounds());
		}
		break;
		case OB_SELECTION_TIME:
		{
			BString text;
			int32 selection = f_citylist->CurrentSelection();
			text = ((TZoneItem *)f_citylist->ItemAt(selection))->Text();
			f_seltext->SetText(text.String());
		}
		break;
		default:
		break;
	}
}
