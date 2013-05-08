/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _MERGED_DIRECTORY_H
#define _MERGED_DIRECTORY_H


#include <EntryList.h>
#include <ObjectList.h>


class BDirectory;


class BMergedDirectory : public BEntryList {
public:
			// policy how to handle equally named entries in different
			// directories
			enum BPolicy {
				B_ALLOW_DUPLICATES,
				B_ALWAYS_FIRST,
				B_COMPARE
			};

public:
								BMergedDirectory(
									BPolicy policy = B_ALWAYS_FIRST);
	virtual						~BMergedDirectory();

			status_t			Init();

			BPolicy				Policy() const;
			void				SetPolicy(BPolicy policy);

			status_t			AddDirectory(BDirectory* directory);
			status_t			AddDirectory(const char* path);

	virtual status_t			GetNextEntry(BEntry* entry,
									bool traverse = false);
	virtual status_t			GetNextRef(entry_ref* ref);
	virtual int32				GetNextDirents(struct dirent* direntBuffer,
									size_t bufferSize,
									int32 maxEntries = INT_MAX);
	virtual status_t			Rewind();
	virtual int32				CountEntries();

protected:
	virtual	bool				ShallPreferFirstEntry(const entry_ref& entry1,
									int32 index1, const entry_ref& entry2,
									int32 index2);
									// always invoked with index1 < index2

private:
			typedef BObjectList<BDirectory> DirectoryList;
			struct EntryNameSet;

private:
			bool				_IsBestEntry(const char* name);

private:
			DirectoryList		fDirectories;
			BPolicy				fPolicy;
			int32				fDirectoryIndex;
			EntryNameSet*		fVisitedEntries;
};


#endif	// _MERGED_DIRECTORY_H
