/*
	APRView.cpp:	The Appearance app's color handler
	
	Handles all the grunt work. Color settings are stored in a BMessage to
	allow for easy transition to and from disk. The messy details are in the
	hard-coded strings to translate the enumerated color_which defines to
	strings and such, but even those should be easy to maintain.
*/
#include <OS.h>
#include <Directory.h>
#include <Alert.h>
#include <Path.h>
#include <Entry.h>
#include <File.h>
#include <stdio.h>

#include "APRView.h"
#include "PortLink.h"
#include "defs.h"
#include "ColorWell.h"

//#define DEBUG_COLORSET

// uncomment this line to query the server for the current GUI settings
#define LOAD_SETTINGS_FROM_DISK

#define SAVE_COLORSET 'svcs'
#define DELETE_COLORSET 'dlcs'
#define LOAD_COLORSET 'ldcs'
#define COLOR_DROPPED 'cldp'

void SetRGBColor(rgb_color *col,uint8 r, uint8 g, uint8 b, uint8 a=255);
void PrintRGBColor(rgb_color col);

APRView::APRView(const BRect &frame, const char *name, int32 resize, int32 flags)
	:BView(frame,name,resize,flags), settings(B_SIMPLE_DATA)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect rect(10,60,110,160);

	BMenuBar *mb=new BMenuBar(BRect(0,0,Bounds().Width(),16),"menubar");

	settings_menu=new BMenu("Settings");
	settings_menu->AddItem(new BMenuItem("Save Color Set",new BMessage(SAVE_COLORSET),'S'));
	settings_menu->AddSeparatorItem();
	settings_menu->AddItem(new BMenuItem("Delete Color Set",new BMessage(DELETE_COLORSET)));
	mb->AddItem(settings_menu);

	colorset_menu=LoadColorSets();
	if(colorset_menu)
		mb->AddItem(colorset_menu);
	else
	{
		// We should *never* be here, but just in case....
		colorset_menu=new BMenu("Color Sets");
		mb->AddItem(colorset_menu);
	}
	AddChild(mb);

	BRect wellrect(0,0,20,20);
	wellrect.OffsetTo(Bounds().Width()-(wellrect.Width()+10),25);

	colorwell=new ColorWell(wellrect,new BMessage(COLOR_DROPPED));
	AddChild(colorwell);

	wellrect.OffsetTo(10,25);
	wellrect.right=colorwell->Frame().left - 20;
	colorset_label=new BStringView(wellrect,"colorset_label","Color Set: ");
	AddChild(colorset_label);
	colorset_name=new BString("<untitled>");


	// Set up list of color attributes
	attrlist=new BListView(rect,"AttributeList");
	
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
	
	BRect cvrect;
	
	cvrect.Set(0,0,50,25);
	cvrect.OffsetBy(scrollview->Frame().right+20,
		scrollview->Frame().bottom-cvrect.Height());

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

	BEntry entry(COLOR_SET_DIR);
	entry_ref ref;
	entry.GetRef(&ref);
	savepanel=new BFilePanel(B_SAVE_PANEL, NULL,
		&ref, 0, false);

	attribute=B_PANEL_BACKGROUND_COLOR;
	attrstring="PANEL_BACKGROUND";
	LoadSettings();
	picker->SetValue(GetColorFromMessage(&settings,attrstring.String()));
}

APRView::~APRView(void)
{
	delete savepanel;
	delete colorset_name;
}

void APRView::AttachedToWindow(void)
{
	picker->SetTarget(this);
	attrlist->Select(0);
	attrlist->SetTarget(this);
	apply->SetTarget(this);
	defaults->SetTarget(this);
	try_settings->SetTarget(this);
	revert->SetTarget(this);
	settings_menu->SetTargetForItems(this);
	colorset_menu->SetTargetForItems(this);
	colorwell->SetTarget(this);

	BMessenger msgr(this);
	savepanel->SetTarget(msgr);
}

void APRView::MessageReceived(BMessage *msg)
{
	if(msg->WasDropped())
	{
		rgb_color *col;
		ssize_t size;
		if(msg->FindData("RGBColor",(type_code)'RGBC',(const void**)&col,&size)==B_OK)
		{
			picker->SetValue(*col);
			colorwell->SetColor(*col);
		}
	}

	switch(msg->what)
	{
		case DELETE_COLORSET:
		{
			// Construct the path and delete
			BString path(COLOR_SET_DIR);
			path+=*colorset_name;
			
			BString printstring("Remove ");
			printstring+=*colorset_name;
			printstring+=" from disk permenantly?";
			BAlert *a=new BAlert("OpenBeOS",printstring.String(),"Yes", "No");
			if(a->Go()==0)
			{
				int stat=remove(path.String());
				if(stat!=0)
				{
#ifdef DEBUG_COLORSET
printf("MSG: Delete Request - couldn't delete file %s\n",path.String());
#endif
				}
				else
				{
					BMenuItem *item=colorset_menu->FindItem(colorset_name->String());
					if(item!=NULL)
					{
						if(colorset_menu->RemoveItem(item))
							delete item;
					}
				}
			}
			break;
		}
		case LOAD_COLORSET:
		{
			BString name;
			if(msg->FindString("name",&name)!=B_OK)
			{
#ifdef DEBUG_COLORSET
printf("MSG: Load Request - couldn't find file name\n");
#endif
				break;
			}
			LoadColorSet(name);
			break;
		}
		case SAVE_COLORSET:
		{
			savepanel->Show();
			break;
		}
		case B_SAVE_REQUESTED:
		{
			BString name;
			if(msg->FindString("name",&name)!=B_OK)
			{
#ifdef DEBUG_COLORSET
printf("MSG: Save Request - couldn't find file name\n");
#endif
				break;
			}
			SaveColorSet(name);
			break;
		}
		case UPDATE_COLOR:
		{
			// Received from the color picker when its color changes

			rgb_color col=picker->ValueAsColor();

			colorwell->SetColor(col);
			colorwell->Invalidate();
			
			// Update current attribute in the settings
			settings.ReplaceData(attrstring.String(),(type_code)'RGBC',
			&col,sizeof(rgb_color));

			SetColorSetName("<untitled>");
			break;
		}
		case ATTRIBUTE_CHOSEN:
		{
			// Received when the user chooses a GUI attribute from the list
			attribute=SelectionToAttribute(attrlist->CurrentSelection());
			attrstring=SelectionToString(attrlist->CurrentSelection());
			
			rgb_color col=GetColorFromMessage(&settings,attrstring.String(),0);
			picker->SetValue(col);
			colorwell->SetColor(col);
			colorwell->Invalidate();

			SetColorSetName("<untitled>");
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
			colorwell->SetColor(col);
			colorwell->Invalidate();
			break;
		}
		case DEFAULT_SETTINGS:
		{
			SetDefaults();
			rgb_color col=GetColorFromMessage(&settings,attrstring.String(),0);
			picker->SetValue(col);
			colorwell->SetColor(col);
			colorwell->Invalidate();
			break;
		}
		default:
			BView::MessageReceived(msg);
			break;
	}
}

BMenu *APRView::LoadColorSets(void)
{
#ifdef DEBUG_COLORSET
printf("Loading color sets from disk\n");
#endif
	// This function populates the member menu *colorset_menu with the color
	// set files located in the color set directory. To ensure that there are
	// no entries pointing to invalid color sets, they are validated before
	// a menu item is added.
	BDirectory dir;
	BEntry entry;
	BPath path;
	BString name;

	BMenu *menu=new BMenu("Color Sets");

	status_t dirstat=dir.SetTo(COLOR_SET_DIR);
	if(dirstat!=B_OK)
	{
		// We couldn't set the directory, so more than likely it just
		// doesn't exist. Create it and return an empty menu.
		switch(dirstat)
		{
			case B_NAME_TOO_LONG:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for color sets. "
					"You will be able to change system colors, but be unable to save them to a color set. "
					"Please contact OpenBeOS about Appearance Preferences::APRView::"
					"LoadColorSets::B_NAME_TOO_LONG for a bugfix", "OK",
					 NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				a->Go();
				break;
			}
			case B_ENTRY_NOT_FOUND:
			{
				create_directory(COLOR_SET_DIR,0777);
				break;
			}
			case B_BAD_VALUE:
			{
				printf("APRView::LoadColorSets(): Invalid colorset folder path.\n");
				break;
			}
			case B_NO_MEMORY:
			{
				printf("APRView::LoadColorSets(): No memory left. We're probably going to crash now. \n");
				break;
			}
			case B_BUSY:
			{
				printf("APRView::LoadColorSets(): Busy node " COLOR_SET_DIR "\n");
				break;
			}
			case B_FILE_ERROR:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for color sets "
					"because of a file error. Perhaps there is a file (instead of a folder) at " COLOR_SET_DIR
					"? You will be able to change system colors, but be unable to save them to a color set. ",
					"OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				a->Go();
				break;
			}
			case B_NO_MORE_FDS:
			{
				BAlert *a=new BAlert("OpenBeOS","Couldn't open the folder for color sets "
					"because there are too many open files. Please close some files and restart "
					" this application.", "OK",
					 NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				a->Go();
				if(Window())
					Window()->PostMessage(B_QUIT_REQUESTED);
				break;
			}
		}
		
		return menu;
	}

	int32 count=dir.CountEntries();

	BMessage *msg;
	for(int32 i=0;i<count;i++)
	{
		dir.GetNextEntry(&entry);
		entry.GetPath(&path);
		
		name=path.Path();
		name.Remove(0,name.FindLast('/')+1);

		msg=new BMessage(LOAD_COLORSET);
		msg->AddString("name",name);
		menu->AddItem(new BMenuItem(name.String(),msg));
	}
	
	return menu;
}	

void APRView::LoadColorSet(const BString &name)
{
	// Load the current GUI color settings from a color set file.

#ifdef DEBUG_COLORSET
printf("LoadColorSet: %s\n",name.String());
#endif

	BDirectory dir,newdir;
	if(dir.SetTo(COLOR_SET_DIR)==B_ENTRY_NOT_FOUND)
	{
#ifdef DEBUG_COLORSET
printf("Color set folder not found. Creating %s\n",COLOR_SET_DIR);
#endif
		create_directory(COLOR_SET_DIR,0777);
	}

	BString path(COLOR_SET_DIR);
	path+=name.String();
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("Couldn't open file %s for read\n",path.String());
#endif
		return;
	}
	if(settings.Unflatten(&file)==B_OK)
	{
#ifdef DEBUG_COLORSET
settings.PrintToStream();
#endif
		BString internal_name;
		settings.FindString("name",&internal_name);
//		BString namestr("Color Set: ");
//		namestr+=internal_name.String();
//		colorset_label->SetText(namestr.String());
		SetColorSetName(internal_name.String());

		picker->SetValue(GetColorFromMessage(&settings,attrstring.String()));
		colorwell->SetColor(picker->ValueAsColor());
		return;
	}
#ifdef DEBUG_COLORSET
printf("Error unflattening file %s\n",name.String());
#endif
}

void APRView::SaveColorSet(const BString &name)
{

	// Save the current color attributes as a flattened BMessage in the 
	// color set folder
	BString path(COLOR_SET_DIR);
	path+=name.String();

#ifdef DEBUG_COLORSET
printf("SaveColorSet: %s\n",path.String());
#endif

	if(settings.ReplaceString("name",name.String())!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("SaveColorSet: Couldn't replace set name in settings\n");
#endif
	}

	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("SaveColorSet: Couldn't open settings file for write\n");
#endif
	}
	
	if(settings.Flatten(&file)!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("SaveColorSet: Couldn't flatten settings to file\n");
#endif
		return;
	}

	BMessage *msg=new BMessage(LOAD_COLORSET);
	msg->AddString("name",name.String());

	if(colorset_menu->AddItem(new BMenuItem(name.String(),msg))==false)
	{
#ifdef DEBUG_COLORSET
printf("SaveColorSet: Error in adding item to menu\n");
#endif
	}
	SetColorSetName(name.String());
}

void APRView::DeleteColorSet(const BString &name)
{
	// Moves the current color set to the trash if it has been saved to disk
	// Note that it also makes the name <untitled> once more.
	
}

color_which APRView::SelectionToAttribute(int32 index)
{
	// This simply converts the selected index to the appropriate color_which
	// attribute. A bit of a hack, but it should be relarively easy to maintain
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

void APRView::SetColorSetName(const char *name)
{
	if(!name)
		return;
	BString namestr("Color Set: ");
	colorset_name->SetTo(name);
	namestr+=name;
	colorset_label->SetText(namestr.String());
	colorset_label->Invalidate();
}

void APRView::SaveSettings(void)
{
	// Save the current GUI color settings to the GUI colors file in the 
	// path specified in defs.h
	
	BString path(SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;
	printf("%s\n",path.String());
	BFile file(path.String(),B_READ_WRITE|B_CREATE_FILE|B_ERASE_FILE);
	
	settings.Flatten(&file);
}

void APRView::LoadSettings(void)
{
	// Load the current GUI color settings from disk. This is done instead of
	// getting them from the server at this point for testing purposes. Comment
	// out the #define LOAD_SETTINGS_FROM_DISK line to use the server query code

#ifdef LOAD_SETTINGS_FROM_DISK

#ifdef DEBUG_COLORSET
printf("Loading settings from disk\n");
#endif
	settings.MakeEmpty();

	BDirectory dir,newdir;
	if(dir.SetTo(SETTINGS_DIR)==B_ENTRY_NOT_FOUND)
	{
#ifdef DEBUG_COLORSET
printf("Color set folder not found. Creating %s\n",SETTINGS_DIR);
#endif
		create_directory(SETTINGS_DIR,0777);
	}

	BString path(SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;
	BFile file(path.String(),B_READ_ONLY);

	if(file.InitCheck()!=B_OK)
	{
#ifdef DEBUG_COLORSET
printf("Couldn't open file %s for read\n",path.String());
#endif
		return;
	}
	if(settings.Unflatten(&file)==B_OK)
	{
		settings.FindString("name",colorset_name);
		SetColorSetName(colorset_name->String());
//		BString namestr("Color Set: ");
//		namestr+=colorset_name->String();
//		colorset_label->SetText(namestr.String());

		picker->SetValue(GetColorFromMessage(&settings,attrstring.String()));
		colorwell->SetColor(picker->ValueAsColor());
#ifdef DEBUG_COLORSET
settings.PrintToStream();
#endif
		return;
	}
#ifdef DEBUG_COLORSET
printf("Error unflattening SystemColors file %s\n",path.String());
#endif
	
	// If we get this far, we have encountered an error, so reset the settings
	// to the defaults
	SetDefaults();
	SaveSettings();
#else
	// TODO:
	// Write the server query code
#endif
}

void APRView::SetDefaults(void)
{
#ifdef DEBUG_COLORSET
printf("Initializing color settings to defaults\n");
#endif
	settings.MakeEmpty();
	settings.AddString("name","Default");
	colorset_name->SetTo("Default");
	
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

//	BString labelstr("Color Set: ");
//	labelstr+=colorset_name->String();
//	colorset_label->SetText(labelstr.String());
	SetColorSetName("Default");
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

const char *APRView::AttributeToString(const color_which &attr)
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
