/*
 * Copyright 2001-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SERVER_CURSOR_H
#define SERVER_CURSOR_H


#include "ServerBitmap.h"

#include <Point.h>
#include <String.h>


class CursorManager;


class ServerCursor : public ServerBitmap {
public:
								ServerCursor(BRect r, color_space space,
									int32 flags, BPoint hotspot,
									int32 bytesPerRow = -1,
									screen_id screen = B_MAIN_SCREEN_ID);
								ServerCursor(const uint8* cursorDataFromR5);
								ServerCursor(const uint8* alreadyPaddedData,
									uint32 width, uint32 height,
									color_space format);
								ServerCursor(const ServerCursor* cursor);

	virtual						~ServerCursor();

	//! Returns the cursor's hot spot
			void				SetHotSpot(BPoint pt);
			BPoint				GetHotSpot() const
									{ return fHotSpot; }

			void				SetOwningTeam(team_id tid)
									{ fOwningTeam = tid; }
			team_id				OwningTeam() const
									{ return fOwningTeam; }

			int32				Token() const
									{ return fToken; }

			void				AttachedToManager(CursorManager* manager);

			const uint8*		CursorData() const
									{ return fCursorData; }

private:
	friend class CursorManager;

			BPoint				fHotSpot;
			team_id				fOwningTeam;
			uint8*				fCursorData;
			CursorManager*		fManager;
};


typedef BReference<ServerCursor> ServerCursorReference;


#endif	// SERVER_CURSOR_H
