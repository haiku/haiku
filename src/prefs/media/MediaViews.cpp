/*

MediaViews by Sikosis

(C)2003

*/

// Includes -------------------------------------------------------------------------------------------------- //
#include <Box.h>
#include <Button.h>
#include <TextView.h>
#include <MenuField.h>
#include <Menu.h>
#include <MediaRoster.h>
#include <Deskbar.h>
#include <Entry.h>
#include <stdio.h>
#include <MediaAddOn.h>
#include "MediaViews.h"

BarView::BarView(BRect frame) 
 : BView (frame, "barView", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW )
{
}

void 
BarView::Draw(BRect updateRect)
{
	BRect r = Bounds();
	
	// Display the 3D Look Divider Bar
	SetHighColor(140,140,140,0);
	StrokeLine(BPoint(r.left,r.top),BPoint(r.right,r.top));
	SetHighColor(255,255,255,0);
	StrokeLine(BPoint(r.left,r.bottom),BPoint(r.right,r.bottom));
}


SettingsView::SettingsView (BRect frame, bool isVideo)
 : BView (frame, "SettingsView", B_FOLLOW_ALL_SIDES, B_WILL_DRAW ),
 	mIsVideo(isVideo)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	BRect rect(frame);
	rect.left += 11;
	rect.top += 12;
	rect.right -=20;
	rect.bottom = rect.top + 104;
	BBox *defaultsBox = new BBox(rect, "defaults");
	defaultsBox->SetLabel(mIsVideo ? "Default Nodes" : "Defaults");
	AddChild(defaultsBox);
	
	BRect defaultRect(20, 22, 250, 40);
	mMenu1 = new BMenu("menu1");
	mMenu1->SetLabelFromMarked(true);
	BMenuField *menuField1 = new BMenuField(defaultRect, "menuField1", 
		mIsVideo ? "Video Input:" : "Audio Input:", mMenu1);
	defaultsBox->AddChild(menuField1);
	menuField1->SetDivider(75);
	
	defaultRect.OffsetBy(0, 26);
	mMenu2 = new BMenu("menu2");
	mMenu2->SetLabelFromMarked(true);
	BMenuField *menuField2 = new BMenuField(defaultRect, "menuField2", 
		mIsVideo ? "Video Output:" : "Audio Output:", mMenu2);
	defaultsBox->AddChild(menuField2);
	menuField2->SetDivider(75);
	
	if(!mIsVideo) {
		defaultRect.OffsetBy(186, 0);
		BMenu *mMenu3 = new BMenu("menu3");
		mMenu3->SetLabelFromMarked(true);
		BMenuField *menuField3 = new BMenuField(defaultRect, "menuField3", 
			"Channel:", mMenu3);
		defaultsBox->AddChild(menuField3);
		menuField3->SetDivider(50);
		defaultRect.OffsetBy(-186, 0);
	}
	
	defaultRect.OffsetBy(0, 32);
	BRect restartTextRect(defaultRect);
	restartTextRect.OffsetTo(B_ORIGIN);
	restartTextRect.InsetBy(1,1);
	rgb_color red_color = {222, 32, 33};
	mRestartTextView = new BTextView(defaultRect, "restartTextView", restartTextRect, 
		be_plain_font, &red_color, B_FOLLOW_ALL, B_WILL_DRAW);
	mRestartTextView->Insert("Restart the Media Server to apply changes.");
	mRestartTextView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	defaultsBox->AddChild(mRestartTextView);
	mRestartTextView->Hide();
	
	rect.top = rect.bottom + 10;
	rect.bottom = rect.top + 162;
	BBox *realtimeBox = new BBox(rect, "realtime");
	realtimeBox->SetLabel("Real-Time");
	AddChild(realtimeBox);
	
	BMessage *message = new BMessage(ML_ENABLE_REAL_TIME);
	message->AddBool("isVideo", mIsVideo);
	BRect rect2(22,20, 190, 40);
	mRealtimeCheckBox = new BCheckBox(rect2, "realtimeCheckBox", 
		mIsVideo ? "Enable Real-Time Video" : "Enable Real-Time Audio", message);
	realtimeBox->AddChild(mRealtimeCheckBox);
	
	uint32 flags;
	BMediaRoster::Roster()->GetRealtimeFlags(&flags);
	if(flags & (mIsVideo ? B_MEDIA_REALTIME_VIDEO : B_MEDIA_REALTIME_AUDIO))
		mRealtimeCheckBox->SetValue(B_CONTROL_ON);
		
	rect2.top += 26;
	rect2.bottom = rect2.top + 80;
	rect2.right = rect.right - 15;
	BRect textRect(3, 3, rect2.Width() - 3, rect2.Height() - 3);
	BTextView *textView = new BTextView(rect2, "textView", textRect, B_FOLLOW_ALL, B_WILL_DRAW);
	textView->Insert(mIsVideo ? "Enabling Real-Time Video allows the BeOS to perform video operations as fast and smoothly as possible.  It achieves optimum performance by using more RAM."
		: "Enabling Real-time Audio allows BeOS to record and play audio as fast as possible.  It achieves this performance by using more CPU and RAM.");
	textView->Insert("\n\nOnly enable this feature if you need the lowest latency possible.");
	textView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	realtimeBox->AddChild(textView);
	
	rect.top = rect.bottom + 11;
	rect.bottom = rect.top + 20;
	rect.left = rect.right - 130;
	BButton *restartButton = new BButton(rect, "restartButton", 
		"Restart Media Services", new BMessage(ML_RESTART_MEDIA_SERVER));
	AddChild(restartButton);
	
	if(!mIsVideo) {
		rect.top += 4;
		rect.left = frame.left + 33;
		rect.right = rect.left + 180;
		
		mVolumeCheckBox = new BCheckBox(rect, "volumeCheckBox", 
			"Show Volume Control on Deskbar", new BMessage(ML_SHOW_VOLUME_CONTROL));
		AddChild(mVolumeCheckBox);
		
		if(BDeskbar().HasItem("MediaReplicant"))
			mVolumeCheckBox->SetValue(B_CONTROL_ON);	
	}
}

void
SettingsView::AddNodes(BList &list, bool isInput)
{
	BMenu *menu = isInput ? mMenu1 : mMenu2;
	BMenuItem *item;
	while(item = menu->RemoveItem((int32)0))
		delete item;
		
	BMessage message(ML_DEFAULT_CHANGE);
	message.AddBool("isVideo", mIsVideo);
	message.AddBool("isInput", isInput);
		
	for(int32 i=0; i<list.CountItems(); i++) {
		dormant_node_info *info = static_cast<dormant_node_info *>(list.ItemAt(i));
		menu->AddItem(new SettingsItem(info, new BMessage(message)));
	}
}

void
SettingsView::SetDefault(dormant_node_info &info, bool isInput)
{
	BMenu *menu = isInput ? mMenu1 : mMenu2;
		
	for(int32 i=0; i<menu->CountItems(); i++) {
		SettingsItem *item = static_cast<SettingsItem *>(menu->ItemAt(i));
		if(item->mInfo && item->mInfo->addon == info.addon && item->mInfo->flavor_id == info.flavor_id) {
			item->SetMarked(true);
			break;
		}
	}
}

SettingsItem::SettingsItem(dormant_node_info *info, BMessage *message, 
			char shortcut, uint32 modifiers)
	: BMenuItem(info->name, message, shortcut, modifiers),
	mInfo(info)
{
	
}
		