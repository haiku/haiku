/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef DATA_EDITOR_H
#define DATA_EDITOR_H


#include <File.h>
#include <Entry.h>
#include <Locker.h>
#include <ObjectList.h>


class DataChange;

class DataEditor {
	public:
		DataEditor();
		DataEditor(entry_ref &ref, const char *attribute = NULL);
		DataEditor(BEntry &entry, const char *attribute = NULL);
		DataEditor(const DataEditor &editor);
		~DataEditor();

		status_t SetTo(const char *path, const char *attribute = NULL);
		status_t SetTo(entry_ref &ref, const char *attribute = NULL);
		status_t SetTo(BEntry &entry, const char *attribute = NULL);
		status_t SetToAttribute(const char *attribute);

		bool IsReadOnly() const { return fIsReadOnly; }
		bool IsDevice() const { return fIsDevice; }
		bool IsAttribute() const { return fAttribute != NULL; }
		//bool IsModified() const { return fIsModified; }

		const char *Attribute() const { return fAttribute; }

		status_t InitCheck();

		status_t Replace(off_t offset, const uint8 *data, size_t length);
		status_t Remove(off_t offset, off_t length);
		status_t Insert(off_t offset, const uint8 *data, size_t length);

		status_t MoveBy(int32 bytes);
		status_t MoveTo(off_t offset);

		status_t Undo();
		status_t Redo();

		bool CanUndo() const;
		bool CanRedo() const;

		status_t SetFileSize(off_t size);
		off_t FileSize() const { return fSize; }

		status_t SetViewOffset(off_t offset);
		off_t ViewOffset() const { return fViewOffset; }

		status_t SetViewSize(size_t size);
		off_t ViewSize() const { return fViewSize; }

		void SetBlockSize(size_t size);
		size_t BlockSize() const { return fBlockSize; }

		status_t GetViewBuffer(const uint8 **_buffer);

		BLocker &Locker() { return fLock; }
		bool Lock();
		void Unlock();

		status_t StartWatching(BMessenger target);
		status_t StartWatching(BHandler *handler, BLooper *looper = NULL);
		void StopWatching(BMessenger target);
		void StopWatching(BHandler *handler, BLooper *looper = NULL);

		BFile &File() { return fFile; }

	private:
		void SendNotices(uint32 what, BMessage *message = NULL);
		status_t Update();
		void AddChange(DataChange *change);
		void ApplyChanges();
		void RemoveRedos();

		BObjectList<BMessenger> fObservers;

		BFile		fFile;
		const char	*fAttribute;
		bool		fIsDevice, fIsReadOnly;
		off_t		fRealSize, fSize;

		BObjectList<DataChange>	fChanges;
		DataChange				*fFirstChange;
		DataChange				*fLastChange;

		BLocker		fLock;
		uint8		*fView;
		off_t		fRealViewOffset, fViewOffset;
		size_t		fRealViewSize, fViewSize;
		bool		fNeedsUpdate;

		size_t		fBlockSize;
};

static const uint32 kMsgDataEditorUndoState = 'deUS';
static const uint32 kMsgDataEditorRedoState = 'deRS';
static const uint32 kMsgDataEditorModifiedState = 'deMS';
static const uint32 kMsgDataEditorUpdate = 'deUp';

#endif	/* DATA_EDITOR_H */
