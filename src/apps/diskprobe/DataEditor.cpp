/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DataEditor.h"

#include <Autolock.h>
#include <NodeMonitor.h>
#include <Drivers.h>
#include <fs_attr.h>

#include <string.h>
#include <unistd.h>


#ifdef TRACE
#	undef TRACE
#endif

//#define TRACE_DATA_EDITOR
#ifdef TRACE_DATA_EDITOR
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif


class StateWatcher {
	public:
		StateWatcher(DataEditor &editor);
		~StateWatcher();

	private:
		DataEditor	&fEditor;
		bool		fCouldUndo;
		bool		fCouldRedo;
		bool		fWasModified;
};

class DataChange {
	public:
		virtual ~DataChange();

		virtual void Apply(off_t offset, uint8 *buffer, size_t size) = 0;
		virtual void Revert(off_t offset, uint8 *buffer, size_t size) = 0;
		virtual void GetRange(off_t fileSize, off_t &_offset, off_t &_size) = 0;
};

class ReplaceChange : public DataChange {
	public:
		ReplaceChange(off_t offset, const uint8 *data, size_t size);
		~ReplaceChange();

		virtual void Apply(off_t offset, uint8 *buffer, size_t size);
		virtual void Revert(off_t offset, uint8 *buffer, size_t size);
		virtual void GetRange(off_t fileSize, off_t &_offset, off_t &_size);

	private:
		void Normalize(off_t bufferOffset, size_t bufferSize,
			off_t &offset, size_t &dataOffset, size_t &size);

		uint8 	*fNewData;
		uint8 	*fOldData;
		size_t	fSize;
		off_t	fOffset;
};


//---------------------


#ifdef TRACE_DATA_EDITOR

#define DUMPED_BLOCK_SIZE 16

void
dump_block(const uint8 *buffer, int size, const char *prefix)
{
	for (int i = 0; i < size;) {
		int start = i;

		printf(prefix);
		for (; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x", *(unsigned char *)(buffer + i));
		}
		printf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					printf(".");
				else
					printf("%c", c);
			} else
				break;
		}
		printf("\n");
	}
}
#endif	// TRACE_DATA_EDITOR


static int
CompareOffsets(const off_t *a, const off_t *b)
{
	if (*a < *b)
		return -1;
	else if (*a > *b)
		return 1;

	return 0;
}


//	#pragma mark -


StateWatcher::StateWatcher(DataEditor &editor)
	:
	fEditor(editor)
{
	fCouldUndo = editor.CanUndo();
	fCouldRedo = editor.CanRedo();
	fWasModified = editor.IsModified();
}


StateWatcher::~StateWatcher()
{
	BMessage update;
	if (fCouldRedo != fEditor.CanRedo())
		update.AddBool("can_redo", fEditor.CanRedo());
	if (fCouldUndo != fEditor.CanUndo())
		update.AddBool("can_undo", fEditor.CanUndo());
	if (fWasModified != fEditor.IsModified())
		update.AddBool("modified", fEditor.IsModified());

	// only send an update if we have to report anything
	if (!update.IsEmpty())
		fEditor.SendNotices(kMsgDataEditorStateChange, &update);
}


//	#pragma mark -


DataChange::~DataChange()
{
}


//	#pragma mark -


ReplaceChange::ReplaceChange(off_t offset, const uint8 *data, size_t size)
{
	fOffset = offset;

	fNewData = (uint8 *)malloc(size);
	fOldData = (uint8 *)malloc(size);
	if (fNewData != NULL && fOldData != NULL) {
		memcpy(fNewData, data, size);
		fSize = size;		
	} else
		fSize = 0;
}


ReplaceChange::~ReplaceChange()
{
	free(fNewData);
	free(fOldData);
}


/** Normalizes the supplied offset & size pair to be limited by
 *	the buffer offset and size.
 *	All parameters must have been initialized before calling this
 *	method.
 */

void 
ReplaceChange::Normalize(off_t bufferOffset, size_t bufferSize, off_t &offset,
	size_t &dataOffset, size_t &size)
{
	if (fOffset < bufferOffset) {
		offset = bufferOffset;
		dataOffset = bufferOffset - fOffset;
		size -= dataOffset;
	}

	if (offset + size > bufferOffset + bufferSize)
		size = bufferOffset + bufferSize - offset;
}


void 
ReplaceChange::Apply(off_t bufferOffset, uint8 *buffer, size_t bufferSize)
{
	// is it in our range?
	if (fOffset > bufferOffset + bufferSize || fOffset + fSize < bufferOffset)
		return;

	// don't change anything outside the supplied buffer
	off_t offset = fOffset;
	size_t dataOffset = 0;
	size_t size = fSize;
	Normalize(bufferOffset, bufferSize, offset, dataOffset, size);
	if (size == 0)
		return;

#if TRACE_DATA_EDITOR
	printf("Apply %p (buffer offset = %Ld):\n", this, bufferOffset);
	dumpBlock(buffer + offset - bufferOffset, size, "old:");
	dumpBlock(fNewData + dataOffset, size, "new:");
#endif

	// now we can safely exchange the buffer!
	memcpy(fOldData + dataOffset, buffer + offset - bufferOffset, size);
	memcpy(buffer + offset - bufferOffset, fNewData + dataOffset, size);
}


void 
ReplaceChange::Revert(off_t bufferOffset, uint8 *buffer, size_t bufferSize)
{
	// is it in our range?
	if (fOffset - bufferOffset > bufferSize || fOffset + fSize < bufferOffset)
		return;

	// don't change anything outside the supplied buffer
	off_t offset = fOffset;
	size_t dataOffset = 0;
	size_t size = fSize;
	Normalize(bufferOffset, bufferSize, offset, dataOffset, size);
	if (size == 0)
		return;

#ifdef TRACE_DATA_EDITOR
	printf("Revert %p (buffer offset = %Ld):\n", this, bufferOffset);
	dumpBlock(buffer + offset - bufferOffset, size, "old:");
	dumpBlock(fOldData + dataOffset, size, "new:");
#endif

	// now we can safely revert the buffer!
	memcpy(buffer + offset - bufferOffset, fOldData + dataOffset, size);
}


void 
ReplaceChange::GetRange(off_t /*fileSize*/, off_t &_offset, off_t &_size)
{
	_offset = fOffset;
	_size = fSize;
}


//	#pragma mark -


DataEditor::DataEditor()
	: BLocker("data view")
{
}


DataEditor::DataEditor(entry_ref &ref, const char *attribute)
	: BLocker("data view")
{
	SetTo(ref, attribute);
}


DataEditor::DataEditor(BEntry &entry, const char *attribute)
	: BLocker("data view")
{
	SetTo(entry, attribute);
}


DataEditor::DataEditor(const DataEditor &editor)
	: BLocker("data view")
{
}


DataEditor::~DataEditor()
{
}


status_t 
DataEditor::SetTo(const char *path, const char *attribute)
{
	BEntry entry(path);
	return SetTo(entry, attribute);
}


status_t 
DataEditor::SetTo(entry_ref &ref, const char *attribute)
{
	BEntry entry(&ref);
	return SetTo(entry, attribute);
}


status_t 
DataEditor::SetTo(BEntry &entry, const char *attribute)
{
	status_t status = fFile.SetTo(&entry, B_READ_WRITE);
	if (status < B_OK) {
		// try to open read only
		status = fFile.SetTo(&entry, B_READ_ONLY);
		if (status < B_OK)
			return status;

		fIsReadOnly = true;
	}

	entry.GetRef(&fRef);

	struct stat stat;
	stat.st_mode = 0;

	fFile.GetStat(&stat);
	fIsDevice = (stat.st_mode & (S_IFBLK | S_IFCHR)) != 0;

	if (attribute != NULL)
		fAttribute = strdup(attribute);
	else
		fAttribute = NULL;

	fBlockSize = 512;

	if (IsAttribute()) {
		attr_info info;
		status = fFile.GetAttrInfo(fAttribute, &info);
		if (status != B_OK) {
			fFile.Unset();
			return status;
		}

		fSize = info.size;
		fType = info.type;
	} else if (fIsDevice) {
		// ToDo: is there any other possibility to issue a ioctl() from a BFile?
		device_geometry geometry;
		int device = fFile.Dup();
		if (device < 0 || ioctl(device, B_GET_GEOMETRY, &geometry) < 0) {
			if (device >= 0)
				close(device);
			fFile.Unset();
			return B_ERROR;
		}
		close(device);

		fSize = 1LL * geometry.head_count * geometry.cylinder_count
			* geometry.sectors_per_track * geometry.bytes_per_sector;
		fBlockSize = geometry.bytes_per_sector;
	} else {
		status = fFile.GetSize(&fSize);
		if (status < B_OK) {
			fFile.Unset();
			return status;
		}
	}

	fLastChange = fFirstChange = NULL;
	fChangesFromSaved = 0;
	fView = NULL;
	fRealViewOffset = 0;
	fViewOffset = 0;
	fRealViewSize = fViewSize = fBlockSize;
	fNeedsUpdate = true;

	return B_OK;
}


status_t 
DataEditor::InitCheck()
{
	return fFile.InitCheck();
}


void
DataEditor::AddChange(DataChange *change)
{
	if (change == NULL)
		return;

	StateWatcher watcher(*this);
		// update state observers

	RemoveRedos();

	fChanges.AddItem(change);
	fLastChange = change;
	fChangesFromSaved++;

	fLastChange->Apply(fRealViewOffset, fView, fRealViewSize);
	// ToDo: try to join changes

	// update observers

	SendNotices(fLastChange);
}


status_t
DataEditor::Replace(off_t offset, const uint8 *data, size_t length)
{
	BAutolock locker(this);

	if (offset >= fSize)
		return B_BAD_VALUE;
	if (offset + length > fSize)
		length = fSize - offset;

	if (fNeedsUpdate) {
		status_t status = Update();
		if (status < B_OK)
			return status;
	}

	ReplaceChange *change = new ReplaceChange(offset, data, length);
	AddChange(change);

	return B_OK;
}


status_t
DataEditor::Remove(off_t offset, off_t length)
{
	BAutolock locker(this);

	// not yet implemented
	// ToDo: this needs some changes to the whole change mechanism

	return B_ERROR;
}


status_t
DataEditor::Insert(off_t offset, const uint8 *text, size_t length)
{
	BAutolock locker(this);

	// not yet implemented
	// ToDo: this needs some changes to the whole change mechanism

	return B_ERROR;
}


void
DataEditor::ApplyChanges()
{
	if (fLastChange == NULL && fFirstChange == NULL)
		return;

	int32 firstIndex = fFirstChange != NULL ? fChanges.IndexOf(fFirstChange) + 1 : 0;
	int32 lastIndex = fChanges.IndexOf(fLastChange);

	if (fChangesFromSaved >= 0) {
		// ascend changes
		TRACE(("ApplyChanges(): ascend from %ld to %ld\n", firstIndex, lastIndex));

		for (int32 i = firstIndex; i <= lastIndex; i++) {
			DataChange *change = fChanges.ItemAt(i);
			change->Apply(fRealViewOffset, fView, fRealViewSize);
		}
	} else {
		// descend changes
		TRACE(("ApplyChanges(): descend from %ld to %ld\n", firstIndex - 1, lastIndex));

		for (int32 i = firstIndex - 1; i > lastIndex; i--) {
			DataChange *change = fChanges.ItemAt(i);
			change->Revert(fRealViewOffset, fView, fRealViewSize);
		}
	}
}


status_t
DataEditor::Save()
{
	BAutolock locker(this);

	if (!IsModified())
		return B_OK;

	StateWatcher watcher(*this);

	// Do we need to ascend or descend the list of changes?

	int32 firstIndex = fFirstChange != NULL ? fChanges.IndexOf(fFirstChange) + 1 : 0;
	int32 lastIndex = fChanges.IndexOf(fLastChange);
	if (fChangesFromSaved < 0 && firstIndex != lastIndex) {
		// swap indices
		ASSERT(firstIndex > lastIndex);

		int32 temp = firstIndex - 1;
		firstIndex = lastIndex;
		lastIndex = temp;
	}

	if (firstIndex < 0)
		firstIndex = 0;
	if (lastIndex > fChanges.CountItems() - 1)
		lastIndex = fChanges.CountItems();

	// Collect ranges of data we need to write.

	// ToDo: This is a very simple implementation and could be drastically
	// improved by having items that save ranges, not just offsets. If Insert()
	// and Remove() are going to be implemented, it should be improved that
	// way to reduce the memory footprint to something acceptable.

	BObjectList<off_t> list;
	for (int32 i = firstIndex; i <= lastIndex; i++) {
		DataChange *change = fChanges.ItemAt(i);

		off_t offset, size;
		change->GetRange(FileSize(), offset, size);
		offset -= offset % BlockSize();

		while (size > 0) {
			list.BinaryInsertCopyUnique(offset, CompareOffsets);
			offset += BlockSize();
			size -= BlockSize();
		}
	}

	// read in data and apply changes, write it back to disk

	off_t oldOffset = fViewOffset;

	for (int32 i = 0; i < list.CountItems(); i++) {
		off_t offset = *list.ItemAt(i);

		// load the data into our view
		SetViewOffset(offset, false);

		if (fNeedsUpdate) {
			status_t status = Update();
			if (status < B_OK)
				return status;
		}

		// save back to disk

		// don't try to change the file size
		size_t size = fRealViewSize;
		if (fRealViewOffset + fRealViewSize > fSize)
			size = fSize - fRealViewOffset;

		ssize_t bytesWritten;
		if (IsAttribute())
			bytesWritten = fFile.WriteAttr(fAttribute, fType, fRealViewOffset, fView, fRealViewSize);
		else
			bytesWritten = fFile.WriteAt(fRealViewOffset, fView, fRealViewSize);

		if (bytesWritten < B_OK)
			return bytesWritten;
	}

	// update state

	SetViewOffset(oldOffset, false);
	if (fNeedsUpdate)
		Update();

	fChangesFromSaved = 0;
	fFirstChange = fLastChange;

	return B_OK;
}


/** This method will be called by DataEditor::AddChange()
 *	immediately before a change is applied.
 *	It removes all pending redo nodes from the list that would
 *	come after the current change.
 */

void
DataEditor::RemoveRedos()
{
	if (fLastChange == NULL)
		return;

	int32 start = fChanges.IndexOf(fLastChange) + 1;

	for (int32 i = fChanges.CountItems(); i-- > start; ) {
		DataChange *change = fChanges.RemoveItemAt(i);
		delete change;
	}
}


status_t 
DataEditor::Undo()
{
	BAutolock locker(this);

	if (!CanUndo())
		return B_ERROR;

	StateWatcher watcher(*this);
		// update state observers

	DataChange *undoChange = fLastChange;

	int32 index = fChanges.IndexOf(undoChange);
	fChangesFromSaved--;
	undoChange->Revert(fRealViewOffset, fView, fRealViewSize);

	if (index > 0)
		fLastChange = fChanges.ItemAt(index - 1);
	else
		fLastChange = NULL;

	// update observers
	SendNotices(undoChange);

	return B_OK;
}


status_t 
DataEditor::Redo()
{
	BAutolock locker(this);

	if (!CanRedo())
		return B_ERROR;

	StateWatcher watcher(*this);
		// update state observers

	int32 index = fChanges.IndexOf(fLastChange);
	fLastChange = fChanges.ItemAt(index + 1);
	fChangesFromSaved++;

	fLastChange->Apply(fRealViewOffset, fView, fRealViewSize);

	// update observers
	SendNotices(fLastChange);

	return B_OK;
}


bool 
DataEditor::CanUndo() const
{
	return fLastChange != NULL;
}


bool 
DataEditor::CanRedo() const
{
	return fChanges.IndexOf(fLastChange) < fChanges.CountItems() - 1;
}


status_t 
DataEditor::SetFileSize(off_t size)
{
	fSize = size;
	return B_OK;
}


status_t
DataEditor::SetViewOffset(off_t offset, bool sendNotices)
{
	if (fView == NULL) {
		status_t status = SetViewSize(fViewSize);
		if (status < B_OK)
			return status;
	}

	if (offset < 0 || offset > fSize)
		return B_BAD_VALUE;

	if (offset == fViewOffset)
		return B_OK;

	fRealViewOffset = (offset / fBlockSize) * fBlockSize;
	fViewOffset = offset;
	fNeedsUpdate = true;

	if (sendNotices)
		SendNotices(kMsgDataEditorOffsetChange);

	return B_OK;
}


status_t
DataEditor::SetViewOffset(off_t offset)
{
	return SetViewOffset(offset, true);
}


status_t 
DataEditor::SetViewSize(size_t size)
{
	size_t realSize = (size + fBlockSize - 1) & ~(fBlockSize - 1);
		// round to the next multiple of block size

	if (realSize == fRealViewSize && fView != NULL) {
		fViewSize = size;
		return B_OK;
	}
	if (realSize == 0)
		return B_BAD_VALUE;

	uint8 *view = (uint8 *)realloc(fView, realSize);
	if (view == NULL)
		return B_NO_MEMORY;

	fView = view;
	fRealViewSize = realSize;
	fViewSize = size;
	fNeedsUpdate = true;

	return B_OK;
}


void 
DataEditor::SetBlockSize(size_t size)
{
	fBlockSize = size;
}


status_t 
DataEditor::Update()
{
	ssize_t bytesRead;
	if (IsAttribute())
		bytesRead = fFile.ReadAttr(fAttribute, fType, fRealViewOffset, fView, fRealViewSize);
	else
		bytesRead = fFile.ReadAt(fRealViewOffset, fView, fRealViewSize);

	if (bytesRead < B_OK)
		return bytesRead;

	ApplyChanges();
	fNeedsUpdate = false;

	return B_OK;
}


status_t
DataEditor::UpdateIfNeeded(bool *_updated = NULL)
{
	if (!fNeedsUpdate) {
		if (_updated)
			*_updated = false;
		return B_OK;
	}

	status_t status = B_OK;

	if (fView == NULL)
		status = SetViewOffset(fViewOffset);

	if (status == B_OK && fNeedsUpdate) {
		status = Update();
		if (status == B_OK && _updated)
			*_updated = true;
	}

	return status;
}


status_t
DataEditor::GetViewBuffer(const uint8 **_buffer)
{
	if (!IsLocked())
		debugger("DataEditor: view not locked");

	status_t status = UpdateIfNeeded();
	if (status < B_OK)
		return status;

	*_buffer = fView + fViewOffset - fRealViewOffset;
	return B_OK;
}


void 
DataEditor::SendNotices(DataChange *change)
{
	off_t offset, size;
	change->GetRange(FileSize(), offset, size);

	// update observer
	BMessage update;
	update.AddInt64("offset", offset);
	update.AddInt64("size", size);
	SendNotices(kMsgDataEditorUpdate, &update);
}


void 
DataEditor::SendNotices(uint32 what, BMessage *message)
{
	if (fObservers.CountItems() == 0)
		return;

	BMessage *notice;
	if (message) {
		notice = new BMessage(*message);
		notice->what = what;
	} else
		notice = new BMessage(what);

	for (int32 i = fObservers.CountItems(); i-- > 0;) {
		BMessenger *messenger = fObservers.ItemAt(i);
		messenger->SendMessage(notice);
	}

	delete notice;
}


status_t 
DataEditor::StartWatching(BMessenger target)
{
	BAutolock locker(this);

	node_ref node;
	status_t status = fFile.GetNodeRef(&node);
	if (status < B_OK)
		return status;

	fObservers.AddItem(new BMessenger(target));

	return watch_node(&node, B_WATCH_STAT | B_WATCH_ATTR, target);
}


status_t
DataEditor::StartWatching(BHandler *handler, BLooper *looper)
{
	return StartWatching(BMessenger(handler, looper));
}


void 
DataEditor::StopWatching(BMessenger target)
{
	BAutolock locker(this);

	for (int32 i = fObservers.CountItems(); i-- > 0;) {
		BMessenger *messenger = fObservers.ItemAt(i);
		if (*messenger == target) {
			fObservers.RemoveItemAt(i);
			delete messenger;
			break;
		}
	}

	stop_watching(target);
}


void 
DataEditor::StopWatching(BHandler *handler, BLooper *looper)
{
	StopWatching(BMessenger(handler, looper));
}

