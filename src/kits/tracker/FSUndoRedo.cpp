#include "Commands.h"
#include "FSUndoRedo.h"
#include "FSUtils.h"

#include <Autolock.h>
#include <Volume.h>
#include <Node.h>
#include <Path.h>


static const int32 kUndoRedoListMaxCount = 20;


namespace BPrivate {

class UndoItem {
	public:
		virtual ~UndoItem() {}

		virtual status_t Undo() = 0;
		virtual status_t Redo() = 0;

		virtual void UpdateEntry(BEntry* /*entry*/, const char* /*name*/) {}
			// updates the name of the target from the source entry "entry"
};

static BObjectList<UndoItem> sUndoList, sRedoList;
static BLocker sLock("undo");

class UndoItemCopy : public UndoItem {
	public:
		UndoItemCopy(BObjectList<entry_ref>* sourceList, BDirectory &target,
			BList* pointList, uint32 moveMode);
		virtual ~UndoItemCopy();

		virtual status_t Undo();
		virtual status_t Redo();
		virtual void UpdateEntry(BEntry* entry, const char* name);

	private:
		BObjectList<entry_ref> fSourceList;
		BObjectList<entry_ref> fTargetList;
		entry_ref	fSourceRef, fTargetRef;
		uint32		fMoveMode;
};


class UndoItemMove : public UndoItem {
	public:
		/** source - list of file(s) that were moved.  Assumes ownership.
		 *	origfolder - location it was moved from
		 */
		UndoItemMove(BObjectList<entry_ref>* sourceList, BDirectory &target,
			BList* pointList);
		virtual ~UndoItemMove();

		virtual status_t Undo();
		virtual status_t Redo();

	private:
		BObjectList<entry_ref> fSourceList;
		entry_ref	fSourceRef, fTargetRef;
};


class UndoItemFolder : public UndoItem {
	public:
		UndoItemFolder(const entry_ref &ref);
			// ref - entry_ref indicating the folder created
		virtual ~UndoItemFolder();

		virtual status_t Undo();
		virtual status_t Redo();

	private:
		// this ref has two different meanings in the different states of
		// this object:
		//  - Undo() - fRef indicates the folder that was created via
		//    FSCreateNewFolderIn(...)
		//  - Redo() - fRef indicates the folder in which
		//    FSCreateNewFolderIn() should be performed
		entry_ref	fRef;
};


class UndoItemRename : public UndoItem {
	public:
		UndoItemRename(const entry_ref &origRef, const entry_ref &ref);
		UndoItemRename(const BEntry &entry, const char* newName);
		virtual ~UndoItemRename();

		virtual status_t Undo();
		virtual status_t Redo();

	private:
		entry_ref	fRef, fOrigRef;
};


class UndoItemRenameVolume : public UndoItem {
	public:
		UndoItemRenameVolume(BVolume &volume, const char* newName);
		virtual ~UndoItemRenameVolume();

		virtual status_t Undo();
		virtual status_t Redo();

	private:
		BVolume	fVolume;
		BString	fOldName, fNewName;
};


//--------------------------


static status_t
ChangeListSource(BObjectList<entry_ref> &list, BEntry &entry)
{
	node_ref source;
	if (entry.GetNodeRef(&source) != B_OK)
		return B_ERROR;

	for (int32 index = 0; index < list.CountItems(); index++) {
		entry_ref* ref = list.ItemAt(index);

		ref->device = source.device;
		ref->directory = source.node;
	}

	return B_OK;
}


static void
AddUndoItem(UndoItem* item)
{
	BAutolock locker(sLock);

	// we have a restricted number of possible undos
	if (sUndoList.CountItems() == kUndoRedoListMaxCount)
		sUndoList.RemoveItem(sUndoList.LastItem());

	sUndoList.AddItem(item, 0);
	sRedoList.MakeEmpty();
}


//	#pragma mark -


Undo::~Undo()
{
	if (fUndo != NULL)
		AddUndoItem(fUndo);
}


void
Undo::UpdateEntry(BEntry* entry, const char* destName)
{
	if (fUndo != NULL)
		fUndo->UpdateEntry(entry, destName);
}


void
Undo::Remove()
{
	delete fUndo;
	fUndo = NULL;
}


MoveCopyUndo::MoveCopyUndo(BObjectList<entry_ref>* sourceList,
	BDirectory &dest, BList* pointList, uint32 moveMode)
{
	if (moveMode == kMoveSelectionTo)
		fUndo = new UndoItemMove(sourceList, dest, pointList);
	else
		fUndo = new UndoItemCopy(sourceList, dest, pointList, moveMode);
}


NewFolderUndo::NewFolderUndo(const entry_ref &ref)
{
	fUndo = new UndoItemFolder(ref);
}


RenameUndo::RenameUndo(BEntry &entry, const char* newName)
{
	fUndo = new UndoItemRename(entry, newName);
}


RenameVolumeUndo::RenameVolumeUndo(BVolume &volume, const char* newName)
{
	fUndo = new UndoItemRenameVolume(volume, newName);
}


//	#pragma mark -


UndoItemCopy::UndoItemCopy(BObjectList<entry_ref>* sourceList,
	BDirectory &target, BList* /*pointList*/, uint32 moveMode)
	:
	fSourceList(*sourceList),
	fTargetList(*sourceList),
	fMoveMode(moveMode)
{
	BEntry entry(sourceList->ItemAt(0));

	BEntry sourceEntry;
	entry.GetParent(&sourceEntry);
	sourceEntry.GetRef(&fSourceRef);

	BEntry targetEntry;
	target.GetEntry(&targetEntry);
	targetEntry.GetRef(&fTargetRef);
	ChangeListSource(fTargetList, targetEntry);
}


UndoItemCopy::~UndoItemCopy()
{
}


status_t
UndoItemCopy::Undo()
{
	FSDeleteRefList(new BObjectList<entry_ref>(fTargetList), true, false);
	return B_OK;
}


status_t
UndoItemCopy::Redo()
{
	FSMoveToFolder(new BObjectList<entry_ref>(fSourceList),
		new BEntry(&fTargetRef), FSUndoMoveMode(fMoveMode), NULL);

	return B_OK;
}


void
UndoItemCopy::UpdateEntry(BEntry* entry, const char* name)
{
	entry_ref changedRef;
	if (entry->GetRef(&changedRef) != B_OK)
		return;

	for (int32 index = 0; index < fSourceList.CountItems(); index++) {
		entry_ref* ref = fSourceList.ItemAt(index);
		if (changedRef != *ref)
			continue;

		ref = fTargetList.ItemAt(index);
		ref->set_name(name);
	}
}


//	#pragma mark -


UndoItemMove::UndoItemMove(BObjectList<entry_ref>* sourceList,
	BDirectory &target, BList* /*pointList*/)
	:
	fSourceList(*sourceList)
{
	BEntry entry(sourceList->ItemAt(0));
	BEntry source;
	entry.GetParent(&source);
	source.GetRef(&fSourceRef);

	BEntry targetEntry;
	target.GetEntry(&targetEntry);
	targetEntry.GetRef(&fTargetRef);
}


UndoItemMove::~UndoItemMove()
{
}


status_t
UndoItemMove::Undo()
{
	BObjectList<entry_ref>* list = new BObjectList<entry_ref>(fSourceList);
	BEntry entry(&fTargetRef);
	ChangeListSource(*list, entry);

	// FSMoveToFolder() owns its arguments
	FSMoveToFolder(list, new BEntry(&fSourceRef),
		FSUndoMoveMode(kMoveSelectionTo), NULL);

	return B_OK;
}


status_t
UndoItemMove::Redo()
{
	// FSMoveToFolder() owns its arguments
	FSMoveToFolder(new BObjectList<entry_ref>(fSourceList),
		new BEntry(&fTargetRef), FSUndoMoveMode(kMoveSelectionTo), NULL);

	return B_OK;
}


//	#pragma mark -


UndoItemFolder::UndoItemFolder(const entry_ref &ref)
	:
	fRef(ref)
{
}


UndoItemFolder::~UndoItemFolder()
{
}


status_t
UndoItemFolder::Undo()
{
	FSDelete(new entry_ref(fRef), false, false);
	return B_OK;
}


status_t
UndoItemFolder::Redo()
{
	return FSCreateNewFolder(&fRef);
}


//	#pragma mark -


UndoItemRename::UndoItemRename(const entry_ref &origRef, const entry_ref &ref)
	:
	fRef(ref),
	fOrigRef(origRef)
{
}


UndoItemRename::UndoItemRename(const BEntry &entry, const char* newName)
{
	entry.GetRef(&fOrigRef);

	fRef = fOrigRef;
	fRef.set_name(newName);
}


UndoItemRename::~UndoItemRename()
{
}


status_t
UndoItemRename::Undo()
{
	BEntry entry(&fRef, false);
	return entry.Rename(fOrigRef.name);
}


status_t
UndoItemRename::Redo()
{
	BEntry entry(&fOrigRef, false);
	return entry.Rename(fRef.name);
}


//	#pragma mark -


UndoItemRenameVolume::UndoItemRenameVolume(BVolume &volume,
		const char* newName)
	:
	fVolume(volume),
	fNewName(newName)
{
	char* buffer = fOldName.LockBuffer(B_FILE_NAME_LENGTH);
	if (buffer != NULL) {
		fVolume.GetName(buffer);
		fOldName.UnlockBuffer();
	}
}


UndoItemRenameVolume::~UndoItemRenameVolume()
{
}


status_t
UndoItemRenameVolume::Undo()
{
	return fVolume.SetName(fOldName.String());
}


status_t
UndoItemRenameVolume::Redo()
{
	return fVolume.SetName(fNewName.String());
}


//	#pragma mark -


void
FSUndo()
{
	BAutolock locker(sLock);

	UndoItem* undoItem = sUndoList.FirstItem();
	if (undoItem == NULL)
		return;

	undoItem->Undo();
		// ToDo: evaluate return code

	sUndoList.RemoveItem(undoItem);

	if (sRedoList.CountItems() == kUndoRedoListMaxCount)
		sRedoList.RemoveItem(sRedoList.LastItem());

	sRedoList.AddItem(undoItem, 0);
}


void
FSRedo()
{
	BAutolock locker(sLock);

	UndoItem* undoItem = sRedoList.FirstItem();
	if (undoItem == NULL)
		return;

	undoItem->Redo();
		// ToDo: evaluate return code

	sRedoList.RemoveItem(undoItem);

	if (sUndoList.CountItems() == kUndoRedoListMaxCount)
		sUndoList.RemoveItem(sUndoList.LastItem());

	sUndoList.AddItem(undoItem, 0);
}

}	// namespace BPrivate
