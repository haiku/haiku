#ifndef _SERVER_BITMAP_H_
#define _SERVER_BITMAP_H_

#include <GraphicsDefs.h>
#include <Rect.h>
#include <OS.h>

class DisplayDriver;

class ServerBitmap
{
public:
	ServerBitmap(BRect rect,color_space space,int32 BytesPerLine=0);
	ServerBitmap(int32 w,int32 h,color_space space,int32 BytesPerLine=0);
	ServerBitmap(const char *path);
	ServerBitmap(ServerBitmap *bitmap);
	~ServerBitmap(void);
	void UpdateSettings(void);
	void SetBuffer(void *ptr) { buffer=(uint8*)ptr; }
	void FreeBuffer(void) { if(buffer!=NULL) { delete buffer; buffer=NULL; } }
	uint8 *Bits(void);
	void SetArea(area_id ID);
	area_id Area(void);
	uint32 BitsLength(void);
	BRect Bounds() { return BRect(0,0,width-1,height-1); };
	int32 BytesPerRow(void) { return bytesperline; };
	uint8 BitsPerPixel(void) { return bpp; } 
	color_space ColorSpace(void) { return cspace; }
	
	int32 width,height;
	int32 bytesperline;
	color_space cspace;
	int bpp;

protected:
	void HandleSpace(color_space space, int32 BytesPerLine);
	DisplayDriver *driver;
	area_id areaid;
	uint8 *buffer;
};

#endif