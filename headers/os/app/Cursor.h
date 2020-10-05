/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CURSOR_H
#define _CURSOR_H


#include <Archivable.h>
#include <InterfaceDefs.h>


enum BCursorID {
	B_CURSOR_ID_SYSTEM_DEFAULT					= 1,

	B_CURSOR_ID_CONTEXT_MENU					= 3,
	B_CURSOR_ID_COPY							= 4,
	B_CURSOR_ID_CREATE_LINK						= 29,
	B_CURSOR_ID_CROSS_HAIR						= 5,
	B_CURSOR_ID_FOLLOW_LINK						= 6,
	B_CURSOR_ID_GRAB							= 7,
	B_CURSOR_ID_GRABBING						= 8,
	B_CURSOR_ID_HELP							= 9,
	B_CURSOR_ID_I_BEAM							= 2,
	B_CURSOR_ID_I_BEAM_HORIZONTAL				= 10,
	B_CURSOR_ID_MOVE							= 11,
	B_CURSOR_ID_NO_CURSOR						= 12,
	B_CURSOR_ID_NOT_ALLOWED						= 13,
	B_CURSOR_ID_PROGRESS						= 14,
	B_CURSOR_ID_RESIZE_NORTH					= 15,
	B_CURSOR_ID_RESIZE_EAST						= 16,
	B_CURSOR_ID_RESIZE_SOUTH					= 17,
	B_CURSOR_ID_RESIZE_WEST						= 18,
	B_CURSOR_ID_RESIZE_NORTH_EAST				= 19,
	B_CURSOR_ID_RESIZE_NORTH_WEST				= 20,
	B_CURSOR_ID_RESIZE_SOUTH_EAST				= 21,
	B_CURSOR_ID_RESIZE_SOUTH_WEST				= 22,
	B_CURSOR_ID_RESIZE_NORTH_SOUTH				= 23,
	B_CURSOR_ID_RESIZE_EAST_WEST				= 24,
	B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST	= 25,
	B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST	= 26,
	B_CURSOR_ID_ZOOM_IN							= 27,
	B_CURSOR_ID_ZOOM_OUT						= 28
};


class BCursor : BArchivable {
public:
								BCursor(const void* cursorData);
								BCursor(const BCursor& other);
								BCursor(BCursorID id);
								BCursor(BMessage* data);
								BCursor(const BBitmap* bitmap,
									const BPoint& hotspot);
	virtual	~BCursor();

			status_t			InitCheck() const;

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

			BCursor&			operator=(const BCursor& other);
			bool				operator==(const BCursor& other) const;
			bool				operator!=(const BCursor& other) const;

private:
	virtual	status_t			Perform(perform_code d, void* arg);

	virtual	void				_ReservedCursor1();
	virtual	void				_ReservedCursor2();
	virtual	void				_ReservedCursor3();
	virtual	void				_ReservedCursor4();

			void				_FreeCursorData();

private:
	friend class BApplication;
	friend class BView;

			int32				fServerToken;
			bool				fNeedToFree;

			bool				_reservedWasPendingViewCursor;
				// Probably bogus because of padding.
			uint32				_reserved[6];
};

#endif	// _CURSOR_H
