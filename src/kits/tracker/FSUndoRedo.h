/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/
#ifndef _FS_UNDO_REDO_H
#define _FS_UNDO_REDO_H


#include <Entry.h>
#include <ObjectList.h>


namespace BPrivate {

static const uint32_t kUndoOperation = 'U' << 24;
static const uint32_t kDoOperation = 'T' << 24;

class UndoItem;

class Undo {
	public:
		~Undo();
		void UpdateEntry(BEntry* entry, const char* destName);
		void Remove();

	protected:
		UndoItem* fUndo;
};


class MoveCopyUndo : public Undo {
	public:
		MoveCopyUndo(BObjectList<entry_ref>* sourceList, BDirectory &dest,
			BList* pointList, uint32 moveMode);
};


class NewFolderUndo : public Undo {
	public:
		NewFolderUndo(const entry_ref &ref);
};


class RenameUndo : public Undo {
	public:
		RenameUndo(BEntry &entry, const char* newName);
};


class RenameVolumeUndo : public Undo {
	public:
		RenameVolumeUndo(BVolume &volume, const char* newName);
};


static
inline bool FSIsUndoMoveMode(uint32 moveMode)
{
	return (moveMode & 0xFF000000) == kUndoOperation;
}


static
inline uint32 FSUndoMoveMode(uint32 moveMode)
{
	return (moveMode & 0x00FFFFFF) | kUndoOperation;
}


static
inline uint32 FSMoveMode(uint32 moveMode)
{
	return (moveMode & 0x00FFFFFF) | kDoOperation;
}


extern void FSUndo();
extern void FSRedo();

}	// namespace BPrivate

#endif	// _FS_UNDO_REDO_H
