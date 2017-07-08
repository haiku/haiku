/*
 * Copyright 2001-2007, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_BITMAP_H
#define	_BITMAP_H


#include <Archivable.h>
#include <InterfaceDefs.h>
#include <Rect.h>

class BView;
class BWindow;
namespace BPrivate {
	class BPrivateScreen;
}

enum {
	B_BITMAP_CLEAR_TO_WHITE				= 0x00000001,
	B_BITMAP_ACCEPTS_VIEWS				= 0x00000002,
	B_BITMAP_IS_AREA					= 0x00000004,
	B_BITMAP_IS_LOCKED					= 0x00000008 | B_BITMAP_IS_AREA,
	B_BITMAP_IS_CONTIGUOUS				= 0x00000010 | B_BITMAP_IS_LOCKED,
	B_BITMAP_IS_OFFSCREEN				= 0x00000020,
		// Offscreen but non-overlay bitmaps are not supported on Haiku,
		// but appearantly never were on BeOS either! The accelerant API
		// would need to be extended to so that the app_server can ask
		// the graphics driver to reserve memory for a bitmap and for this
		// to make any sense, an accelerated blit from this memory into
		// the framebuffer needs to be added to the API as well.
	B_BITMAP_WILL_OVERLAY				= 0x00000040 | B_BITMAP_IS_OFFSCREEN,
	B_BITMAP_RESERVE_OVERLAY_CHANNEL	= 0x00000080,

	// Haiku extensions:
	B_BITMAP_NO_SERVER_LINK				= 0x00000100,
		// Cheap to create, object will manage memory itself,
		// no BApplication needs to run, but one can't draw such
		// a BBitmap.
};

#define B_ANY_BYTES_PER_ROW	-1


class BBitmap : public BArchivable {
public:
								BBitmap(BRect bounds, uint32 flags,
									color_space colorSpace,
									int32 bytesPerRow = B_ANY_BYTES_PER_ROW,
									screen_id screenID = B_MAIN_SCREEN_ID);
								BBitmap(BRect bounds, color_space colorSpace,
									bool acceptsViews = false,
									bool needsContiguous = false);
								BBitmap(const BBitmap& source, uint32 flags);
								BBitmap(const BBitmap& source);
								BBitmap(const BBitmap* source,
									bool acceptsViews = false,
									bool needsContiguous = false);
	virtual						~BBitmap();

	// Archiving
								BBitmap(BMessage* data);
	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

			status_t			InitCheck() const;
			bool				IsValid() const;

			status_t			LockBits(uint32* state = NULL);
			void				UnlockBits();

			area_id				Area() const;
			void*				Bits() const;
			int32				BitsLength() const;
			int32				BytesPerRow() const;
			color_space			ColorSpace() const;
			BRect				Bounds() const;

			status_t			SetDrawingFlags(uint32 flags);
			uint32				Flags() const;

			void				SetBits(const void* data, int32 length,
									int32 offset, color_space colorSpace);

	// not part of the R5 API
			status_t			ImportBits(const void* data, int32 length,
									int32 bpr, int32 offset,
									color_space colorSpace);
			status_t			ImportBits(const void* data, int32 length,
									int32 bpr, color_space colorSpace,
									BPoint from, BPoint to, int32 width,
									int32 height);
			status_t			ImportBits(const BBitmap* bitmap);
			status_t			ImportBits(const BBitmap* bitmap, BPoint from,
									BPoint to, int32 width, int32 height);

			status_t			GetOverlayRestrictions(
									overlay_restrictions* restrictions) const;

	// to mimic a BWindow
	virtual	void				AddChild(BView* view);
	virtual	bool				RemoveChild(BView* view);
			int32				CountChildren() const;
			BView*				ChildAt(int32 index) const;
			BView*				FindView(const char* viewName) const;
			BView*				FindView(BPoint point) const;
			bool				Lock();
			void				Unlock();
			bool				IsLocked() const;

			BBitmap&			operator=(const BBitmap& source);

	class Private;
private:
	friend class BView;
	friend class BApplication;
	friend class ::BPrivate::BPrivateScreen;
	friend class Private;

	virtual	status_t			Perform(perform_code d, void* arg);
	virtual	void				_ReservedBitmap1();
	virtual	void				_ReservedBitmap2();
	virtual	void				_ReservedBitmap3();

			int32				_ServerToken() const;
			void				_InitObject(BRect bounds,
									color_space colorSpace, uint32 flags,
									int32 bytesPerRow, screen_id screenID);
			void				_CleanUp();
			void				_AssertPointer();

			void				_ReconnectToAppServer();

private:
			uint8*				fBasePointer;
			int32				fSize;
			color_space			fColorSpace;
			BRect				fBounds;
			int32				fBytesPerRow;
			BWindow*			fWindow;
			int32				fServerToken;
			int32				fAreaOffset;
			uint8				unused;
			area_id				fArea;
			area_id				fServerArea;
			uint32				fFlags;
			status_t			fInitError;
};

#endif	// _BITMAP_H
