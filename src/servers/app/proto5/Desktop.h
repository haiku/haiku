#ifndef _OBDESKTOP_H_
#define _OBDESKTOP_H_


#include <InterfaceDefs.h>
#include <GraphicsDefs.h>
#include <Rect.h>

class DisplayDriver;
class ServerWindow;
class Layer;

void init_desktop(int8 workspaces);
void shutdown_desktop(void);
DisplayDriver *get_gfxdriver(void);

// Workspace access functions
int32 CountWorkspaces(void);
void SetWorkspaceCount(uint32 count);

int32 CurrentWorkspace(void);
void ActivateWorkspace(uint32 workspace);

const color_map *SystemColors(void);
status_t SetScreenSpace(uint32 index, uint32 res, bool stick=true);

void AddWindowToDesktop(ServerWindow *win,uint32 workspace);
void RemoveWindowFromDesktop(ServerWindow *win);
Layer *GetRootLayer(void);
int32 GetViewToken(void);

typedef struct 
{
	color_space mode;
	BRect frame;
	uint32 spaces;
	float min_refresh_rate;
	float max_refresh_rate;
	float refresh_rate;
	uchar h_position;
	uchar v_position;
	uchar h_size;
	uchar v_size;
} screen_info;

#endif