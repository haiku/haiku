/*
	Desktop.cpp:
		Code necessary to handle the Desktop, defined as the collection of workspaces.
*/

//#define DEBUG_WORKSPACES

#include <List.h>
#include <Path.h>
#include <GraphicsCard.h>
#include <stdio.h>
#include "DisplayDriver.h"
#include "ViewDriver.h"
#include "Desktop.h"

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
int32 workspace_count, active_workspace;
BList *desktop;
Workspace *pactive_workspace;
color_map system_palette;
ViewDriver *gfxdriver;
//----------------------------------------------------


//-------------------INTERNAL CLASS DEFS--------------
class Workspace
{
public:
	Workspace(void);
	Workspace(BPath imagepath);
	~Workspace();
	
	BList layerlist;
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
	layerlist.MakeEmpty();
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
}

Workspace::~Workspace(void)
{
	workspace_count--;
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
	gfxdriver->Initialize();

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
		set_rgb_color(&(wksp->bgcolor),0,200,236);
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
	set_rgb_color(&(pactive_workspace->bgcolor),80,85,152);
	gfxdriver->Clear(pactive_workspace->bgcolor);

#ifdef DEBUG_WORKSPACES
printf("Desktop initialized\n");
#endif
}

void shutdown_desktop(void)
{
	int32 i,count=desktop->CountItems();
	for(i=0;i<count;i++)
	{
		if(desktop->ItemAt(i)!=NULL)
			delete desktop->ItemAt(i);
	}
	desktop->MakeEmpty();
	delete desktop;
	gfxdriver->Shutdown();
	delete gfxdriver;
}


int32 CountWorkspaces(void)
{
#ifdef DEBUG_WORKSPACES
printf("CountWorkspaces() returned %ld\n",workspace_count);
#endif
	return workspace_count;
}

void SetWorkspaceCount(int32 count)
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
		for(int32 i=count;i<workspace_count;i++)
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
		for(int32 i=count;i<workspace_count;i++)
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

void ActivateWorkspace(int32 workspace)
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
	if(pactive_workspace->screendata.spaces != proposed->screendata.spaces)
	{
		gfxdriver->SetScreen(proposed->screendata.spaces);
		gfxdriver->Clear(proposed->bgcolor);
	}
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

status_t SetScreenSpace(int32 index, uint32 res, bool stick = true)
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

status_t GetScrollBarInfo(scroll_bar_info *info)
{
	return B_ERROR;
}

status_t SetScrollBarInfo(scroll_bar_info *info)
{
	//printf("set_scroll_bar_info()\n");
	return B_ERROR;
}

bigtime_t IdleTime()
{
	return 0;
}

void RunSelectPrinterPanel()
{	// Not sure what to do with this guy just yet
}
 
void RunAddPrinterPanel()
{	// Not sure what to do with this guy just yet
}
 
void RunBeAbout()
{	// the BeOS About Window belongs to Tracker....
}
 
void SetFocusFollowsMouse(bool follow)
{
	//printf("set_focus_follows_mouse(%s)\n",(follow==true)?"true":"false");
}
 
bool FocusFollowsMouse()
{
	return false;
}

// Another "just in case DW screws up again" section
#ifdef DEBUG_WORKSPACES
#undef DEBUG_WORKSPACES
#endif