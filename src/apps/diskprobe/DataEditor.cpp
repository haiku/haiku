/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DataEditor.h"

#include <Autolock.h>
#include <NodeMonitor.h>
#include <Debug.h>
#include <Directory.h>
#include <Drivers.h>
#include <fs_attr.h>
#include <fs_info.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>


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
		virtual bool Merge(DataChange *change);
		virtual void GetRange(off_t fileSize, off_t &_offset, off_t &_size) = 0;
};

class ReplaceChange : public DataChange {
	public:
		ReplaceChange(off_t offset, const uint8 *data, size_t size);
		~ReplaceChange();

		virtual void Apply(off_t offset, uint8 *buffer, size_t size);
		virtual void Revert(off_t offset, uint8 *buffer, size_t size);
		virtual bool Merge(DataChange *change);
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

static void
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
CompareCaseInsensitive(const uint8 *a, const uint8 *b, size_t size)
{
	for (size_t i = 0; i < size; i++) {
		uint8 diff = tolower(a[i]) - tolower(b[i]);
		if (diff)
			return diff;
	}

	return 0;
}


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


bool
DataChange::Merge(DataChange *change)
{
	return false;
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
	if (fOffset > (bufferOffset + (off_t)bufferSize)
		|| (fOffset + (off_t)fSize) < bufferOffset) {
		return;
	}

	// don't change anything outside the supplied buffer
	off_t offset = fOffset;
	size_t dataOffset = 0;
	size_t size = fSize;
	Normalize(bufferOffset, bufferSize, offset, dataOffset, size);
	if (size == 0)
		return;

#ifdef TRACE_DATA_EDITOR
	printf("Apply %p (buffer offset = %Ld):\n", this, bufferOffset);
	dump_block(buffer + offset - bufferOffset, size, "old:");
	dump_block(fNewData + dataOffset, size, "new:");
#endif

	// now we can safely exchange the buffer!
	memcpy(fOldData + dataOffset, buffer + offset - bufferOffset, size);
	memcpy(buffer + offset - bufferOffset, fNewData + dataOffset, size);
}


void
ReplaceChange::Revert(off_t bufferOffset, uint8 *buffer, size_t bufferSize)
{
	// is it in our range?
	if (fOffset - bufferOffset > (off_t)bufferSize
		|| fOffset + (off_t)fSize < bufferOffset) {
		return;
	}

	// don't change anything outside the supplied buffer
	off_t offset = fOffset;
	size_t dataOffset = 0;
	size_t size = fSize;
	Normalize(bufferOffset, bufferSize, offset, dataOffset, size);
	if (size == 0)
		return;

#ifdef TRACE_DATA_EDITOR
	printf("Revert %p (buffer offset = %Ld):\n", this, bufferOffset);
	dump_block(buffer + offset - bufferOffset, size, "old:");
	dump_block(fOldData + dataOffset, size, "new:");
#endif

	// now we can safely revert the buffer!
	memcpy(buffer + offset - bufferOffset, fOldData + dataOffset, size);
}


bool
ReplaceChange::Merge(DataChange *_change)
{
	ReplaceChange *change = dynamic_cast<ReplaceChange *>(_change);
	if (change == NULL)
		return false;

	if (change->fOffset + change->fSize == fOffset + fSize
		&& change->fSize == 1) {
		// this is a special case - the new change changed the last byte of
		// the old change: we do this since the same byte is changed twice
		// in hex mode editing.
		fNewData[fSize - 1] = change->fNewData[0];
#ifdef TRACE_DATA_EDITOR
		printf("Merge one byte %p (offset = %Ld, size = %lu):\n", this, fOffset,
			fSize);
		dump_block(fOldData, fSize, "old:");
		dump_block(fNewData, fSize, "new:");
#endif
		return true;
	}

	// are the changes adjacent?

	if (change->fOffset + (off_t)change->fSize != fOffset
		&& fOffset + (off_t)fSize != change->fOffset)
		return false;

	// okay, we can try to merge the two changes

	uint8 *newData = fOffset < change->fOffset ? fNewData : change->fNewData;
	size_t size = fSize + change->fSize;

	if ((newData = (uint8 *)realloc(newData, size)) == NULL)
		return false;

	uint8 *oldData = fOffset < change->fOffset ? fOldData : change->fOldData;
	if ((oldData = (uint8 *)realloc(oldData, size)) == NULL) {
		fNewData = (uint8 *)realloc(newData, fSize);
			// if this fails, we can't do anything about it
		return false;
	}

	if (fOffset < change->fOffset) {
		memcpy(newData + fSize, change->fNewData, change->fSize);
		memcpy(oldData + fSize, change->fOldData, change->fSize);
	} else {
		memcpy(newData + change->fSize, fNewData, fSize);
		memcpy(oldData + change->fSize, fOldData, fSize);
		change->fNewData = fNewData;
		change->fOldData = fOldData;
			// transfer ownership, so that they will be deleted with the change
		fOffset = change->fOffset;
	}

	fNewData = newData;
	fOldData = oldData;
	fSize = size;

#ifdef TRACE_DATA_EDITOR
	printf("Merge %p (offset = %Ld, size = %lu):\n", this, fOffset, fSize);
	dump_block(fOldData, fSize, "old:");
	dump_block(fNewData, fSize, "new:");
#endif

	return true;
}


void
ReplaceChange::GetRange(off_t /*fileSize*/, off_t &_offset, off_t &_size)
{
	_offset = fOffset;
	_size = fSize;
}


//	#pragma mark -


DataEditor::DataEditor()
	:
	BLocker("data view"),
	fAttribute(NULL)
{
}


DataEditor::DataEditor(entry_ref &ref, const char *attribute)
	:
	BLocker("data view"),
	fAttribute(NULL)
{
	SetTo(ref, attribute);
}


DataEditor::DataEditor(BEntry &entry, const char *attribute)
	:
	BLocker("data view"),
	fAttribute(NULL)
{
	SetTo(entry, attribute);
}


DataEditor::DataEditor(const DataEditor &editor)
	:
	BLocker("data view"),
	fAttribute(NULL)
{
}


DataEditor::~DataEditor()
{
	free((void*)fAttribute);
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
	fSize = 0;
	fLastChange = fFirstChange = NULL;
	fChangesFromSaved = 0;
	fView = NULL;
	fRealViewOffset = 0;
	fViewOffset = 0;
	fRealViewSize = fViewSize = fBlockSize = 512;

	free((void*)fAttribute);
	if (attribute != NULL)
		fAttribute = strdup(attribute);
	else
		fAttribute = NULL;

	struct stat stat;
	status_t status = entry.GetStat(&stat);
	if (status < B_OK)
		return status;

	entry.GetRef(&fAttributeRef);

	bool isFileSystem = false;

	if (entry.IsDirectory()) {
		// we redirect root directories to their volumes
		BDirectory directory(&entry);
		if (directory.InitCheck() == B_OK && directory.IsRootDirectory()) {
			fs_info info;
			if (fs_stat_dev(stat.st_dev, &info) != 0)
				return errno;

			status = entry.SetTo(info.device_name);
			if (status < B_OK)
				return status;

			entry.GetStat(&stat);

			fBlockSize = info.block_size;
			if (fBlockSize > 0 && fBlockSize <= 65536)
				isFileSystem = true;
		}
	}

	status = fFile.SetTo(&entry, B_READ_WRITE);
	if (status < B_OK) {
		// try to open read only
		status = fFile.SetTo(&entry, B_READ_ONLY);
		if (status < B_OK)
			return status;

		fIsReadOnly = true;
	} else
		fIsReadOnly = false;

	entry.GetRef(&fRef);
	fIsDevice = S_ISBLK(stat.st_mode) || S_ISCHR(stat.st_mode);

	if (IsAttribute()) {
		BNode node(&fAttributeRef);
		attr_info info;
		status = node.GetAttrInfo(fAttribute, &info);
		if (status != B_OK) {
			fFile.Unset();
			return status;
		}

		fSize = info.size;
		fType = info.type;
	} else if (fIsDevice) {
		device_geometry geometry;
		int device = fFile.Dup();
		if (device < 0 || ioctl(device, B_GET_GEOMETRY, &geometry,
				sizeof(geometry)) < 0) {
			if (device >= 0)
				close(device);
			fFile.Unset();
			return B_ERROR;
		}
		close(device);

		fSize = 1LL * geometry.head_count * geometry.cylinder_count
			* geometry.sectors_per_track * geometry.bytes_per_sector;
		if (fSize < 0)
			fSize = 0;
		if (!isFileSystem)
			fBlockSize = geometry.bytes_per_sector;
	} else if (entry.IsDirectory() || entry.IsSymLink()) {
		fSize = 0;
		fIsReadOnly = true;
	} else {
		status = fFile.GetSize(&fSize);
		if (status < B_OK) {
			fFile.Unset();
			return status;
		}
	}

	if (fBlockSize == 0)
		fBlockSize = 512;

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
	change->Apply(fRealViewOffset, fView, fRealViewSize);

	SendNotices(change);
		// update observers

	// try to join changes
	if (fLastChange == NULL || !fLastChange->Merge(change)) {
		fChanges.AddItem(change);
		fLastChange = change;
		fChangesFromSaved++;
	} else
		delete change;
}


status_t
DataEditor::Replace(off_t offset, const uint8 *data, size_t length)
{
	if (IsReadOnly())
		return B_NOT_ALLOWED;

	BAutolock locker(this);

	if (offset >= fSize)
		return B_BAD_VALUE;
	if (offset + (off_t)length > fSize)
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
	if (IsReadOnly())
		return B_NOT_ALLOWED;

	BAutolock locker(this);

	// not yet implemented
	// ToDo: this needs some changes to the whole change mechanism

	return B_ERROR;
}


status_t
DataEditor::Insert(off_t offset, const uint8 *text, size_t length)
{
	if (IsReadOnly())
		return B_NOT_ALLOWED;

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

	int32 firstIndex = fFirstChange != NULL ? fChanges.IndexOf(fFirstChange) + 1
		: 0;
	int32 lastIndex = fChanges.IndexOf(fLastChange);

	if (fChangesFromSaved >= 0) {
		// ascend changes
		TRACE(("ApplyChanges(): ascend from %ld to %ld\n", firstIndex,
			lastIndex));

		for (int32 i = firstIndex; i <= lastIndex; i++) {
			DataChange *change = fChanges.ItemAt(i);
			change->Apply(fRealViewOffset, fView, fRealViewSize);
		}
	} else {
		// descend changes
		TRACE(("ApplyChanges(): descend from %ld to %ld\n", firstIndex - 1,
			lastIndex));

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

	int32 firstIndex = fFirstChange != NULL ? fChanges.IndexOf(fFirstChange) + 1
		: 0;
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
		if (fRealViewOffset + (off_t)fRealViewSize > (off_t)fSize)
			size = fSize - fRealViewOffset;

		ssize_t bytesWritten;
		if (IsAttribute()) {
			bytesWritten = fFile.WriteAttr(fAttribute, fType, fRealViewOffset,
				fView, size);
		} else
			bytesWritten = fFile.WriteAt(fRealViewOffset, fView, size);

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
	// ToDo: implement me!
	//fSize = size;
	//return B_OK;
	return B_ERROR;
}


status_t
DataEditor::SetViewOffset(off_t offset, bool sendNotices)
{
	BAutolock locker(this);

	if (fView == NULL) {
		status_t status = SetViewSize(fViewSize);
		if (status < B_OK)
			return status;
	}

	if (offset < 0 || offset > fSize)
		return B_BAD_VALUE;

	offset = (offset / fViewSize) * fViewSize;
	if (offset == fViewOffset)
		return B_OK;

	fViewOffset = offset;
	fRealViewOffset = (fViewOffset / fBlockSize) * fBlockSize;
	fNeedsUpdate = true;

	if (sendNotices) {
		BMessage update;
		update.AddInt64("offset", fViewOffset);
		SendNotices(kMsgDataEditorParameterChange, &update);
	}

	return B_OK;
}


status_t
DataEditor::SetViewOffset(off_t offset)
{
	return SetViewOffset(offset, true);
}


status_t
DataEditor::SetViewSize(size_t size, bool sendNotices)
{
	BAutolock locker(this);

	size_t realSize = (size + fBlockSize - 1) & ~(fBlockSize - 1);
		// round to the next multiple of block size

	if (realSize == fRealViewSize && fViewSize == size && fView != NULL)
		return B_OK;

	if (realSize == 0)
		return B_BAD_VALUE;

	if (realSize != fRealViewSize || fView == NULL) {
		uint8 *view = (uint8 *)realloc(fView, realSize);
		if (view == NULL)
			return B_NO_MEMORY;

		fView = view;
		fRealViewSize = realSize;
	}

	fViewSize = size;
	fNeedsUpdate = true;

	// let's correct the view offset if necessary
	if (fViewOffset % size)
		SetViewOffset(fViewOffset);

	if (sendNotices) {
		BMessage update;
		update.AddInt32("view_size", size);
		SendNotices(kMsgDataEditorParameterChange, &update);
	}

	return B_OK;
}


status_t
DataEditor::SetViewSize(size_t size)
{
	return SetViewSize(size, true);
}


status_t
DataEditor::SetBlockSize(size_t size)
{
	BAutolock locker(this);

	fBlockSize = size;
	status_t status = SetViewOffset(fViewOffset, false);
		// this will trigger an update of the real view offset
	if (status == B_OK)
		status = SetViewSize(fViewSize, false);

	BMessage update;
	update.AddInt32("block_size", size);
	update.AddInt64("offset", fViewOffset);
	SendNotices(kMsgDataEditorParameterChange, &update);

	return status;
}


status_t
DataEditor::Update()
{
	ssize_t bytesRead;
	if (IsAttribute()) {
		BNode node(&fAttributeRef);
		bytesRead = node.ReadAttr(fAttribute, fType, fRealViewOffset, fView,
			fRealViewSize);
	} else
		bytesRead = fFile.ReadAt(fRealViewOffset, fView, fRealViewSize);

	if (bytesRead < B_OK)
		return bytesRead;

	if ((size_t)bytesRead < fRealViewSize) {
		// make sure the rest of data is cleared
		memset(fView + bytesRead, 0, fRealViewSize - bytesRead);
	}

	ApplyChanges();
	fNeedsUpdate = false;

	return B_OK;
}


status_t
DataEditor::UpdateIfNeeded(bool *_updated)
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
DataEditor::ForceUpdate()
{
	BAutolock locker(this);

	status_t status = B_OK;

	off_t newSize = fSize;
	if (IsAttribute()) {
		// update attribute size (we ignore the type for now)
		attr_info info;
		status = fFile.GetAttrInfo(fAttribute, &info);
		if (status != B_OK) {
			// The attribute may have just been removed before
			// it gets rewritten, so we don't do anything
			// else here (we just set the file size to 0)
			newSize = 0;
		} else
			newSize = info.size;
	} else if (!IsDevice()) {
		// update file size

		if (fFile.GetSize(&newSize) != B_OK)
			return B_ERROR;
	}

	if (fSize != newSize) {
		fSize = newSize;

		// update observers
		BMessage update;
		update.AddInt64("file_size", newSize);
		SendNotices(kMsgDataEditorParameterChange, &update);
	}

	if (fView == NULL)
		status = SetViewOffset(fViewOffset);
	else {
		BMessage update;
		update.AddInt64("offset", fViewOffset);
		update.AddInt64("size", fViewSize);
		SendNotices(kMsgDataEditorUpdate, &update);
	}

	if (status == B_OK)
		status = Update();

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


off_t
DataEditor::Find(off_t startPosition, const uint8 *data, size_t dataSize,
	bool caseInsensitive, bool cyclic, BMessenger progressMonitor,
	volatile bool *stop)
{
	if (data == NULL || dataSize == 0)
		return B_BAD_VALUE;

	if (startPosition < 0)
		startPosition = 0;

	BAutolock locker(this);

	typedef int (*compare_func)(const uint8 *a, const uint8 *b, size_t size);
	compare_func compareFunc;
	if (caseInsensitive)
		compareFunc = CompareCaseInsensitive;
	else
		compareFunc = (compare_func)memcmp;

	bool savedIsReadOnly = fIsReadOnly;
	fIsReadOnly = true;
	off_t savedOffset = fViewOffset;
	off_t position = (startPosition / fRealViewSize) * fRealViewSize;
	size_t firstByte = startPosition % fRealViewSize;
	size_t matchLastOffset = 0;
	off_t foundAt = B_ENTRY_NOT_FOUND;
	bool noStop = false;
	if (stop == NULL)
		stop = &noStop;

	// start progress monitor
	{
		BMessage progress(kMsgDataEditorFindProgress);
		progress.AddBool("running", true);
		progressMonitor.SendMessage(&progress);
	}

	bigtime_t lastReport = 0;

	off_t blocks = fSize;
	if (!cyclic)
		blocks -= position;
	blocks = (blocks + fRealViewSize - 1) / fRealViewSize;

	for (; blocks-- > 0 && !*stop; position += fRealViewSize) {
		if (position > fSize)
			position = 0;

		SetViewOffset(position, false);
		if (fNeedsUpdate)
			Update();

		bigtime_t current = system_time();
		if (lastReport + 500000LL < current) {
			// report the progress two times a second
			BMessage progress(kMsgDataEditorFindProgress);
			progress.AddInt64("position", position);
			progressMonitor.SendMessage(&progress);

			lastReport = current;
		}

		// search for data in current block

		if (matchLastOffset != 0) {
			// we had a partial match in the previous block, let's
			// check if it is a whole match
			if (!compareFunc(fView, data + matchLastOffset,
					dataSize - matchLastOffset)) {
				matchLastOffset = 0;
				break;
			}

			foundAt = B_ENTRY_NOT_FOUND;
			matchLastOffset = 0;
		}

		for (size_t i = firstByte; i < fRealViewSize; i++) {
			if (position + (off_t)(i + dataSize) > (off_t)fSize)
				break;

			if (!compareFunc(fView + i, data, 1)) {
				// one byte matches, compare the rest
				size_t size = dataSize - 1;
				size_t offset = i + 1;

				if (offset + size > fRealViewSize)
					size = fRealViewSize - offset;

				if (size == 0 || !compareFunc(fView + offset, data + 1, size)) {
					foundAt = position + i;

					if (size != dataSize - 1) {
						// only a partial match, we have to check the start
						// of the next block
						matchLastOffset = size + 1;
					}
					break;
				}
			}
		}

		if (foundAt >= 0 && matchLastOffset == 0)
			break;

		firstByte = 0;
	}

	fIsReadOnly = savedIsReadOnly;

	if (foundAt >= 0 && matchLastOffset != 0)
		foundAt = B_ENTRY_NOT_FOUND;

	// stop progress monitor
	{
		BMessage progress(kMsgDataEditorFindProgress);
		progress.AddBool("running", false);
		progress.AddInt64("position", foundAt >= 0 ? foundAt : savedOffset);
		progressMonitor.SendMessage(&progress);
	}

	SetViewOffset(savedOffset, false);
		// this is to ensure that we're sending an update when
		// we're set to the found data offset

	if (foundAt < 0 && *stop)
		return B_INTERRUPTED;

	return foundAt;
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

