#ifndef _SERVER_SPRITE_H
#define _SERVER_SPRITE_H

#include <GraphicsDefs.h>
#include <Rect.h>

class ServerBitmap;

class ServerCursor
{
public:
	ServerCursor(ServerBitmap *bmp);
	ServerCursor(int8 *data,color_space space);
	~ServerCursor(void);
	void MoveTo(int32 x, int32 y);
	void SetCursor(int8 *data, color_space space);
	void SetCursor(ServerBitmap *bmp);
	
	ServerBitmap 	*bitmap,
					*mask;
	
	color_space cspace;
	BRect bounds;
	int	width,height;
	BPoint position;
	bool is_initialized;
};

#endif