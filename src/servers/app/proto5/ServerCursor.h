#ifndef _SERVER_SPRITE_H
#define _SERVER_SPRITE_H

#include <GraphicsDefs.h>
#include <Rect.h>

class ServerBitmap;

class ServerCursor
{
public:
	ServerCursor(ServerBitmap *bmp,ServerBitmap *cmask=NULL);
	ServerCursor(int8 *data);
	ServerCursor(void);
	~ServerCursor(void);
	void MoveTo(int32 x, int32 y);
	void SetCursor(int8 *data);
	void SetCursor(ServerBitmap *bmp, ServerBitmap *cmask=NULL);
	
	ServerBitmap 	*bitmap;
	
	color_space cspace;
	BRect bounds;
	int	width,height;
	BPoint position,hotspot;
	bool is_initialized;
};

extern int8 default_cursor[];

#endif