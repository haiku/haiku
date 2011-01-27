/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_ENTRY_H
#define PACKAGE_ENTRY_H


#include <sys/stat.h>

#include <package/hpkg/PackageData.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


class PackageEntry {
public:
								PackageEntry(PackageEntry* parent,
									const char* name);

			const PackageEntry*	Parent() const		{ return fParent; }
			const char*			Name() const		{ return fName; }
			void*				UserToken() const	{ return fUserToken; }

			mode_t				Mode() const		{ return fMode; }

			const timespec&		AccessTime() const
									{ return fAccessTime; }
			const timespec&		ModifiedTime() const
									{ return fModifiedTime; }
			const timespec&		CreationTime() const
									{ return fCreationTime; }

			PackageData&		Data()	{ return fData; }

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
			PackageEntry*		fParent;
			const char*			fName;
			void*				fUserToken;
			mode_t				fMode;
			timespec			fAccessTime;
			timespec			fModifiedTime;
			timespec			fCreationTime;
			PackageData			fData;
			const char*			fSymlinkPath;
};


inline void
PackageEntry::SetType(uint32 type)
{
	fMode = (fMode & ~(uint32)S_IFMT) | (type & S_IFMT);
}


inline void
PackageEntry::SetPermissions(uint32 permissions)
{
	fMode = (fMode & ~(uint32)ALLPERMS) | (permissions & ALLPERMS);
}


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// PACKAGE_ENTRY_H
