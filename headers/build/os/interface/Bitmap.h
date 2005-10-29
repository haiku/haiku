//------------------------------------------------------------------------------
//	Copyright (c) 2001-2005, Haiku, Inc.
//
//	Distributed under the terms of the MIT license.
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

enum {
	B_BITMAP_CLEAR_TO_WHITE				= 0x00000001,
	B_BITMAP_ACCEPTS_VIEWS				= 0x00000002,
	B_BITMAP_IS_AREA					= 0x00000004,
	B_BITMAP_IS_LOCKED					= 0x00000008 | B_BITMAP_IS_AREA,
	B_BITMAP_IS_CONTIGUOUS				= 0x00000010 | B_BITMAP_IS_LOCKED,
	B_BITMAP_IS_OFFSCREEN				= 0x00000020,
	B_BITMAP_WILL_OVERLAY				= 0x00000040 | B_BITMAP_IS_OFFSCREEN,
	B_BITMAP_RESERVE_OVERLAY_CHANNEL	= 0x00000080,
	B_BITMAP_NO_SERVER_LINK				= 0x00000100
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

//----- Private or reserved -----------------------------------------//
	
	virtual status_t Perform(perform_code d, void *arg);

private:
	virtual void _ReservedBitmap1();
	virtual void _ReservedBitmap2();
	virtual void _ReservedBitmap3();

	BBitmap(const BBitmap &);
	BBitmap &operator=(const BBitmap &);

	char *get_shared_pointer() const;
	int32 get_server_token() const;
	void InitObject(BRect bounds, color_space colorSpace, uint32 flags,
					int32 bytesPerRow, screen_id screenID);
	void CleanUp();

	void AssertPtr();

	void		*fBasePtr;
	int32		fSize;
	color_space	fColorSpace;
	BRect		fBounds;
	int32		fBytesPerRow;
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
