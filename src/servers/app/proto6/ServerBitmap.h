#ifndef _SERVER_BITMAP_H_
#define _SERVER_BITMAP_H_

#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>
#include "DisplayDriver.h"

class ServerBitmap
{
public:
	ServerBitmap(BRect rect,color_space space,int32 BytesPerLine=0);
	ServerBitmap(int32 w,int32 h,color_space space,int32 BytesPerLine=0);
	ServerBitmap(int32 w,int32 h,color_space space, area_id areaID, int32 BytesPerLine=0);
	ServerBitmap(void);
	ServerBitmap(const char *path);
	~ServerBitmap(void);
	void UpdateSettings(void);
	void SetBuffer(void *buffer);
	uint8 *Buffer(void);
	void SetArea(area_id ID);
	area_id Area(void);
	uint32 BitsLength(void);
	BRect Bounds() { return BRect(0,0,width-1,height-1); };
	int32 BytesPerRow(void) { return bytesperline; };

	int32 width,height;
	int32 bytesperline;
	color_space cspace;
	int bpp;

protected:
	void HandleSpace(color_space space, int32 BytesPerLine);
	DisplayDriver *driver;
	bool is_vram,is_area;
	area_id areaid;
	uint8 *buffer;
};

#endif