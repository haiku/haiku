/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>		
 */

#include "MenuView.h"
#include <Roster.h>
#include <Message.h>
#include <stdio.h>
#include <malloc.h>

#define TOGGLE_TRIGGERS 'tgtr'
#define ALT_COMMAND 'acmd'
#define CTRL_COMMAND 'ccmd'
#define SET_FONT_SIZE 'sfsz'
#define SET_DEFAULTS 'sdef'
#define REVERT_SETTINGS 'rvst'


MenuView::MenuView(BRect frame, const char *name, int32 resize, int32 flags)
	: BView(frame, name, resize, flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	get_menu_info(&menuinfo);
	revertinfo = menuinfo;
	revert = false;
	
	fontmenu = new FontMenu;
	fontmenu->SetLabelFromMarked(true);
	fontmenu->GetFonts();
	fontmenu->Update();

	fontsizemenu = new BMenu("");
	fontsizemenu->SetLabelFromMarked(true);

	BMenuItem *item;
	BMessage *msg;

	msg = new BMessage(SET_FONT_SIZE);
	msg->AddInt32("size", 10);
	item = new BMenuItem("10", msg);
	fontsizemenu->AddItem(item);

	msg = new BMessage(SET_FONT_SIZE);
	msg->AddInt32("size", 11);
	item = new BMenuItem("11", msg);
	fontsizemenu->AddItem(item);

	msg = new BMessage(SET_FONT_SIZE);
	msg->AddInt32("size", 12);
	item = new BMenuItem("12", msg);
	fontsizemenu->AddItem(item);

	msg = new BMessage(SET_FONT_SIZE);
	msg->AddInt32("size", 14);
	item = new BMenuItem("14", msg);
	fontsizemenu->AddItem(item);

	msg = new BMessage(SET_FONT_SIZE);
	msg->AddInt32("size", 18);
	item = new BMenuItem("18", msg);
	fontsizemenu->AddItem(item);
	
	msg = new BMessage(SET_FONT_SIZE);
	msg->AddInt32("size", 20);
	item = new BMenuItem("20", msg);
	fontsizemenu->AddItem(item);
	
	msg = new BMessage(SET_FONT_SIZE);
	msg->AddInt32("size", 22);
	item = new BMenuItem("22", msg);
	fontsizemenu->AddItem(item);
	
	msg = new BMessage(SET_FONT_SIZE);
	msg->AddInt32("size", 24);
	item = new BMenuItem("24", msg);
	fontsizemenu->AddItem(item);
	
	switch((int32)menuinfo.font_size)
	{
		case 10:
			item = fontsizemenu->FindItem("10");
			break;
		
		case 11:
			item = fontsizemenu->FindItem("11");
			break;
		
		case 12:
			item = fontsizemenu->FindItem("12");
			break;
		
		case 14:
			item = fontsizemenu->FindItem("14");
			break;
		
		case 18:
			item = fontsizemenu->FindItem("18");
			break;
		
		case 20:
			item = fontsizemenu->FindItem("20");
			break;
		
		case 22:
			item = fontsizemenu->FindItem("22");
			break;
			
		case 24:
			item = fontsizemenu->FindItem("24");
			break;
			
		default:
		{
			char fontsizestr[10];
			sprintf(fontsizestr, "%ld", (int32)menuinfo.font_size);
			msg = new BMessage(SET_FONT_SIZE);
			msg->AddInt32("size", (int32)menuinfo.font_size);
			item = new BMenuItem(fontsizestr,msg);
			fontsizemenu->AddItem(item);
			break;
		}
	}
	item->SetMarked(true);	
	
	BRect r(10, 10, 210, 40);
	r.OffsetTo((Bounds().Width() - r.Width()) / 2, 10);
	fontfield=new BMenuField(r, "fontfield", "", fontmenu);
	fontfield->SetLabel("Font:");
	fontfield->SetDivider(StringWidth("Font:") + 5);
	AddChild(fontfield);
	
	r.OffsetTo((Bounds().Width() - r.Width()) / 2, fontfield->Frame().bottom + 10);
	fontsizefield=new BMenuField(r, "fontsizefield", "", fontsizemenu);
	fontsizefield->SetLabel("Font Size:");
	fontsizefield->SetDivider(StringWidth("Font Size:") + 5);
	AddChild(fontsizefield);
	
	r.OffsetTo(10, fontsizefield->Frame().bottom + 10);
	r.right += 20;
	showtriggers = new BCheckBox(r, "showtriggers", "Always Show Triggers", 
		new BMessage(TOGGLE_TRIGGERS));
	showtriggers->ResizeToPreferred();
	showtriggers->MoveTo((Bounds().Width() - showtriggers->Bounds().Width()) / 2, r.top);
	AddChild(showtriggers);
	if (menuinfo.triggers_always_shown)
		showtriggers->SetValue(B_CONTROL_ON);
	
	r.bottom = r.top + 75;
	r.OffsetTo((Bounds().Width() - r.Width()) / 2, showtriggers->Frame().bottom + 10);
	BBox *radiobox = new BBox(r, "radiobox");
	radiobox->SetLabel("Shortcut Key");
	AddChild(radiobox);
	
	r.Set(10, 20, 50, 30);
	altcmd = new BRadioButton(r, "altcommand", "Alt", new BMessage(ALT_COMMAND));
	altcmd->ResizeToPreferred();
	radiobox->AddChild(altcmd);

	r.OffsetBy(0, 25);
	ctrlcmd = new BRadioButton(r, "ctrlcommand", "Control", new BMessage(CTRL_COMMAND));
	ctrlcmd->ResizeToPreferred();
	radiobox->AddChild(ctrlcmd);

	MarkCommandKey();
	revertaltcmd = altcommand;
	
	defaultsbutton = new BButton(r, "defaultsbutton", "Defaults", new BMessage(SET_DEFAULTS));
	defaultsbutton->ResizeToPreferred();

	revertbutton = new BButton(r, "revertbutton", "Revert", new BMessage(REVERT_SETTINGS));
	revertbutton->ResizeToPreferred();	
	
	defaultsbutton->MoveTo( (Bounds().Width() -
			(defaultsbutton->Bounds().Width() + revertbutton->Bounds().Width() + 10)) / 2,
			radiobox->Frame().bottom + 20);
	revertbutton->MoveTo(defaultsbutton->Frame().right + 10, radiobox->Frame().bottom + 20);
	
	AddChild(defaultsbutton);
	AddChild(revertbutton);
	revertbutton->SetEnabled(false);	
}


MenuView::~MenuView(void)
{
}


void
MenuView::AttachedToWindow(void)
{
	for(int32 i = 0; i < fontmenu->CountItems(); i++) {
		BMenu *menu = fontmenu->SubmenuAt(i);
		if (menu)
			menu->SetTargetForItems(this);
	}
	fontmenu->SetTargetForItems(this);
	fontsizemenu->SetTargetForItems(this);
	showtriggers->SetTarget(this);
	altcmd->SetTarget(this);
	ctrlcmd->SetTarget(this);
	defaultsbutton->SetTarget(this);
	revertbutton->SetTarget(this);
}


void
MenuView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case B_MODIFIERS_CHANGED:
			MarkCommandKey();
			break;
			
		case MENU_FONT_FAMILY:
		case MENU_FONT_STYLE:
		{
			if (!revert)
				revertbutton->SetEnabled(true);
			revert = true;
			font_family *family;
			msg->FindPointer("family", (void**)&family);
			font_style *style;
			msg->FindPointer("style", (void**)&style);
			menuinfo.f_family = *family;
			menuinfo.f_style = *style;
			set_menu_info(&menuinfo);
			
			fontmenu->Update();
			break;
		}
		case SET_FONT_SIZE:
		{
			if (!revert)
				revertbutton->SetEnabled(true);
			revert=true;
			int32 size;
			msg->FindInt32("size", &size);
			menuinfo.font_size = size;
			set_menu_info(&menuinfo);
			fontmenu->Update();
			break;
		}	
		case ALT_COMMAND:
			if (!revert)
				revertbutton->SetEnabled(true);
			revert = true;
			altcommand = true;
			SetCommandKey(altcommand);
			break;
			
		case CTRL_COMMAND:
			if (!revert)
				revertbutton->SetEnabled(true);
			revert = true;
			altcommand = false;
			SetCommandKey(altcommand);
			break;
			
		case TOGGLE_TRIGGERS:
			if (!revert)
				revertbutton->SetEnabled(true);
			revert = true;
			menuinfo.triggers_always_shown = (showtriggers->Value() == B_CONTROL_ON);
			set_menu_info(&menuinfo);
			break;
			
		case REVERT_SETTINGS:
			revert = false;
			revertbutton->SetEnabled(false);
			SetCommandKey(revertaltcmd);
			altcommand = revertaltcmd;
			MarkCommandKey();
			menuinfo = revertinfo;
			fontmenu->Update();
			showtriggers->SetValue(menuinfo.triggers_always_shown ? B_CONTROL_ON : B_CONTROL_OFF);
			set_menu_info(&revertinfo);
			break;
	
		case SET_DEFAULTS:
			if (!revert)
				revertbutton->SetEnabled(true);
			revert = true;
			
			menuinfo.triggers_always_shown = false;
			menuinfo.font_size = 12.0;
			sprintf(menuinfo.f_family, "%s", "Swis721 BT");
			sprintf(menuinfo.f_style, "%s", "Roman");
			set_menu_info(&menuinfo);

			fontmenu->Update();
			altcommand = true;
			SetCommandKey(altcommand);
			MarkCommandKey();
			showtriggers->SetValue(menuinfo.triggers_always_shown ? B_CONTROL_ON : B_CONTROL_OFF);
			break;
	
		default:
			BView::MessageReceived(msg);
			break;
	}
}


void
MenuView::MarkCommandKey(void)
{
	key_map *keys; 
	char *chars; 
	
	get_key_map(&keys, &chars); 
	
	bool altAsShortcut = (keys->left_command_key == 0x5d)
		&& (keys->left_control_key == 0x5c); 
		
	free(chars); 
	free(keys);
	
	if (altAsShortcut)
		altcmd->SetValue(B_CONTROL_ON);
	else
		ctrlcmd->SetValue(B_CONTROL_ON);
}


void
MenuView::SetCommandKey(bool useAlt)
{
	if (useAlt)	{
		set_modifier_key(B_LEFT_COMMAND_KEY, 0x5d);
		set_modifier_key(B_LEFT_CONTROL_KEY, 0x5c);
	} else {
		set_modifier_key(B_LEFT_COMMAND_KEY, 0x5c);
		set_modifier_key(B_LEFT_CONTROL_KEY, 0x5d);
	}
	be_roster->Broadcast(new BMessage(B_MODIFIERS_CHANGED));
}
