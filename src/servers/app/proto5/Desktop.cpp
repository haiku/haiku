/*
	Desktop.cpp:
		Code necessary to handle the Desktop, defined as the collection of workspaces.
*/

#define DEBUG_WORKSPACES

#include <List.h>
#include <Path.h>
#include <GraphicsCard.h>
#include <stdio.h>
#include <Locker.h>
#include "ServerWindow.h"
#include "ServerCursor.h"
#include "Layer.h"
#include "DisplayDriver.h"
#include "ViewDriver.h"
#include "SecondDriver.h"
#include "Desktop.h"
#include "WindowBorder.h"

class ServerWindow;
class ServerBitmap;
class Workspace;

// Quick hack to make setting rgb_colors much easier to live with
void set_rgb_color(rgb_color *col,uint8 red, uint8 green, uint8 blue, uint8 alpha=255)
{
	col->red=red;
	col->green=green;
	col->blue=blue;
	col->alpha=alpha;
} 

//--------------------GLOBALS-------------------------
uint32 workspace_count, active_workspace;
BList *desktop;
ServerCursor *startup_cursor;
Workspace *pactive_workspace;
color_map system_palette;
DisplayDriver *gfxdriver;
int32 token_count=-1;
BLocker *workspacelock;
BLocker *layerlock;
UpdateNode *upnode;
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
	gfxdriver=new ViewDriver();
//	gfxdriver=new SecondDriver();
	gfxdriver->Initialize();

	workspacelock=new BLocker();
	layerlock=new BLocker();

#ifdef DEBUG_WORKSPACES
printf("Driver %s\n", (gfxdriver->IsInitialized()==true)?"initialized":"NOT initialized");
#endif
	
	desktop=new BList(0);

	// Create the workspaces we're supposed to have
	for(int8 i=0; i<workspaces; i++)
	{
		desktop->AddItem(new Workspace());
	}

	// Load workspace preferences here

	// We're going to punch in some hardcoded settings for testing purposes.
	// There are 3 workspaces hardcoded in for now, but we'll only use the 
	// second one.
	Workspace *wksp=(Workspace*)desktop->ItemAt(1);
	if(wksp!=NULL)	// in case I forgot something
	{
		rgb_color bcol;
		set_rgb_color(&bcol,0,200,236);
		
//		set_rgb_color(&(wksp->bgcolor),0,200,236);
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
	gfxdriver->SetScreen(pactive_workspace->screendata.spaces);

	// Clear the screen
	set_rgb_color(&(pactive_workspace->toplayer->bgcolor),80,85,152);
	gfxdriver->Clear(pactive_workspace->toplayer->bgcolor);
	startup_cursor=new ServerCursor(default_cursor);
	gfxdriver->SetCursor(startup_cursor);

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
	delete startup_cursor;
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
		gfxdriver->SetScreen(proposed->screendata.spaces);
		gfxdriver->Clear(proposed->bgcolor);
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
		gfxdriver->SetScreen(res);
		gfxdriver->Clear(pactive_workspace->bgcolor);
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

// Another "just in case DW screws up again" section
#ifdef DEBUG_WORKSPACES
#undef DEBUG_WORKSPACES
#endif