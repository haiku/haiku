/*
	APRView.cpp:	The Appearance app's bulky boy
	
	Handles all the grunt work. Color settings are stored in a BMessage to
	allow for easy transition to and from disk. The messy details are in the
	hard-coded strings to translate the enumerated color_which defines to
	strings and such, but even those should be easy to maintain.
*/
#include <OS.h>
#include "APRView.h"
#include "PortLink.h"
#include <Directory.h>
#include <File.h>
#include <stdio.h>

#define SETTINGS_DIR "/boot/home/config/settings/app_server/"

#define APPLY_SETTINGS 'aply'
#define REVERT_SETTINGS 'rvrt'
#define DEFAULT_SETTINGS 'dflt'
#define TRY_SETTINGS 'trys'

#define ATTRIBUTE_CHOSEN 'atch'
#define UPDATE_COLOR 'upcl'

void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255);
void PrintRGBColor(rgb_color col);

APRView::APRView(BRect frame, const char *name, int32 resize, int32 flags)
	:BView(frame,name,resize,flags)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// Set up list of color attributes
	attrlist=new BListView(BRect(10,10,110,110),"AttributeList");
	
	scrollview=new BScrollView("ScrollView",attrlist, B_FOLLOW_LEFT |
		B_FOLLOW_TOP, 0, false, true);
	AddChild(scrollview);
	scrollview->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	attrlist->SetSelectionMessage(new BMessage(ATTRIBUTE_CHOSEN));

	attrlist->AddItem(new BStringItem("Window Background"));
	attrlist->AddItem(new BStringItem("Menu Background"));
	attrlist->AddItem(new BStringItem("Selected Menu Background"));
	attrlist->AddItem(new BStringItem("Menu Text"));
	attrlist->AddItem(new BStringItem("Selected Menu Text"));
	attrlist->AddItem(new BStringItem("Window Tab"));
	attrlist->AddItem(new BStringItem("Keyboard Navigation"));
	attrlist->AddItem(new BStringItem("Desktop Color"));
	
	picker=new BColorControl(BPoint(scrollview->Frame().right+20,scrollview->Frame().top),B_CELLS_32x8,5.0,"Picker",
		new BMessage(UPDATE_COLOR));
	AddChild(picker);
	
	BRect cvrect=picker->Frame();
	cvrect.OffsetBy(picker->Bounds().Width()+10,0);
	cvrect.right=Bounds().right-10;
	
	colorview=new BView(cvrect,"ColorView",B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	colorview->SetViewColor(picker->ValueAsColor());
	AddChild(colorview);

	cvrect.Set(0,0,50,25);
	cvrect.OffsetBy(picker->Frame().left,
		picker->Frame().bottom+20);

	defaults=new BButton(cvrect,"DefaultsButton","Defaults",
		new BMessage(DEFAULT_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(defaults);

	cvrect.OffsetBy(70,0);
	revert=new BButton(cvrect,"RevertButton","Revert",
		new BMessage(REVERT_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(revert);
	// Eventually we might want to have this disabled at the start and
	// enabled when the user makes changes and such. Then again, maybe we won't.
//	revert->SetEnabled(false);
	
	cvrect.OffsetBy(70,0);
	try_settings=new BButton(cvrect,"TryButton","Try",
		new BMessage(TRY_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(try_settings);
	
	// Eventually we might want to have this disabled at the start and
	// enabled when the user makes changes and such. Then again, maybe we won't.
//	try_settings->SetEnabled(false);
	
	cvrect.OffsetBy(70,0);
	apply=new BButton(cvrect,"ApplyButton","Apply",
		new BMessage(APPLY_SETTINGS),B_FOLLOW_LEFT |B_FOLLOW_TOP,
		B_WILL_DRAW | B_NAVIGABLE);
	AddChild(apply);
	
	attribute=B_PANEL_BACKGROUND_COLOR;
	attrstring="PANEL_BACKGROUND";
	LoadSettings();
	picker->SetValue(GetColorFromMessage(&settings,attrstring.String()));
	
}

APRView::~APRView(void)
{
}

void APRView::AttachedToWindow(void)
{
	picker->SetTarget(this);
	attrlist->SetTarget(this);
	apply->SetTarget(this);
	defaults->SetTarget(this);
	try_settings->SetTarget(this);
	revert->SetTarget(this);
	attrlist->Select(0);
}

void APRView::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case UPDATE_COLOR:
		{
			rgb_color col=picker->ValueAsColor();

			colorview->SetViewColor(col);
			colorview->Invalidate();
			
			// Update current attribute here
			settings.ReplaceData(attrstring.String(),(type_code)'RGBC',
			&col,sizeof(rgb_color));
			break;
		}
		case ATTRIBUTE_CHOSEN:
		{
			attribute=SelectionToAttribute(attrlist->CurrentSelection());
			attrstring=SelectionToString(attrlist->CurrentSelection());
			
			rgb_color col=GetColorFromMessage(&settings,attrstring.String(),0);
			picker->SetValue(col);
			colorview->SetViewColor(col);
			colorview->Invalidate();
			break;
		}
		case APPLY_SETTINGS:
		{
			SaveSettings();
			NotifyServer();
			break;
		}
		case TRY_SETTINGS:
		{
			// Tell server to apply settings here without saving them.
			// Theoretically, the user can set this temporarily and keep it
			// until the next boot.
			NotifyServer();
			break;
		}
		case REVERT_SETTINGS:
		{
			LoadSettings();
			rgb_color col=GetColorFromMessage(&settings,attrstring.String(),0);
			picker->SetValue(col);
			colorview->SetViewColor(col);
			colorview->Invalidate();
			break;
		}
		case DEFAULT_SETTINGS:
		{
			SetDefaults();
			rgb_color col=GetColorFromMessage(&settings,attrstring.String(),0);
			picker->SetValue(col);
			colorview->SetViewColor(col);
			colorview->Invalidate();
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

color_which APRView::SelectionToAttribute(int32 index)
{
	switch(index)
	{	
		case 0:
			return B_PANEL_BACKGROUND_COLOR;
			break;	// not necessary, but do it anyway. ;)
		case 1:
			return B_MENU_BACKGROUND_COLOR;
			break;	// not necessary, but do it anyway. ;)
		case 2:
			return B_MENU_SELECTION_BACKGROUND_COLOR;
			break;	// not necessary, but do it anyway. ;)
		case 3:
			return B_MENU_ITEM_TEXT_COLOR;
			break;	// not necessary, but do it anyway. ;)
		case 4:
			return B_MENU_SELECTED_ITEM_TEXT_COLOR;
			break;	// not necessary, but do it anyway. ;)
		case 5:
			return B_WINDOW_TAB_COLOR;
			break;	// not necessary, but do it anyway. ;)
		case 6:
			return B_KEYBOARD_NAVIGATION_COLOR;
			break;	// not necessary, but do it anyway. ;)
		default:
			return B_DESKTOP_COLOR;
			break;	// not necessary, but do it anyway. ;)
	}
	
}

void APRView::SaveSettings(void)
{
	BString path(SETTINGS_DIR);
	path+="SystemColors";
	printf("%s\n",path.String());
	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	
	settings.Flatten(&file);
}

void APRView::LoadSettings(void)
{
	settings.MakeEmpty();

	BDirectory dir,newdir;
	if(dir.SetTo(SETTINGS_DIR)==B_ENTRY_NOT_FOUND)
		create_directory(SETTINGS_DIR,0777);

	BString path(SETTINGS_DIR);
	path+="SystemColors";
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()==B_OK)
	{
		if(settings.Unflatten(&file)==B_OK)
		{
			// Get the color for the current attribute here
			return;
		}
		printf("Error unflattening settings file %s\n",path.String());
	}
	
	// If we get this far, we have encountered an error, so reset the settings
	// to the defaults
	SetDefaults();
}

void APRView::SetDefaults(void)
{
	settings.MakeEmpty();
	rgb_color col={216,216,216,255};
	settings.AddData("PANEL_BACKGROUND",(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,219,219,219,0);
	settings.AddData("MENU_BACKGROUND",(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,115,120,184);
	settings.AddData("MENU_SELECTION_BACKGROUND",(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,0);
	settings.AddData("MENU_ITEM_TEXT",(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,255,255);
	settings.AddData("MENU_SELECTED_ITEM_TEXT",(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,255,203,0);
	settings.AddData("WINDOW_TAB",(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,0,0,229);
	settings.AddData("KEYBOARD_NAVIGATION",(type_code)'RGBC',&col,sizeof(rgb_color));

	SetRGBColor(&col,51,102,160);
	settings.AddData("DESKTOP",(type_code)'RGBC',&col,sizeof(rgb_color));
}

void APRView::NotifyServer(void)
{
	// Send a message to the app_server with the settings which we have.
	// While it might be worthwhile to someday have a separate protocol for
	// updating just one color, for now, we just need something to get things
	// done. Extract all the colors from the settings BMessage, attach them
	// to a PortLink message and fire it off to the server.
	
	// Name taken from ServerProtocol.h
	port_id serverport=find_port("OBappserver");

	if(serverport==B_NAME_NOT_FOUND)
		return;
	
	PortLink *pl=new PortLink(serverport);

	// 'suic'==SET_UI_COLORS message from ServerProtocol.h
	pl->SetOpCode('suic');
	
	rgb_color col;
	col=GetColorFromMessage(&settings,"PANEL_BACKGROUND");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"MENU_BACKGROUND");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"MENU_SELECTION_BACKGROUND");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"MENU_ITEM_TEXT");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"MENU_SELECTED_ITEM_TEXT");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"WINDOW_TAB");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"KEYBOARD_NAVIGATION");
	pl->Attach(&col,sizeof(rgb_color));
	col=GetColorFromMessage(&settings,"DESKTOP_COLOR");
	pl->Attach(&col,sizeof(rgb_color));

	pl->Flush();
	delete pl;
}

rgb_color APRView::GetColorFromMessage(BMessage *msg, const char *name, int32 index=0)
{
	// Simple function to do the dirty work of getting an rgb_color from
	// a message
	rgb_color *col,rcolor={0,0,0,0};
	uint8 *ptr;
	ssize_t size;

	if(msg->FindData(name,(type_code)'RGBC',index,(const void**)&ptr,&size)==B_OK)
	{
		col=(rgb_color*)ptr;
		rcolor=*col;
	}
	return rcolor;
}

const char *APRView::AttributeToString(color_which attr)
{
	switch(attr)
	{
		case B_PANEL_BACKGROUND_COLOR:
			return "PANEL_BACKGROUND";
			break;
		case B_MENU_BACKGROUND_COLOR:
			return "MENU_BACKGROUND";
			break;
		case B_MENU_SELECTION_BACKGROUND_COLOR:
			return "MENU_SELECTION_BACKGROUND";
			break;
		case B_MENU_ITEM_TEXT_COLOR:
			return "MENU_ITEM_TEXT";
			break;
		case B_MENU_SELECTED_ITEM_TEXT_COLOR:
			return "MENU_SELECTED_ITEM_TEXT";
			break;
		case B_WINDOW_TAB_COLOR:
			return "WINDOW_TAB";
			break;
		case B_KEYBOARD_NAVIGATION_COLOR:
			return "KEYBOARD_NAVIGATION";
			break;
		default:
			return "DESKTOP_COLOR";
			break;
	}
}

const char *APRView::SelectionToString(int32 index)
{
	switch(index)
	{
		case 0:
			return "PANEL_BACKGROUND";
			break;
		case 1:
			return "MENU_BACKGROUND";
			break;
		case 2:
			return "MENU_SELECTION_BACKGROUND";
			break;
		case 3:
			return "MENU_ITEM_TEXT";
			break;
		case 4:
			return "MENU_SELECTED_ITEM_TEXT";
			break;
		case 5:
			return "WINDOW_TAB";
			break;
		case 6:
			return "KEYBOARD_NAVIGATION";
			break;
		default:
			return "DESKTOP_COLOR";
			break;
	}
}

void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255)
{
	col->red=r;
	col->green=g;
	col->blue=b;
	col->alpha=a;
}

void PrintRGBColor(rgb_color col)
{
	printf("RGB Color (%d,%d,%d,%d)\n",col.red,col.green,col.blue,col.alpha);
}

