//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Bitmap.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	BBitmap objects represent off-screen windows that
//					contain bitmap data.
//------------------------------------------------------------------------------

#ifndef	_BITMAP_H
#define	_BITMAP_H

#include <Archivable.h>
#include <InterfaceDefs.h>
#include <Rect.h>

class BWindow;

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

#define B_ANY_BYTES_PER_ROW	-1

//----------------------------------------------------------------//
//----- BBitmap class --------------------------------------------//

class BBitmap : public BArchivable {
public:
	BBitmap(BRect bounds, uint32 flags, color_space colorSpace,
			int32 bytesPerRow = B_ANY_BYTES_PER_ROW,
			screen_id screenID = B_MAIN_SCREEN_ID);
	BBitmap(BRect bounds, color_space colorSpace, bool acceptsViews = false,
			bool needsContiguous = false);
	BBitmap(const BBitmap *source, bool acceptsViews = false,
			bool needsContiguous = false);
	virtual ~BBitmap();

	// Archiving
	BBitmap(BMessage *data);
	static BArchivable *Instantiate(BMessage *data);
	virtual status_t Archive(BMessage *data, bool deep = true) const;

	status_t InitCheck() const;
	bool IsValid() const;

	status_t LockBits(uint32 *state = NULL);
	void UnlockBits();

	area_id Area() const;
	void *Bits() const;
	int32 BitsLength() const;
	int32 BytesPerRow() const;
	color_space ColorSpace() const;
	BRect Bounds() const;

	void SetBits(const void *data, int32 length, int32 offset,
				 color_space colorSpace);

	// not part of the R5 API
	status_t ImportBits(const void *data, int32 length, int32 bpr,
						int32 offset, color_space colorSpace);
	status_t ImportBits(const BBitmap *bitmap);

	status_t GetOverlayRestrictions(overlay_restrictions *restrictions) const;

	// to mimic a BWindow
	virtual void AddChild(BView *view);
	virtual bool RemoveChild(BView *view);
	int32 CountChildren() const;
	BView *ChildAt(int32 index) const;
	BView *FindView(const char *viewName) const;
	BView *FindView(BPoint point) const;
	bool Lock();
	void Unlock();
	bool IsLocked() const;

//----- Private or reserved -----------------------------------------//
	
	virtual status_t Perform(perform_code d, void *arg);

private:
	friend class BView;
	friend class BApplication;
	friend void _get_screen_bitmap_(BBitmap *, BRect, bool);

	virtual void _ReservedBitmap1();
	virtual void _ReservedBitmap2();
	virtual void _ReservedBitmap3();

	BBitmap(const BBitmap &);
	BBitmap &operator=(const BBitmap &);

	char *get_shared_pointer() const;
	int32 get_server_token() const;
	void InitObject(BRect bounds, color_space colorSpace, uint32 flags,
					int32 bytesPerRow, screen_id screenID);
	void AssertPtr();

	void		*fBasePtr;
	int32		fSize;
	color_space	fColorSpace;
	BRect		fBounds;
	int32		fBytesPerRow;
	BWindow		*fWindow;
	int32		fServerToken;
	int32		fToken;
	uint8		unused;
	area_id		fArea;
	area_id		fOrigArea;
	uint32		fFlags;
	status_t	fInitError;
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif	// _BITMAP_H
