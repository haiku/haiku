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
		size = offset - bufferOffset + offset;
}


void 
ReplaceChange::Apply(off_t bufferOffset, uint8 *buffer, size_t bufferSize)
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
		if (status != B_OK)
			return status;

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
	fView = NULL;
	fRealViewOffset = 0;
	fViewOffset = 0;
	fRealViewSize = fViewSize = fBlockSize;
	fNeedsUpdate = true;

	return B_OK;
}


status_t 
DataEditor::SetToAttribute(const char *attribute)
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	fAttribute = attribute;

	// ToDo: attributes are not yet supported
	return B_ERROR;
}


status_t 
DataEditor::InitCheck()
{
	if (fAttribute != NULL)
		// ToDo: for now!
		return B_ERROR;

	return fFile.InitCheck();
}


void 
DataEditor::AddChange(DataChange *change)
{
	if (change == NULL)
		return;

	bool removed = RemoveRedos();
	bool changed = !CanUndo();

	fChanges.AddItem(change);
	fLastChange = change;

	fLastChange->Apply(fRealViewOffset, fView, fRealViewSize);

	// update observers

	SendNotices(fLastChange);

	BMessage update;
	if (removed)
		update.AddBool("can_redo", false);
	if (changed)
		update.AddBool("can_undo", true);
	SendNotices(kMsgDataEditorStateChange, &update);
}


status_t 
DataEditor::Replace(off_t offset, const uint8 *data, size_t length)
{
	if (!IsLocked())
		debugger("DataEditor: view not locked");

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
	if (!IsLocked())
		debugger("DataEditor: view not locked");

	// not yet implemented

	return B_ERROR;
}


status_t 
DataEditor::Insert(off_t offset, const uint8 *text, size_t length)
{
	if (!IsLocked())
		debugger("DataEditor: view not locked");

	// not yet implemented

	return B_ERROR;
}


void 
DataEditor::ApplyChanges()
{
	if (fLastChange == NULL)
		return;

	int32 count = fChanges.IndexOf(fLastChange) + 1;

	for (int32 i = 0; i < count; i++) {
		DataChange *change = fChanges.ItemAt(i);
		change->Apply(fRealViewOffset, fView, fRealViewSize);
	}
}


/** This method will be called by DataEditor::AddChange()
 *	immediately before a change is applied.
 *	It removes all pending redo nodes from the list that would
 *	come after the current change.
 */

bool
DataEditor::RemoveRedos()
{
	if (fLastChange == NULL)
		return false;

	int32 start = fChanges.IndexOf(fLastChange) + 1;
	bool removed = false;

	for (int32 i = fChanges.CountItems(); i-- > start; ) {
		DataChange *change = fChanges.RemoveItemAt(i);
		delete change;
		removed = true;
	}

	return removed;
}


status_t 
DataEditor::Undo()
{
	BAutolock locker(this);

	if (!CanUndo())
		return B_ERROR;

	bool couldRedo = CanRedo();
	DataChange *undoChange = fLastChange;

	int32 index = fChanges.IndexOf(undoChange);
	undoChange->Revert(fRealViewOffset, fView, fRealViewSize);

	if (index > 0)
		fLastChange = fChanges.ItemAt(index - 1);
	else
		fLastChange = NULL;

	// update observers
	SendNotices(undoChange);

	BMessage update;
	if (!couldRedo)
		update.AddBool("can_redo", true);
	if (!CanUndo())
		update.AddBool("can_undo", false);
	SendNotices(kMsgDataEditorStateChange, &update);

	return B_OK;
}


status_t 
DataEditor::Redo()
{
	BAutolock locker(this);

	if (!CanRedo())
		return B_ERROR;

	bool couldUndo = CanUndo();

	int32 index = fChanges.IndexOf(fLastChange);
	fLastChange = fChanges.ItemAt(index + 1);

	fLastChange->Apply(fRealViewOffset, fView, fRealViewSize);

	// update observers
	SendNotices(fLastChange);

	BMessage update;
	if (!CanRedo())
		update.AddBool("can_redo", false);
	if (!couldUndo)
		update.AddBool("can_undo", true);
	SendNotices(kMsgDataEditorStateChange, &update);

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
DataEditor::SetViewOffset(off_t offset)
{
	if (fView == NULL) {
		status_t status = SetViewSize(fViewSize);
		if (status < B_OK)
			return status;
	}

	fRealViewOffset = (offset / fBlockSize) * fBlockSize;
	fViewOffset = offset;
	fNeedsUpdate = true;

	SendNotices(kMsgDataEditorOffsetChange);
	return B_OK;
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

