/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold (bonefish@users.sf.net)
 */
#ifndef	_BITMAP_H
#define	_BITMAP_H


#include <Archivable.h>
#include <InterfaceDefs.h>
#include <Rect.h>

class BPrivateScreen;
class BWindow;

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
		status_t ImportBits(const void *data, int32 length, int32 bpr,
							color_space colorSpace, BPoint from, BPoint to,
							int32 width, int32 height);
		status_t ImportBits(const BBitmap *bitmap);
		status_t ImportBits(const BBitmap *bitmap, BPoint from, BPoint to,
							int32 width, int32 height);

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
		friend class BPrivateScreen;

		virtual void _ReservedBitmap1();
		virtual void _ReservedBitmap2();
		virtual void _ReservedBitmap3();

		BBitmap(const BBitmap &);
		BBitmap &operator=(const BBitmap &);

		int32 _ServerToken() const;
		void _InitObject(BRect bounds, color_space colorSpace, uint32 flags,
						int32 bytesPerRow, screen_id screenID);
		void _CleanUp();
		void _AssertPointer();

		uint8		*fBasePointer;
		int32		fSize;
		color_space	fColorSpace;
		BRect		fBounds;
		int32		fBytesPerRow;
		BWindow		*fWindow;
		int32		fServerToken;
		int32		fAreaOffset;
		uint8		unused;
		area_id		fArea;
		area_id		fServerArea;
		uint32		fFlags;
		status_t	fInitError;
};

#endif	// _BITMAP_H
