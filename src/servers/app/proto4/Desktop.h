#ifndef _OBDESKTOP_H_
#define _OBDESKTOP_H_


#include <InterfaceDefs.h>
#include <GraphicsDefs.h>
#include <Rect.h>
class DisplayDriver;

void init_desktop(int8 workspaces);
void shutdown_desktop(void);
DisplayDriver *get_gfxdriver(void);

// Workspace access functions
int32 CountWorkspaces(void);
void SetWorkspaceCount(int32 count);
int32 CurrentWorkspace(void);
void ActivateWorkspace(int32 workspace);
const color_map *SystemColors(void);
status_t SetScreenSpace(int32 index, uint32 res, bool stick = true);

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