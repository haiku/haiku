#ifndef _SERVER_BITMAP_H_
#define _SERVER_BITMAP_H_

#include <GraphicsDefs.h>
#include <Rect.h>
#include "DisplayDriver.h"

class ServerBitmap
{
public:
	ServerBitmap(BRect rect,color_space space,int32 BytesPerLine=0);
	ServerBitmap(int32 w,int32 h,color_space space,int32 BytesPerLine=0);
	ServerBitmap(void);
	ServerBitmap(const char *path);
	~ServerBitmap(void);
	void UpdateSettings(void);

	int32 width,height;
	int32 bytesperline;
	color_space cspace;
	int bpp;
	uint8 *buffer;

protected:
	void HandleSpace(color_space space, int32 BytesPerLine);
	DisplayDriver *driver;
	bool is_vram;
};

#endif