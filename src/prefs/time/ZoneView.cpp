/*
	ZoneView.cpp
*/

#include <Button.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Directory.h>
#include <ListItem.h> 
#include <ListView.h> 
#include <MenuField.h>  
#include <MenuItem.h>
#include <Path.h> 
#include <PopUpMenu.h> 
#include <ScrollView.h>
#include <string.h>
#include <StringView.h>
#include <time.h>
#include <View.h>

#include "TimeMessages.h"
#include "ZoneView.h"

ZoneView::ZoneView(BRect frame):
	BView(frame,"",B_FOLLOW_ALL,B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP | B_PULSE_NEEDED)
	{
		buildView();
	} //ZoneView::ZoneView()
	
void
ZoneView:: buildView()
{
	//viewcolor
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	//BMenuField with BPopUpMenu for the timezones.
	//The box...
	BRect boxrect(8.0, 1.0, 157.0, 20.0);
	
	//popup menu
	fZonePopUp = new BPopUpMenu("",true, true, B_ITEMS_IN_COLUMN);
	
	//Menufield..
	BMenuField *mField;
	mField= new BMenuField(boxrect, "mfield", NULL, fZonePopUp, true, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE);
	
	AddChild(mField);
	
	
	//Adding listview. Matsu
	BRect flistframe(10.0, 25.0, 140.0, 170.0); 
	//BListView *fCityList; to header
	fCityList= new BListView(flistframe,"cities", B_SINGLE_SELECTION_LIST, B_FOLLOW_LEFT | B_FOLLOW_TOP,
								B_WILL_DRAW | B_NAVIGABLE | B_FRAME_EVENTS);
	//selectionmessage
	BMessage *cityChange = new BMessage(TIME_ZONE_CHANGED);								
	fCityList -> SetSelectionMessage(cityChange);
	
	//place listview in a scrollview. matsu
	BScrollView *scrollList;
	scrollList = new BScrollView("scroll_list", fCityList, B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true);
	AddChild(scrollList);
	
	//adding menuitems
	BMenuItem *zoneItem;
	BMessage *zoneMessage;
	
	zoneItem= new BMenuItem("Africa", zoneMessage = new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Africa");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Antarctica", zoneMessage = new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Antarctica");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Arctic", zoneMessage = new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Arctic");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Asia", zoneMessage = new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Asia");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Atlantic Ocean", zoneMessage = new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Atlantic");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Australia", zoneMessage = new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Australia");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Europe", zoneMessage = new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Europe");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Greenland", zoneMessage = new BMessage(REGION_CHANGED));
	zoneMessage -> AddString("region", "Greenland");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Indian Ocean", zoneMessage= new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Indian");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("North and Central America", zoneMessage= new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "North_and_Central_America");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("Pacific Ocean", zoneMessage = new BMessage(REGION_CHANGED)); 
	zoneMessage -> AddString("region", "Pacific");
	fZonePopUp -> AddItem(zoneItem);
	zoneItem= new BMenuItem("South America", zoneMessage = new BMessage(REGION_CHANGED));
	zoneMessage -> AddString("region", "South_America");
	fZonePopUp -> AddItem(zoneItem);
	
	//label #1
	BStringView *fCurrent;
	
	const char *current= "Current time zone:";
	float length = be_plain_font -> StringWidth(current);
	
	BRect currframe(168.0, 43.0, 168.0+length, 53.0);
	fCurrent = new BStringView(currframe, "current", current);
	
	AddChild(fCurrent);
	
	//label #2
	BStringView *fTimeIn;
	
	const char *timeIn= "Time in:";
	float timeLength;
	
	timeLength = be_plain_font -> StringWidth(timeIn);
	
	BRect timeFrame(168.0, 87.0, 168.0+timeLength, 97.0);
	
	fTimeIn= new BStringView(timeFrame, "time", timeIn);
	
	AddChild(fTimeIn);
	
	//The "Set" button:
	BRect buttonFrame(247.0, 155.0, 324.0, 165.0);
	fSetTimeZone = new BButton(buttonFrame, "set", "Set", new BMessage(SET_TIME_ZONE));
	fSetTimeZone -> SetEnabled(false);
	AddChild(fSetTimeZone);
	
	//current time zone
	getCurrentTZ();
}//ZoneView::buildView()

void
ZoneView::ChangeRegion(BMessage *message)
{
	//First: get the region name from the message:
	const char *area;
	message -> FindString("region",&area);
	
	//clear listview from previous items
	fCityList -> MakeEmpty();
	
	//Enter time zones directory. Find subdir with name that matches string stored in area.
	//Enter subdirectory and count the items. For each item, add a StringItem to fCityList
 	//Time zones directory 
 	BDirectory zoneDir("/boot/beos/etc/timezones/"); 
 	BDirectory cityDir;
 	BStringItem *city;
 	entry_ref ref;
 	
 	//locate subdirectory:
	if ( zoneDir.Contains(area, B_DIRECTORY_NODE) ){
		cityDir.SetTo(&zoneDir, area);  //There is a subdir with a name that matches 'area'. That's the one!!
		//iterate over the items in the subdir and fill the listview with stringitems:	
		while(cityDir.GetNextRef(&ref) == B_NO_ERROR){
			city = new BStringItem(ref.name);
			fCityList -> AddItem(city);
		}
	}

}//ZoneView::ChangeRegion()

void
ZoneView::getCurrentTime()
{
	char tmp[10];
	struct tm locTime;
	time_t current;
	
	current= time(0);
	locTime= *localtime(&current);
	
	strftime(tmp, 10, "%I:%M %p", &locTime);
	
	strcpy(fTimeStr, tmp);
	
	//strftime(tmp, 10, "%z", &locTime);
	
	//strcpy(fZoneStr,tmp);
	
}//ZoneView::getCurrentTime()

void
ZoneView::getCurrentTZ()
{

 	BPath path;
 	BEntry tzLink;
 	char buffer[B_FILE_NAME_LENGTH];
 	
 	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK){
 		path.Append("timezone");
		tzLink.SetTo(path.Path(), true);
 		tzLink.GetName(buffer);
 		strcpy(fZoneStr, buffer);
 	}
 				
 	
}//ZoneView::getCurrentTz()

void
ZoneView::Draw(BRect rect)
{
	
	
	
	BPoint tZone(170.0, 65.0);
	BPoint tTime(280.0, 65.0);
	
	//BPoint bZone(170.0, 107.0);
	//BPoint bTime(280.0, 107.0);
	
	SetHighColor(ViewColor());
	SetLowColor(ViewColor());
	FillRect(Bounds());
	SetHighColor(0,0,0,255);
	
	DrawString(fZoneStr, tZone);
	DrawString(fTimeStr, tTime);
	
	//DrawString(fZoneStr, bZone);
	//DrawString(fTimeStr, bTime);	
	
}//ZoneView:: Draw()

void
ZoneView::Pulse()
{
	getCurrentTime();
	
	Draw(Bounds());
}//ZoneView:: Pulse()

void
ZoneView::newTimeZone()
{
	
	int32 selection;
	BStringItem *item;
	const char *text;
	
	selection = fCityList -> CurrentSelection();
	item = (BStringItem *)fCityList -> ItemAt(selection);
	text = item -> Text();
	strcpy(fZoneStr, text);
	Draw(Bounds());
	fSetTimeZone -> SetEnabled(true);
}//ZoneView::newTimeZone()

void
ZoneView::setTimeZone()
{
	/*
		set time based on supplied timezone. How to do this?
		replace symlink, "timezone", in B_USER_SETTINGS_DIR with a link to the new timezone
	*/
	
	fSetTimeZone -> SetEnabled(false);
} //ZoneView::setTimeZone()




	