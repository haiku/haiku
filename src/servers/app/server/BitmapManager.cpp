/*
	BitmapManager:
		Handler for allocating and freeing area memory for BBitmaps on the server side.
		Utilizes the public domain pool allocator BGET.
*/

#include "BitmapManager.h"
#include "ServerBitmap.h"
#include <stdio.h>

// Define BGET function calls here because of C++ name mangling causes link problems
typedef long bufsize;
extern "C" void	bpool(void *buffer, bufsize len);
extern "C" void *bget(bufsize size);
extern "C" void *bgetz(bufsize size);
extern "C" void *bgetr(void *buffer, bufsize newsize);
extern "C" void	brel(void *buf);
extern "C" void	bectl(int (*compact)(bufsize sizereq, int sequence),
		       void *(*acquire)(bufsize size),
		       void (*release)(void *buf), bufsize pool_incr);
extern "C" void	bstats(bufsize *curalloc, bufsize *totfree, bufsize *maxfree,
		       long *nget, long *nrel);
extern "C" void	bstatse(bufsize *pool_incr, long *npool, long *npget,
		       long *nprel, long *ndget, long *ndrel);
extern "C" void	bufdump(void *buf);
extern "C" void	bpoold(void *pool, int dumpalloc, int dumpfree);
extern "C" int	bpoolv(void *pool);

// This is a call which makes BGET use a couple of home-grown functions which
// manage the buffer via area functions
extern "C" void set_area_buffer_management(void);

#define BITMAP_AREA_SIZE	B_PAGE_SIZE * 2

BitmapManager::BitmapManager(void)
{
	bmplist=new BList(0);

	// When create_area is passed the B_ANY_ADDRESS flag, the address of the area
	// is stored in the pointer to a pointer.
	bmparea=create_area("bitmap_area",(void**)&buffer,B_ANY_ADDRESS,BITMAP_AREA_SIZE,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if(bmparea==B_BAD_VALUE ||
	   bmparea==B_NO_MEMORY ||
	   bmparea==B_ERROR)
	   	printf("PANIC: BitmapManager couldn't allocate area!!\n");
	
	lock=create_sem(1,"bmpmanager_lock");
	if(lock<0)
		printf("PANIC: BitmapManager couldn't allocate locking semaphore!!\n");
	
	set_area_buffer_management();
	bpool(buffer,BITMAP_AREA_SIZE);
}

BitmapManager::~BitmapManager(void)
{
	if(bmplist->CountItems()>0)
	{
		ServerBitmap *tbmp=NULL;
		int32 itemcount=bmplist->CountItems();
		for(int32 i=0; i<itemcount; i++)
		{
			tbmp=(ServerBitmap*)bmplist->RemoveItem(0L);
			if(tbmp)
			{
				brel(tbmp->_buffer);
				delete tbmp;
				tbmp=NULL;
			}
		}
	}
	delete bmplist;
	delete_area(bmparea);
	delete_sem(lock);
}

ServerBitmap * BitmapManager::CreateBitmap(BRect bounds, color_space space, int32 flags,
	int32 bytes_per_row=-1, screen_id screen=B_MAIN_SCREEN_ID)
{
	acquire_sem(lock);
	ServerBitmap *bmp=new ServerBitmap(bounds, space, flags, bytes_per_row);
	
	// Server version of this code will also need to handle such things as
	// bitmaps which accept child views by checking the flags.
	uint8 *bmpbuffer=(uint8 *)bget(bmp->BitsLength());

	if(!bmpbuffer)
	{
		delete bmp;
		return NULL;
	}
	bmp->_area=area_for(bmpbuffer);
	bmp->_buffer=bmpbuffer;
	bmplist->AddItem(bmp);
	release_sem(lock);
	return bmp;
}

void BitmapManager::DeleteBitmap(ServerBitmap *bitmap)
{
	acquire_sem(lock);
	ServerBitmap *tbmp=(ServerBitmap*)bmplist->RemoveItem(bmplist->IndexOf(bitmap));

	if(!tbmp)
	{
		release_sem(lock);
		return;
	}

	// Server code will require a check to ensure bitmap doesn't have its own area		
	brel(tbmp->_buffer);
	delete tbmp;

	release_sem(lock);
}
