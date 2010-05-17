/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
#ifndef ShortcutsSpec_h
#define ShortcutsSpec_h


#include <Bitmap.h>

#include "CLVListItem.h"
#include "KeyInfos.h"


class CommandActuator;
class MetaKeyStateMap;


MetaKeyStateMap & GetNthKeyMap(int which);

/* Objects of this class represent one hotkey "entry" in the preferences 
 * ListView. Each ShortcutsSpec contains the info necessary to generate both 
 * the proper GUI display, and the proper BitFieldTester and CommandActuator 
 * object for the ShortcutsCatcher add-on to use.
 */
class ShortcutsSpec : public CLVListItem {
public:
	static	void			InitializeMetaMaps();

							ShortcutsSpec(const char* command);
							ShortcutsSpec(const ShortcutsSpec& copyMe);
							ShortcutsSpec(BMessage* from);
							~ShortcutsSpec();

	virtual	status_t		Archive(BMessage* into, bool deep = true) const;
	virtual void 			Pulse(BView* owner);
	static 	BArchivable*	Instantiate(BMessage* from);										
			void			Update(BView* owner, const BFont* font);	
	const 	char* 			GetCellText(int whichColumn) const;
			void			SetCommand(const char* commandStr);
			
	virtual	void			DrawItemColumn(BView* owner, BRect item_column_rect,
								int32 column_index, bool columnSelected,
								bool complete);
	
	static	int				MyCompare(const CLVListItem* a_Item1, 
								const CLVListItem* a_Item2, int32 KeyColumn);

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

			int32 			GetSelectedColumn() const {return fSelectedColumn;}
			void 			SetSelectedColumn(int32 i) {fSelectedColumn = i;}

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
			
			// call this to ensure the icon is up-to-date
			void 			_UpdateIconBitmap();

			char*			fCommand;
			uint32			fCommandLen; // number of bytes in fCommand buffer
			uint32			fCommandNul; // index of the NUL byte in fCommand
			float			fTextOffset;		
			
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

#endif

