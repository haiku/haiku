#ifndef _OBDESKTOP_H_
#define _OBDESKTOP_H_

#include <GraphicsDefs.h>
#include <Rect.h>
class DisplayDriver;

typedef enum
{	PLACE_MANUAL=0,
	PLACE_SCALE=1,
	PLACE_TILE=2
} wallpaper_placement;

void init_desktop(int8 workspaces);
void shutdown_desktop(void);
DisplayDriver *get_gfxdriver(void);

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