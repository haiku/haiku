/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
#ifndef SHORTCUTS_SPEC_H
#define SHORTCUTS_SPEC_H


#include <Bitmap.h>

#include <ColumnListView.h>
#include "KeyInfos.h"


class CommandActuator;
class MetaKeyStateMap;


MetaKeyStateMap& GetNthKeyMap(int which);

/*
 * Objects of this class represent one hotkey "entry" in the preferences
 * ListView. Each ShortcutsSpec contains the info necessary to generate both
 * the proper GUI display, and the proper BitFieldTester and CommandActuator
 * object for the ShortcutsCatcher add-on to use.
 */
class ShortcutsSpec : public BRow, public BArchivable {
public:
	static	void			InitializeMetaMaps();

							ShortcutsSpec(const char* command);
							ShortcutsSpec(const ShortcutsSpec& copyMe);
							ShortcutsSpec(BMessage* from);
							~ShortcutsSpec();

	virtual	status_t		Archive(BMessage* into, bool deep = true) const;
	static	BArchivable*	Instantiate(BMessage* from);
	const	char* 			GetCellText(int whichColumn) const;
			void			SetCommand(const char* commandStr);

	// Returns the name of the Nth Column.
	static	const char*		GetColumnName(int index);

			// Update this spec's state in response to a keystroke to the given
			// column. Returns true iff a change occurred.
			bool 			ProcessColumnKeyStroke(int whichColumn,
								const char* bytes, int32 key);

			// Same as ProcessColumnKeyStroke, but for a mouse click instead.
			bool			ProcessColumnMouseClick(int whichColumn);
	
			// Same as ProcessColumnKeyStroke, but for a text string instead.
			bool			ProcessColumnTextString(int whichColumn,
								const char* string);

			int32 			GetSelectedColumn() const { return fSelectedColumn; }
			void 			SetSelectedColumn(int32 i) { fSelectedColumn = i; }

	// default layout of columns is set in here.
	enum {
		SHIFT_COLUMN_INDEX		= 0,
		CONTROL_COLUMN_INDEX	= 1,
		COMMAND_COLUMN_INDEX	= 2,
		OPTION_COLUMN_INDEX		= 3,
		NUM_META_COLUMNS		= 4, // shift, control, command, option, for now
		KEY_COLUMN_INDEX		= NUM_META_COLUMNS,
		STRING_COLUMN_INDEX		= 5
	};

private:
			void 			_CacheViewFont(BView* owner);
			bool 			_AttemptTabCompletion();

			char*			fCommand;
			uint32			fCommandLen;	// number of bytes in fCommand buffer
			uint32			fCommandNul;	// index of the NUL byte in fCommand

			// icon for associated program. Invalid if none available.
			BBitmap			fBitmap;

			char*			fLastBitmapName;
			bool			fBitmapValid;
			uint32			fKey;
			int32			fMetaCellStateIndex[NUM_META_COLUMNS];
			BPoint			fCursorPt1;
			BPoint			fCursorPt2;
			bool			fCursorPtsValid;
	mutable	char			fScratch[50];
			int32			fSelectedColumn;

private:
	static	void			_InitModifierNames();

	static	const char*		sShiftName;
	static	const char*		sControlName;
	static	const char*		sOptionName;
	static	const char*		sCommandName;
};


#endif	// SHORTCUTS_SPEC_H
