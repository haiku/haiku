#ifndef BITMAP_MANAGER_H_
#define BITMAP_MANAGER_H_

#include <GraphicsDefs.h>
#include <List.h>
#include <Rect.h>
#include <OS.h>
#include "TokenHandler.h"

class ServerBitmap;
class BitmapManager
{
public:
	BitmapManager(void);
	~BitmapManager(void);
	ServerBitmap *CreateBitmap(BRect bounds, color_space space, int32 flags,
		int32 bytes_per_row=-1, screen_id screen=B_MAIN_SCREEN_ID);
	void DeleteBitmap(ServerBitmap *bitmap);
protected:
	BList *bmplist;
	area_id bmparea;
	int8 *buffer;
	TokenHandler tokenizer;
	sem_id lock;
};

#endif