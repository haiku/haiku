
/*
	Screen.cpp:
		Function implementations of screen functions in InterfaceDefs.h
*/
#include <InterfaceDefs.h>
#include <List.h>
#include <Path.h>
#include <GraphicsCard.h>
#include <stdio.h>
#include "DisplayDriver.h"
#include "DirectDriver.h"
#include "Desktop.h"

class ServerWindow;
class ServerBitmap;
class ServerLayer;
class Workspace;

//--------------------GLOBALS-------------------------
int32 workspace_count, active_workspace;
BList desktop;
Workspace *pactive_workspace;
color_map system_palette;
DisplayDriver *gfxdriver;
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
	printf("init_desktop()\n");
/*
	Create the screen bitmap
	Get framebuffer area info
	Call setup_screenmode
	Create top layer for workspace
	Set cursor
	Turn mouse on in display driver
*/
	workspace_count=workspaces;
	if(workspace_count==0)
		workspace_count++;

	// Instantiate and initialize display driver
	gfxdriver=new DirectDriver();
	gfxdriver->Initialize();
	if(gfxdriver->IsInitialized())
		printf("Driver initialized\n");
	else
		printf("Driver NOT initialized\n");
	
	// Create the workspaces we're supposed to have
	for(int8 i=0; i<workspaces; i++)
	{
		desktop.AddItem(new Workspace());
	}

	printf("Added %d workspaces\n", workspaces);
	// Load workspace preferences here


	// Activate workspace 0
	pactive_workspace=(Workspace *)desktop.ItemAt(0);
	gfxdriver->SetScreen(pactive_workspace->screendata.spaces);
}

void shutdown_desktop(void)
{
	printf("Shutdown desktop()\n");
	int32 i,count=desktop.CountItems();
	for(i=0;i<count;i++)
	{
		if(desktop.ItemAt(i)!=NULL)
			delete desktop.ItemAt(i);
	}
	desktop.MakeEmpty();
	delete gfxdriver;
}

int32 count_workspaces(void)
{
	return workspace_count;
}

void set_workspace_count(int32 count)
{
	printf("set_workspace_count(%ld)\n",count);
	if(count<0)
		return;

	if(count<active_workspace)
		activate_workspace(count);

	for(int32 i=count+1;i<=workspace_count;i++)
	{
		if(desktop.ItemAt(i)!=NULL)
		{
			delete desktop.ItemAt(i);
			desktop.RemoveItem(i);
		}
	}
}

int32 current_workspace(void)
{
	return active_workspace;
}

void activate_workspace(int32 workspace)
{
	printf("activate_workspace(%ld)\n",workspace);
	if(workspace>workspace_count || workspace<0)
		return;
		
	Workspace *proposed=(Workspace *)desktop.ItemAt(workspace);

	if(proposed==NULL)
		return;

	if(pactive_workspace->screendata.spaces != proposed->screendata.spaces)
	{
		gfxdriver->SetScreen(proposed->screendata.spaces);
	}

	// What else needs to go here? --DW

}

const color_map *system_colors(void)
{
	return (const color_map *)&system_palette;
}

status_t set_screen_space(int32 index, uint32 res, bool stick = true)
{
	printf("set_screen_space(%ld,%lu, %s)\n",index,res,(stick==true)?"true":"false");
	return B_ERROR;
}

status_t get_scroll_bar_info(scroll_bar_info *info)
{
	return B_ERROR;
}

status_t set_scroll_bar_info(scroll_bar_info *info)
{
	printf("set_scroll_bar_info()\n");
	return B_ERROR;
}

bigtime_t idle_time()
{
	return 0;
}

void run_select_printer_panel()
{
}
 
void run_add_printer_panel()
{
}
 
void run_be_about()
{
}
 
void set_focus_follows_mouse(bool follow)
{
	printf("set_focus_follows_mouse(%s)\n",(follow==true)?"true":"false");
}
 
bool focus_follows_mouse()
{
	return false;
}
