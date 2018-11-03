/*
 * Copyright 2004-2018, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef DATA_EDITOR_H
#define DATA_EDITOR_H


#include <File.h>
#include <Entry.h>
#include <Locker.h>
#include <ObjectList.h>


class BHandler;
class BLooper;
class BMessage;
class BMessenger;

class DataChange;
class StateWatcher;


class DataEditor : public BLocker {
public:
								DataEditor();
								DataEditor(entry_ref& ref,
									const char* attribute = NULL);
								DataEditor(BEntry& entry,
									const char* attribute = NULL);
								DataEditor(const DataEditor& editor);
	virtual						~DataEditor();

			status_t			SetTo(const char* path,
									const char* attribute = NULL);
			status_t			SetTo(entry_ref &ref,
									const char* attribute = NULL);
			status_t			SetTo(BEntry &entry,
									const char* attribute = NULL);

			status_t			Save();

			bool				IsReadOnly() const { return fIsReadOnly; }
			bool				IsDevice() const { return fIsDevice; }
			bool				IsAttribute() const
									{ return fAttribute != NULL; }
			bool				IsModified() const
									{ return fLastChange != fFirstChange; }

			const char*			Attribute() const { return fAttribute; }
			type_code			Type() const { return fType; }

			status_t			InitCheck();

			status_t			Replace(off_t offset, const uint8* data,
									size_t length);
			status_t			Remove(off_t offset, off_t length);
			status_t			Insert(off_t offset, const uint8* data,
									size_t length);

			status_t			MoveBy(int32 bytes);
			status_t			MoveTo(off_t offset);

			status_t			Undo();
			status_t			Redo();

			bool				CanUndo() const;
			bool				CanRedo() const;

			status_t			SetFileSize(off_t size);
			off_t				FileSize() const { return fSize; }

			status_t			SetViewOffset(off_t offset);
			off_t				ViewOffset() const { return fViewOffset; }

			status_t			SetViewSize(size_t size);
			size_t				ViewSize() const { return fViewSize; }

			status_t			SetBlockSize(size_t size);
			size_t				BlockSize() const { return fBlockSize; }

			status_t			UpdateIfNeeded(bool* _updated = NULL);
			status_t			ForceUpdate();
			status_t			GetViewBuffer(const uint8** _buffer);

			status_t			StartWatching(BMessenger target);
			status_t			StartWatching(BHandler* handler,
									BLooper* looper = NULL);
			void				StopWatching(BMessenger target);
			void				StopWatching(BHandler* handler,
									BLooper* looper = NULL);

			off_t				Find(off_t startPosition, const uint8* data,
									size_t dataSize, bool caseInsensitive,
									bool cyclic, BMessenger progressMessenger,
									volatile bool* stop = NULL);

			BFile&				File() { return fFile; }
			const entry_ref&	AttributeRef() const { return fAttributeRef; }
			const entry_ref&	Ref() const { return fRef; }

private:
	friend class StateWatcher;

			status_t			SetViewOffset(off_t offset, bool sendNotices);
			status_t			SetViewSize(size_t size, bool sendNotices);
			void				SendNotices(uint32 what,
									BMessage* message = NULL);
			void				SendNotices(DataChange* change);
			status_t			Update();
			void				AddChange(DataChange* change);
			void				ApplyChanges();
			void				RemoveRedos();

private:
			BObjectList<BMessenger> fObservers;

			entry_ref			fRef;
			entry_ref			fAttributeRef;
			BFile				fFile;
			const char*			fAttribute;
			type_code			fType;
			bool				fIsDevice;
			bool				fIsReadOnly;
			off_t				fRealSize;
			off_t				fSize;

			BObjectList<DataChange>	fChanges;
			DataChange*			fFirstChange;
			DataChange*			fLastChange;
			int32				fChangesFromSaved;

			uint8*				fView;
			off_t				fRealViewOffset;
			off_t				fViewOffset;
			size_t				fRealViewSize;
			size_t				fViewSize;
			bool				fNeedsUpdate;

			size_t				fBlockSize;
};


static const uint32 kMsgDataEditorStateChange = 'deSC';
static const uint32 kMsgDataEditorUpdate = 'deUp';
static const uint32 kMsgDataEditorParameterChange = 'dePC';

static const uint32 kMsgDataEditorFindProgress = 'deFP';


#endif	/* DATA_EDITOR_H */
