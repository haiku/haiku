#include "HEventList.h"
#include "HEventItem.h"

#include "HWindow.h"
#include "IconMenuItem.h"

#include <ClassInfo.h>
#include <MediaFiles.h>
#include <Path.h>
#include <Alert.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <stdio.h>
#include <Mime.h>
#include <Roster.h>
#include <NodeInfo.h>

/***********************************************************
 * Constructor
 ***********************************************************/
HEventList::HEventList(BRect rect
				,CLVContainerView** ContainerView
				,const char* name)
	:ColumnListView(rect
				,ContainerView
				,name
				,B_FOLLOW_ALL
				,B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE
				,B_SINGLE_SELECTION_LIST
				,false
				,false
				,true)
{
	AddColumn(new CLVColumn(NULL,20,CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE|
		CLV_NOT_RESIZABLE|CLV_PUSH_PASS|CLV_MERGE_WITH_RIGHT));
	AddColumn(new CLVColumn("Event",100,CLV_SORT_KEYABLE));
	AddColumn( new CLVColumn("Sound",130,CLV_SORT_KEYABLE));
	AddColumn( new CLVColumn("Path",200,CLV_SORT_KEYABLE));
	SetSortKey(1);
	SetSortFunction(CLVEasyItem::CompareItems);
}

/***********************************************************
 * Destructor
 ***********************************************************/
HEventList::~HEventList()
{
}

/***********************************************************
 * Set type
 ***********************************************************/
void
HEventList::SetType(const char* type)
{
	RemoveAll();
	BMediaFiles mfiles;
	mfiles.RewindRefs(type);
	
	BString name;
	entry_ref ref;
	while(mfiles.GetNextRef(&name,&ref) == B_OK)
	{
		AddItem(new HEventItem(name.String(),BPath(&ref).Path()));
	}
	SortItems();
}

/***********************************************************
 * Remove all items
 ***********************************************************/
void
HEventList::RemoveAll()
{
	int32 count = CountItems();
	while(count> 0)
		delete fPointerList.RemoveItem(--count);
	MakeEmpty();
}

/***********************************************************
 * Add event item
 ***********************************************************/
void
HEventList::AddEvent(HEventItem *item)
{
	fPointerList.AddItem(item);
	AddItem(item);
}

/***********************************************************
 * Remove event item
 ***********************************************************/
void
HEventList::RemoveEvent(HEventItem *item)
{
	item->Remove();
	fPointerList.RemoveItem(item);
	RemoveItem(item);
}

/***********************************************************
 * Selection Changed
 ***********************************************************/
void
HEventList::SelectionChanged()
{
	int32 sel = CurrentSelection();
	if(sel >= 0)
	{
		HEventItem *item = cast_as(ItemAt(sel),HEventItem);
		if(!item)
			return;
		BMessage msg(M_EVENT_CHANGED);
		msg.AddString("name",item->Name());
		
		msg.AddString("path",item->Path());
		Window()->PostMessage(&msg);
	}
}	

/***********************************************************
 * MouseDown
 ***********************************************************/
void
HEventList::MouseDown(BPoint pos)
{
	BPoint point = pos;
	int32 buttons;
	
    Window()->CurrentMessage()->FindInt32("buttons", &buttons); 
    this->MakeFocus(true);
	
    // 右クリックのハンドリング 
    if(buttons == B_SECONDARY_MOUSE_BUTTON)
    {
    	 int32 sel = IndexOf(pos);
    	 if(sel >= 0)
    	 	Select(sel);
    	 else
    	 	DeselectAll();
    	 sel = CurrentSelection();
    	 bool enabled = (sel >= 0)?true:false;
    	 
    	 BPopUpMenu *theMenu = new BPopUpMenu("RIGHT_CLICK",false,false);
    	 BFont font(be_plain_font);
    	 font.SetSize(10);
    	 theMenu->SetFont(&font);
    	/* HWindow *parent = cast_as(Window(),HWindow);
    	 
    	 BMenuField *menufield = cast_as(parent->FindView("filemenu"),BMenuField);
		 BMenu *old_menu = menufield->Menu();
    	 
    	 int32 menus = old_menu->CountItems();
    	 
    	 for(register int32 i = 0;i < menus-3;i++)
    	 {
    	 	BMenuItem *old_item = old_menu->ItemAt(i);
    	 	if(!old_item)
    	 		continue;
    	 	BMessage *old_msg = old_item->Message();
    	 	BMessage *msg = new BMessage(*old_msg );
    	 	BMenuItem *new_item = new BMenuItem(old_item->Label(),msg);
    	 	new_item->SetEnabled(enabled);
    	 	theMenu->AddItem(new_item);	
    	 }
    	 theMenu->AddSeparatorItem();
    	 theMenu->AddItem(new BMenuItem("<none>",new BMessage(M_NONE_MESSAGE)));
		 theMenu->AddItem(new BMenuItem("Other…",new BMessage(M_OTHER_MESSAGE)));
    	 theMenu->FindItem(M_NONE_MESSAGE)->SetEnabled(enabled);
    	 theMenu->FindItem(M_OTHER_MESSAGE)->SetEnabled(enabled);
    	 */
    	 BMenu *submenu = new BMenu("Open With…");
    	 submenu->SetFont(&font);
    	 BMimeType mime("audio");
		
		 if(enabled)
		 {
		 	HEventItem *item = cast_as(this->ItemAt(sel),HEventItem);
		 	const char* path = item->Path();
		 	BFile file(path,B_READ_ONLY);
		 	BNodeInfo ninfo(&file);
		 	char type[B_MIME_TYPE_LENGTH+1];
		 	ninfo.GetType(type);
		 	mime.SetTo(type);
		 }
		 BMessage message;
		 mime.GetSupportingApps(&message);
		/* int32 sub;
		 int32 supers;
    	 message.FindInt32("be:subs", &sub);
    	 message.FindInt32("be:supers", &supers);
    	*/
    	for (int32 index =0; ; index++) 
    	{
			const char *signature;
			int32 length;
		
			if (message.FindData("applications", 'CSTR', index, (const void **)&signature,
				&length) != B_OK)
				break;
			
			entry_ref ref;
			if(be_roster->FindApp(signature,&ref) != B_OK)
				continue;
			BMessage *msg = new BMessage(M_OPEN_WITH);
			msg->AddRef("refs",&ref);
			msg->AddString("sig",signature);
			
			BBitmap *icon = new BBitmap(BRect(0,0,15,15),B_COLOR_8_BIT);
			BNodeInfo::GetTrackerIcon(&ref,icon,B_MINI_ICON);
			
			submenu->AddItem(new IconMenuItem(BPath(&ref).Leaf(),msg,0,0,icon));
		// push each of the supporting apps signature uniquely
		//queryIterator->PushUniqueSignature(signature);
		}
    	 
    	 theMenu->AddItem(submenu);
		 theMenu->AddSeparatorItem();
		 theMenu->AddItem(new BMenuItem("Remove Event",new BMessage(M_REMOVE_EVENT)));
    	 //theMenu->FindItem(M_OPEN_WITH)->SetEnabled(enabled);
    	 theMenu->FindItem(M_REMOVE_EVENT)->SetEnabled(enabled);
    	 submenu->SetEnabled(enabled);
         BRect r;
         ConvertToScreen(&pos);
         r.top = pos.y - 5;
         r.bottom = pos.y + 5;
         r.left = pos.x - 5;
         r.right = pos.x + 5;
         
    	BMenuItem *theItem = theMenu->Go(pos, false,true,r);  
    	if(theItem)
    	{
    	 	BMessage*	aMessage = theItem->Message();
			if(aMessage)
				this->Window()->PostMessage(aMessage);
	 	} 
	 	delete theMenu;
	 }else
	 	ColumnListView::MouseDown(point);
}
