//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Directory.h
	BDirectory interface declaration.
*/

#ifndef __sk_directory_h__
#define __sk_directory_h__

#include <Node.h>
#include <EntryList.h>
#include <StorageDefs.h>
#include <StorageDefs.Private.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

class BSymLink;

/*!
	\class BDirectory
	\brief A directory in the filesystem
	
	Provides an interface for manipulating directories and their contents.

	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	\author <a href="mailto:tylerdauwalder@users.sf.net">Tyler Dauwalder</a>
	
	\version 0.0.0
*/
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
	virtual void _ReservedDirectory1();
	virtual void _ReservedDirectory2();
	virtual void _ReservedDirectory3();
	virtual void _ReservedDirectory4();
	virtual void _ReservedDirectory5();
	virtual void _ReservedDirectory6();

private:
	virtual void close_fd();
	StorageKit::FileDescriptor get_fd() const;

private:
	uint32 _reservedData[7];
	StorageKit::FileDescriptor fDirFd;

	friend class BEntry;
};


// C functions

status_t create_directory(const char *path, mode_t mode);


#ifdef USE_OPENBEOS_NAMESPACE
};		// namespace OpenBeOS
#endif

#endif	// __sk_directory_h__
