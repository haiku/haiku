/*
	Desktop.cpp:
		Code necessary to handle the Desktop, defined as the collection of workspaces.
*/

//#define DEBUG_WORKSPACES

#include <List.h>
#include <Path.h>
#include <GraphicsCard.h>
#include <stdio.h>
#include <Locker.h>
#include <File.h>
#include <String.h>
#include "ServerProtocol.h"
#include "ColorUtils.h"
#include "SystemPalette.h"
#include "ServerBitmap.h"
#include "ServerWindow.h"
#include "Layer.h"
#include "DisplayDriver.h"
#include "ViewDriver.h"
#include "ScreenDriver.h"
#include "Desktop.h"
#include "WindowBorder.h"
#include "ColorSet.h"
#include "RGBColor.h"

class ServerWindow;
class ServerBitmap;
class Workspace;

rgb_color GetColorFromMessage(BMessage *msg, const char *name, int32 index=0);

// Temporarily necessary. Will not be present in the app_server.
int8 default_cursor[]={ 
	16,1,0,0,
	
	// data
	0xff, 0x80, 0x80, 0x80, 0x81, 0x00, 0x80, 0x80, 0x80, 0x40, 0x80, 0x80, 0x81, 0x00, 0xa2, 0x00, 
	0xd4, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

	// mask
	0xff, 0x80, 0xff, 0x80, 0xff, 0x00, 0xff, 0x80, 0xff, 0xc0, 0xff, 0x80, 0xff, 0x00, 0xfe, 0x00, 
	0xdc, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//--------------------GLOBALS-------------------------
uint32 workspace_count, active_workspace;
BList *desktop;
ServerBitmap *startup_cursor;
Workspace *pactive_workspace;
DisplayDriver *gfxdriver;
int32 token_count=-1;
BLocker *workspacelock;
BLocker *layerlock;
BLocker *colorlock;
UpdateNode *upnode;
ColorSet *gui_colors;
//----------------------------------------------------


//-------------------INTERNAL CLASS DEFS--------------
class Workspace
{
public:
	Workspace(void);
	Workspace(BPath imagepath);
	~Workspace();
	void SetBGColor(rgb_color col);
	rgb_color BGColor(void);
	thread_id tid;
	
	RootLayer *toplayer;
	BList focuslist;
	rgb_color bgcolor;
	screen_info screendata,
				olddata;
};

DisplayDriver *get_gfxdriver(void)
{
	return gfxdriver;
}

Workspace::Workspace(void)
{
	workspace_count++;

	// default values
	focuslist.MakeEmpty();
	bgcolor.red=51;
	bgcolor.green=102;
	bgcolor.blue=160;
	screendata.mode=B_CMAP8;
	screendata.spaces=B_8_BIT_640x480;
	screendata.refresh_rate=60.0;
	screendata.min_refresh_rate=60.0;
	screendata.max_refresh_rate=60.0;
	screendata.h_position=(uchar)50;
	screendata.v_position=(uchar)50;
	screendata.h_size=(uchar)50;
	screendata.v_size=(uchar)50;
	olddata=screendata;

	// We need one top layer for each workspace times the number of monitors - ick.
	// Currently, we only have 1 monitor. *Whew*
	char wspace_name[128];
	sprintf(wspace_name,"workspace %ld root",workspace_count);
	toplayer=new RootLayer(BRect(0,0,639,439),wspace_name);
}

Workspace::~Workspace(void)
 {
	workspace_count--;
	toplayer->PruneTree();
}

void Workspace::SetBGColor(rgb_color col)
{
	bgcolor=col;
	toplayer->SetColor(col);
#ifdef DEBUG_WORKSPACES
printf("Workspace::SetBGColor(%d,%d,%d,%d)\n",bgcolor.red,bgcolor.green,
	bgcolor.blue,bgcolor.alpha);
#endif
}

rgb_color Workspace::BGColor(void)
{
	return bgcolor;
}
//---------------------------------------------------


//-----------------GLOBAL FUNCTION DEFS---------------

void init_desktop(int8 workspaces)
{
#ifdef DEBUG_WORKSPACES
printf("init_desktop(%d)\n",workspaces);
#endif
	workspace_count=workspaces;
	if(workspace_count==0)
		workspace_count++;

	// Instantiate and initialize display driver
//	gfxdriver=new ViewDriver();
//	gfxdriver=new SecondDriver();
	gfxdriver=new ScreenDriver();
	gfxdriver->Initialize();

	workspacelock=new BLocker();
	layerlock=new BLocker();
	colorlock=new BLocker();
	gui_colors=new ColorSet;

#ifdef DEBUG_WORKSPACES
printf("Driver %s\n", (gfxdriver->IsInitialized()==true)?"initialized":"NOT initialized");
#endif
	
	desktop=new BList(0);

	// Create the workspaces we're supposed to have
	for(int8 i=0; i<workspaces; i++)
		desktop->AddItem(new Workspace());

	// Load workspace preferences here

	// We're going to punch in some hardcoded settings for testing purposes.
	// There are 3 workspaces hardcoded in for now, but we'll only use the 
	// second one.
	Workspace *wksp=(Workspace*)desktop->ItemAt(1);
	if(wksp!=NULL)	// in case I forgot something
	{
		rgb_color bcol;
		SetRGBColor(&bcol,0,200,236);
		
//		SetRGBColor(&(wksp->bgcolor),0,200,236);
		wksp->SetBGColor(bcol);
		screen_info *tempsd=&(wksp->screendata);
		tempsd->mode=B_CMAP8;
		tempsd->spaces=B_8_BIT_800x600;
		tempsd->frame.Set(0,0,799,599);
	}
	else
		printf("DW goofed on the workspace index :P\n");

	// Activate workspace 0
	pactive_workspace=(Workspace *)desktop->ItemAt(0);
	pactive_workspace->screendata.spaces=B_32_BIT_640x480;
	gfxdriver->SetMode(pactive_workspace->screendata.spaces);

	// Clear the screen
	pactive_workspace->toplayer->SetColor(RGBColor(80,85,152));
//	gfxdriver->Clear(pactive_workspace->toplayer->GetColor());

	// CURSOR_ARROW
	startup_cursor=new ServerBitmap('CURS',"CURSOR_DEFAULT");
	if(!startup_cursor->InitCheck())
	{
		// Apparently, something is screwed up beyond recognition in loading the cursor,
		// so we'll put in one just so we have something usable.
		delete startup_cursor;
		startup_cursor=new ServerBitmap(default_cursor);
	}
	gfxdriver->SetCursor(startup_cursor, BPoint(0,0));
	gfxdriver->ShowCursor();

	pactive_workspace->toplayer->SetVisible(true);
#ifdef DEBUG_WORKSPACES
printf("Desktop initialized\n");
#endif
}

void shutdown_desktop(void)
{
	int32 i,count=desktop->CountItems();
	Workspace *w;
	for(i=0;i<count;i++)
	{
		w=(Workspace*)desktop->ItemAt(i);
		if(w!=NULL)
			delete w;
	}
	desktop->MakeEmpty();
	delete desktop;
	gfxdriver->Shutdown();
	delete gfxdriver;
	delete workspacelock;
	delete layerlock;
	delete colorlock;
	delete startup_cursor;
	delete gui_colors;
}


int32 CountWorkspaces(void)
{
#ifdef DEBUG_WORKSPACES
printf("CountWorkspaces() returned %ld\n",workspace_count);
#endif
	return workspace_count;
}

void SetWorkspaceCount(uint32 count)
{
	if(count<0)
	{
#ifdef DEBUG_WORKSPACES
printf("SetWorkspaceCount(value<0) - DOH!\n");
#endif
		return;
	}
	if(count==workspace_count)
		return;

	// Activate last workspace in new set if we're using one to be nuked
	if(count<active_workspace)
		activate_workspace(count);

	if(count < workspace_count)
	{
		for(uint32 i=count;i<workspace_count;i++)
		{
			if(desktop->ItemAt(i)!=NULL)
			{
				delete (Workspace*)desktop->ItemAt(i);
				desktop->RemoveItem(i);
			}
		}
	}
	else
	{
		// count > workspace_count here
		
		// Once we have default workspace preferences which can be set by the
		// user, we'll end up having to initialize newly-added workspaces to that
		// data. In the mean time, the defaults in the constructor will do. :)
		for(uint32 i=count;i<workspace_count;i++)
		{
			desktop->AddItem(new Workspace());
		}
	}
	
	// don't forget to update our global!
	workspace_count=desktop->CountItems();

#ifdef DEBUG_WORKSPACES
printf("SetWorkspaceCount(%ld)\n",workspace_count);
#endif

}

int32 CurrentWorkspace(void)
{
#ifdef DEBUG_WORKSPACES
printf("CurrentWorkspace() returned %ld\n",active_workspace);
#endif
	return active_workspace;
}

void ActivateWorkspace(uint32 workspace)
{
	// In order to change workspaces, we'll need to make all the necessary
	// changes to the screen to fit the new one, such as resolution,
	// background color, color depth, refresh rate, etc.

	if(workspace>workspace_count || workspace<0)
		return;
		
	Workspace *proposed=(Workspace *)desktop->ItemAt(workspace);

	if(proposed==NULL)
		return;

	// Here's where we update all the screen stuff
//	if(pactive_workspace->screendata.spaces != proposed->screendata.spaces)
//	{
		gfxdriver->SetMode(proposed->screendata.spaces);
//		gfxdriver->Clear(proposed->bgcolor);
		pactive_workspace->toplayer->SetVisible(false);
		proposed->toplayer->SetVisible(true);
//	}
	// more if != then change code goes here. Currently, a lot of the code which
	// uses this stuff is not implemented, so it's kind of moot. However, when we
	// use the real graphics HW, it'll become important.
	
	// actually set our active workspace data to reflect the change in workspaces
	pactive_workspace=proposed;
	active_workspace=workspace;
#ifdef DEBUG_WORKSPACES
printf("ActivateWorkspace(%ld)\n",active_workspace);
#endif
}

const color_map *SystemColors(void)
{
	return (const color_map *)&system_palette;
}

status_t SetScreenSpace(uint32 index, uint32 res, bool stick = true)
{
#ifdef DEBUG_WORKSPACES
printf("SetScreenSpace(%ld,%lu, %s)\n",index,res,(stick==true)?"true":"false");
#endif

	if(index>workspace_count-1)
	{
		printf("SetScreenSpace was passed invalid workspace value %ld\n",index);
		return B_ERROR;
	}
	if(res>0x800000)	// the last "normal" screen mode defined in GraphicsDefs.h
	{
		printf("SetScreenSpace was passed invalid screen value %lu\n",res);
		return B_ERROR;
	}

	// When we actually use app_server preferences, we'll save the screen mode to
	// the preferences so that we use this mode next boot. For now, we treat stick==false
	if(stick==true)
	{
		// save the screen data here
	}
	if(index==active_workspace)
	{
		gfxdriver->SetMode(res);
//		gfxdriver->Clear(pactive_workspace->bgcolor);
	}
	else
	{
		// just set the workspace's data here. right now, do nothing.
	}
	return B_OK;
}

int32 GetViewToken(void)
{
	// Used to generate view and layer tokens
	return ++token_count;
}

void AddWindowToDesktop(ServerWindow *win,uint32 workspace)
{
	// Adds the top layer of each window to the root layer in the specified workspace.
	// Note that in calling AddChild, a redraw is invoked
	if(!win || workspace>workspace_count-1)
		return;

	workspacelock->Lock();
	Workspace *w=(Workspace *)desktop->ItemAt(workspace);
	if(w)
	{
		// a check for the B_CURRENT_WORKSPACE or B_ALL_WORKSPACES specifiers will
		// go here later
		
		w->toplayer->AddChild(win->winborder);
	}
	workspacelock->Unlock();
}

void RemoveWindowFromDesktop(ServerWindow *win)
{
	// A little more time-consuming than AddWindow
	if(!win)
		return;

	workspacelock->Lock();

	// a check for the B_CURRENT_WORKSPACE or B_ALL_WORKSPACES specifiers will
	// go here later
	
	// If the window belongs on only 1 workspace, calling RemoveSelf is the most
	// elegant solution.
	win->winborder->RemoveSelf();
	workspacelock->Unlock();
}

Layer *GetRootLayer(void)
{
	layerlock->Lock();
	Layer *lay=(workspace_count>0)?pactive_workspace->toplayer:NULL;
	layerlock->Unlock();
	
	return lay;
}

ServerWindow *GetActiveWindow(int32 workspace)
{
	Layer *root=GetRootLayer();
	if(!root)
		return NULL;
	Layer *lay=root->topchild, *nextlay;
	while(lay!=NULL)
	{
		if(lay->serverwin && lay->serverwin->HasFocus())
			return lay->serverwin;
		nextlay=lay->lowersibling;
		lay=nextlay;
	}
	return NULL;
}

rgb_color GetColorFromMessage(BMessage *msg, const char *name, int32 index=0)
{
	// Simple function to do the dirty work of getting an rgb_color from
	// a message
	rgb_color *col,rcolor={0,0,0,0};
	ssize_t size;

	if(msg->FindData(name,(type_code)'RGBC',index,(const void**)&col,&size)==B_OK)
		rcolor=*col;
	return rcolor;
}

void SetGUIColor(color_which attr, RGBColor color)
{
	colorlock->Lock();
	
	switch(attr)
	{
		case B_PANEL_BACKGROUND_COLOR:
		{
			gui_colors->panel_background=color;
			break;
		}
		case B_MENU_BACKGROUND_COLOR:
		{
			gui_colors->menu_background=color;
			break;
		}
		case B_MENU_SELECTION_BACKGROUND_COLOR:
		{
			gui_colors->menu_selected_background=color;
			break;
		}
		case B_MENU_ITEM_TEXT_COLOR:
		{
			gui_colors->menu_text=color;
			break;
		}
		case B_MENU_SELECTED_ITEM_TEXT_COLOR:
		{
			gui_colors->menu_selected_text=color;
			break;
		}
		case B_WINDOW_TAB_COLOR:
		{
			gui_colors->window_tab=color;
			break;
		}
		case B_KEYBOARD_NAVIGATION_COLOR:
		{
			gui_colors->keyboard_navigation=color;
			break;
		}
		case B_DESKTOP_COLOR:
		{
			gui_colors->desktop=color;
			break;
		}
		default:
			break;
	}
	
	colorlock->Unlock();
}

RGBColor GetGUIColor(color_which attr)
{
	colorlock->Lock();

	RGBColor c;

	switch(attr)
	{
		case B_PANEL_BACKGROUND_COLOR:
		{
			c=gui_colors->panel_background;
			break;
		}
		case B_MENU_BACKGROUND_COLOR:
		{
			c=gui_colors->menu_background;
			break;
		}
		case B_MENU_SELECTION_BACKGROUND_COLOR:
		{
			c=gui_colors->menu_selected_background;
			break;
		}
		case B_MENU_ITEM_TEXT_COLOR:
		{
			c=gui_colors->menu_text;
			break;
		}
		case B_MENU_SELECTED_ITEM_TEXT_COLOR:
		{
			c=gui_colors->menu_selected_text;
			break;
		}
		case B_WINDOW_TAB_COLOR:
		{
			c=gui_colors->window_tab;
			break;
		}
		case B_KEYBOARD_NAVIGATION_COLOR:
		{
			c=gui_colors->keyboard_navigation;
			break;
		}
		case B_DESKTOP_COLOR:
		{
			c=gui_colors->desktop;
			break;
		}
		default:
			break;
	}
	
	colorlock->Unlock();
	return c;
}

void SetGUIColors(ColorSet set)
{
	colorlock->Lock();
	*gui_colors=set;
	colorlock->Unlock();
}

ColorSet GetGUIColors(void)
{
	colorlock->Lock();
	ColorSet cs(*gui_colors);
	colorlock->Unlock();

	return cs;
}

void LoadGUIColors(void)
{
	// Load the current GUI color settings from a color set file.

	BString path(SERVER_SETTINGS_DIR);
	path+=COLOR_SETTINGS_NAME;

	BFile file(path.String(),B_READ_ONLY);
	BMessage msg;

	if(file.InitCheck()==B_OK)
	{
		if(msg.Unflatten(&file)==B_OK)
		{
			// Just get the "official" GUI colors for now
			gui_colors->panel_background=GetColorFromMessage(&msg,"PANEL_BACKGROUND");
			gui_colors->menu_background=GetColorFromMessage(&msg,"MENU_BACKGROUND");
			gui_colors->menu_selected_background=GetColorFromMessage(&msg,"MENU_SELECTION_BACKGROUND");
			gui_colors->menu_text=GetColorFromMessage(&msg,"MENU_ITEM_TEXT");
			gui_colors->menu_selected_text=GetColorFromMessage(&msg,"MENU_SELECTED_ITEM_TEXT");
			gui_colors->window_tab=GetColorFromMessage(&msg,"WINDOW_TAB");
			gui_colors->keyboard_navigation=GetColorFromMessage(&msg,"KEYBOARD_NAVIGATION");
			gui_colors->desktop=GetColorFromMessage(&msg,"DESKTOP");
			
			// fake the rest
		}
		else
			printf("app_server couldn't get GUI color settings from %s%s. Resetting to defaults\n",
				SERVER_SETTINGS_DIR,COLOR_SETTINGS_NAME);
	}
	else
		printf("app_server couldn't open %s%s. Resetting GUI colors to defaults\n",
			SERVER_SETTINGS_DIR,COLOR_SETTINGS_NAME);
}

// Another "just in case DW screws up again" section
#ifdef DEBUG_WORKSPACES
#undef DEBUG_WORKSPACES
#endif