#ifndef _SERVER_CURSOR_H
#define _SERVER_CURSOR_H

#include <GraphicsDefs.h>
#include <Rect.h>
#include "ServerBitmap.h"

class ServerCursor
{
public:
//	ServerCursor(ServerBitmap *bmp,ServerBitmap *cmask=NULL);
	ServerCursor(int8 *data);
	~ServerCursor(void);
	BRect Bounds() { return BRect(0,0,width-1,height-1); };
	uint8 *Buffer(void) const { return buffer;}
	int32 Width(void) const { return width; }
	int32 Height(void) const { return height; }
	int32 BytesPerLine(void) const { return bytesperline; };
	void PrintToStream(void);
protected:
	uint8 *buffer;
	int32 width,height;
	int32 bytesperline;
};

#endif