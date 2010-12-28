/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 */
#ifndef CURSOR_SET_H
#define CURSOR_SET_H


#include <Bitmap.h>
#include <Cursor.h>
#include <Message.h>

#include <ServerProtocol.h>

class ServerCursor;


/*!
	\brief Class to manage system cursor sets
*/
class CursorSet : public BMessage {
	public:
		CursorSet(const char *name);

		status_t		Save(const char *path,int32 saveflags=0);
		status_t		Load(const char *path);
		status_t		AddCursor(BCursorID which,const BBitmap *cursor, const BPoint &hotspot);
		status_t		AddCursor(BCursorID which, uint8 *data);
		void			RemoveCursor(BCursorID which);
		status_t		FindCursor(BCursorID which, BBitmap **cursor, BPoint *hotspot);
		status_t		FindCursor(BCursorID which, ServerCursor **cursor) const;
		void			SetName(const char *name);
		const char		*GetName();

	private:
		const char *_CursorWhichToString(BCursorID which) const;
		BBitmap *_CursorDataToBitmap(uint8 *data);
};

#endif	// CURSOR_SET_H
