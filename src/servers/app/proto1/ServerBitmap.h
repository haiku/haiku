#ifndef _SERVER_BITMAP_H_
#define _SERVER_BITMAP_H_

#include <GraphicsDefs.h>
#include <Rect.h>
#include "DisplayDriver.h"

class ServerBitmap
{
public:
	ServerBitmap(BRect rect,color_space cspace,int32 BytesPerLine=0);
	~ServerBitmap(void);
	
	int32 width,height;
	int32 bytesperline;
	color_space colorspace;
	uint8 *buffer;
	DisplayDriver *driver;
	bool is_vram;
};

#endif