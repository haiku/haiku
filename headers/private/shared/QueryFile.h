/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef _QUERY_FILE_H
#define _QUERY_FILE_H


#include <EntryList.h>
#include <Query.h>
#include <List.h>
#include <String.h>


class BQueryFile : public BEntryList {
public:
								BQueryFile(const entry_ref& ref);
								BQueryFile(const BEntry& entry);
								BQueryFile(const char* path);
								BQueryFile(BQuery& query);
	virtual						~BQueryFile();

			status_t			InitCheck() const;

			status_t			SetTo(const entry_ref& ref);
			status_t			SetTo(const BEntry& entry);
			status_t			SetTo(const char* path);
			status_t			SetTo(BQuery& query);
			void				Unset();

			status_t			SetPredicate(const char* predicate);
			status_t			AddVolume(const BVolume& volume);
			status_t			AddVolume(dev_t device);

			const char*			Predicate() const;
			int32				CountVolumes() const;
			dev_t				VolumeAt(int32 index) const;

			status_t			WriteTo(const entry_ref& ref);
			status_t			WriteTo(const char* path);

			// BEntryList implementation

	virtual	status_t			GetNextEntry(BEntry* entry,
									bool traverse = false);
	virtual	status_t			GetNextRef(entry_ref* ref);
	virtual	int32				GetNextDirents(struct dirent* buffer,
									size_t length, int32 count = INT_MAX);
	virtual	status_t			Rewind();
	virtual	int32				CountEntries();

	static	const char*			MimeType();

private:
			status_t			_SetQuery(int32 index);

private:
			status_t			fStatus;
			BString				fPredicate;
			BQuery				fQuery;
			BList				fVolumes;
			int32				fCurrentVolumeIndex;
};


#endif	// _QUERY_FILE_H
