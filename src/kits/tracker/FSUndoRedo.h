#ifndef _FS_UNDO_REDO_H
#define _FS_UNDO_REDO_H


#include "ObjectList.h"
#include <Entry.h>


namespace BPrivate {

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
	return (moveMode & '\xff\0\0\0') == 'U\0\0\0';
}


static
inline uint32 FSUndoMoveMode(uint32 moveMode)
{
	return (moveMode & ~'\xff\0\0\0') | 'U\0\0\0';
}


static
inline uint32 FSMoveMode(uint32 moveMode)
{
	return (moveMode & ~'\xff\0\0\0') | 'T\0\0\0';
}


extern void FSUndo();
extern void FSRedo();

}	// namespace BPrivate

#endif	// _FS_UNDO_REDO_H
