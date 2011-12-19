/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ENTRY_H
#define _ENTRY_H


#include <sys/types.h>
#include <sys/stat.h>
#include <SupportDefs.h>

#include <Statable.h>


class BDirectory;
class BPath;


struct entry_ref {
								entry_ref();
								entry_ref(dev_t dev, ino_t dir,
									const char* name);
								entry_ref(const entry_ref& ref);
								~entry_ref();

			status_t			set_name(const char* name);

			bool				operator==(const entry_ref& ref) const;
			bool				operator!=(const entry_ref& ref) const;
			entry_ref&			operator=(const entry_ref& ref);

			dev_t				device;
			ino_t				directory;
			char*				name;
};


class BEntry : public BStatable {
public:
								BEntry();
								BEntry(const BDirectory* dir, const char* path,
									bool traverse = false);
								BEntry(const entry_ref* ref,
									bool traverse = false);
								BEntry(const char* path, bool traverse = false);
								BEntry(const BEntry& entry);
	virtual						~BEntry();

			status_t			InitCheck() const;
			bool				Exists() const;

	virtual status_t			GetStat(struct stat* stat) const;

			status_t			SetTo(const BDirectory* dir, const char* path,
								   bool traverse = false);
			status_t			SetTo(const entry_ref* ref,
									bool traverse = false);
			status_t			SetTo(const char* path, bool traverse = false);
			void				Unset();

			status_t			GetRef(entry_ref* ref) const;
			status_t			GetPath(BPath* path) const;
			status_t			GetParent(BEntry* entry) const;
			status_t			GetParent(BDirectory* dir) const;
			status_t			GetName(char* buffer) const;

			status_t			Rename(const char* path, bool clobber = false);
			status_t			MoveTo(BDirectory* dir, const char* path = NULL,
									bool clobber = false);
			status_t			Remove();

			bool				operator==(const BEntry& item) const;
			bool				operator!=(const BEntry& item) const;

			BEntry&				operator=(const BEntry& item);

private:
			friend class BDirectory;
			friend class BFile;
			friend class BNode;
			friend class BSymLink;

	virtual	void				_PennyEntry1();
	virtual	void				_PennyEntry2();
	virtual	void				_PennyEntry3();
	virtual	void				_PennyEntry4();
	virtual	void				_PennyEntry5();
	virtual	void				_PennyEntry6();

	// BEntry implementation of BStatable::set_stat()
	virtual	status_t			set_stat(struct stat& stat, uint32 what);
			status_t			_SetTo(int dir, const char* path,
									bool traverse);
			status_t			_SetName(const char* name);

			status_t			_Rename(BEntry& target, bool clobber);

			void				_Dump(const char* name = NULL);

			status_t			_GetStat(struct stat* stat) const;
	virtual status_t			_GetStat(struct stat_beos* stat) const;

private:
			int					fDirFd;
			char*				fName;
			status_t			fCStatus;

			uint32				_reserved[4];
};


status_t get_ref_for_path(const char* path, entry_ref* ref);
bool operator<(const entry_ref& a, const entry_ref& b);


#endif	// _ENTRY_H
