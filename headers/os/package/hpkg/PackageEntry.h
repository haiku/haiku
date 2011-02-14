/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PACKAGE_ENTRY_H_
#define _PACKAGE__HPKG__PACKAGE_ENTRY_H_


#include <sys/stat.h>

#include <package/hpkg/PackageData.h>


namespace BPackageKit {

namespace BHPKG {


class BPackageEntry {
public:
								BPackageEntry(BPackageEntry* parent,
									const char* name);

			const BPackageEntry*	Parent() const	{ return fParent; }
			const char*			Name() const		{ return fName; }
			void*				UserToken() const	{ return fUserToken; }

			mode_t				Mode() const		{ return fMode; }

			const timespec&		AccessTime() const
									{ return fAccessTime; }
			const timespec&		ModifiedTime() const
									{ return fModifiedTime; }
			const timespec&		CreationTime() const
									{ return fCreationTime; }

			BPackageData&		Data()	{ return fData; }

			const char*			SymlinkPath() const	{ return fSymlinkPath; }

			void				SetUserToken(void* token)
									{ fUserToken = token; }

			void				SetType(uint32 type);
			void				SetPermissions(uint32 permissions);

			void				SetAccessTime(uint32 seconds)
									{ fAccessTime.tv_sec = seconds; }
			void				SetAccessTimeNanos(uint32 nanos)
									{ fAccessTime.tv_nsec = nanos; }
			void				SetModifiedTime(uint32 seconds)
									{ fModifiedTime.tv_sec = seconds; }
			void				SetModifiedTimeNanos(uint32 nanos)
									{ fModifiedTime.tv_nsec = nanos; }
			void				SetCreationTime(uint32 seconds)
									{ fCreationTime.tv_sec = seconds; }
			void				SetCreationTimeNanos(uint32 nanos)
									{ fCreationTime.tv_nsec = nanos; }

			void				SetSymlinkPath(const char* path)
									{ fSymlinkPath = path; }
private:
			BPackageEntry*		fParent;
			const char*			fName;
			void*				fUserToken;
			mode_t				fMode;
			timespec			fAccessTime;
			timespec			fModifiedTime;
			timespec			fCreationTime;
			BPackageData		fData;
			const char*			fSymlinkPath;
};


inline void
BPackageEntry::SetType(uint32 type)
{
	fMode = (fMode & ~(uint32)S_IFMT) | (type & S_IFMT);
}


inline void
BPackageEntry::SetPermissions(uint32 permissions)
{
	fMode = (fMode & ~(uint32)ALLPERMS) | (permissions & ALLPERMS);
}


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PACKAGE_ENTRY_H_
