#ifndef _TESTBITMAP_H_
#define _TESTBITMAP_H_

#include <GraphicsDefs.h>
#include <OS.h>

enum {
	B_BITMAP_CLEAR_TO_WHITE				= 0x00000001,
	B_BITMAP_ACCEPTS_VIEWS				= 0x00000002,
	B_BITMAP_IS_AREA					= 0x00000004,
	B_BITMAP_IS_LOCKED					= 0x00000008 | B_BITMAP_IS_AREA,
	B_BITMAP_IS_CONTIGUOUS				= 0x00000010 | B_BITMAP_IS_LOCKED,
	B_BITMAP_IS_OFFSCREEN				= 0x00000020,
	B_BITMAP_WILL_OVERLAY				= 0x00000040 | B_BITMAP_IS_OFFSCREEN,
	B_BITMAP_RESERVE_OVERLAY_CHANNEL	= 0x00000080
};

#ifndef B_ANY_BYTES_PER_ROW
#define B_ANY_BYTES_PER_ROW -1
#endif

class TestBitmap
{
public:
	TestBitmap(BRect bounds,color_space depth,bool accepts_views = false,
		bool need_contiguous = false);
	TestBitmap(BRect bounds,uint32 flags,color_space depth,
		int32 bytesPerRow=B_ANY_BYTES_PER_ROW,screen_id screenID=B_MAIN_SCREEN_ID);
	TestBitmap(const TestBitmap *source,bool accepts_views = false,
		bool need_contiguous = false);
	~TestBitmap(void);

	status_t InitCheck();
	bool IsValid();

//	status_t LockBits(uint32 *state=NULL);
//	void UnlockBits();

	area_id Area();
	void *Bits();
	int32 BitsLength();
	int32 BytesPerRow();
	color_space	ColorSpace();
	BRect Bounds();

	void SetBits(const void *data,int32 length,int32 offset,color_space cs);
/*	virtual	void AddChild(BView *view);
	virtual	bool RemoveChild(BView *view);
	int32 CountChildren();
	BView *ChildAt(int32 index);
	BView *FindView(const char *view_name);
	BView *FindView(BPoint point);
	bool Lock();
	void Unlock();
	bool IsLocked();
*/
protected:
	void HandleSpace(color_space space, int32 BytesPerLine);
	uint8 *buffer;

	int32 width,height;
	int32 bytesperline;
	color_space cspace;
	int bpp;
	area_id area;
};

#endif