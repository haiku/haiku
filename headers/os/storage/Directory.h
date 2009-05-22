/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DIRECTORY_H
#define _DIRECTORY_H


#include <Node.h>
#include <EntryList.h>
#include <StorageDefs.h>


class BFile;
class BSymLink;
struct stat_beos;


class BDirectory : public BNode, public BEntryList {
	public:
		BDirectory();
		BDirectory(const BDirectory &dir);
		BDirectory(const entry_ref *ref);
		BDirectory(const node_ref *nref);
		BDirectory(const BEntry *entry);
		BDirectory(const char *path);
		BDirectory(const BDirectory *dir, const char *path);

		virtual ~BDirectory();

		status_t SetTo(const entry_ref *ref);
		status_t SetTo(const node_ref *nref);
		status_t SetTo(const BEntry *entry);
		status_t SetTo(const char *path);
		status_t SetTo(const BDirectory *dir, const char *path);

		status_t GetEntry(BEntry *entry) const;

		bool IsRootDirectory() const;

		status_t FindEntry(const char *path, BEntry *entry,
			bool traverse = false) const;

		bool Contains(const char *path, int32 nodeFlags = B_ANY_NODE) const;
		bool Contains(const BEntry *entry, int32 nodeFlags = B_ANY_NODE) const;

		status_t GetStatFor(const char *path, struct stat *st) const;

		virtual status_t GetNextEntry(BEntry *entry, bool traverse = false);
		virtual status_t GetNextRef(entry_ref *ref);
		virtual int32 GetNextDirents(dirent *buf, size_t bufSize,
			int32 count = INT_MAX);
		virtual status_t Rewind();
		virtual int32 CountEntries();

		status_t CreateDirectory(const char *path, BDirectory *dir);
		status_t CreateFile(const char *path, BFile *file,
			bool failIfExists = false);
		status_t CreateSymLink(const char *path, const char *linkToPath,
			BSymLink *link);

		BDirectory &operator=(const BDirectory &dir);

	private:
		friend class BNode;
		friend class BEntry;
		friend class BFile;

		status_t _GetStatFor(const char *path, struct stat *st) const;
		status_t _GetStatFor(const char *path, struct stat_beos *st) const;

		virtual void _ErectorDirectory1();
		virtual void _ErectorDirectory2();
		virtual void _ErectorDirectory3();
		virtual void _ErectorDirectory4();
		virtual void _ErectorDirectory5();
		virtual void _ErectorDirectory6();

	private:
		virtual void close_fd();
		int get_fd() const;

	private:
		uint32 _reservedData[7];
		int fDirFd;
};

status_t create_directory(const char *path, mode_t mode);

#endif	// _DIRECTORY_H
