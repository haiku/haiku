/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "PackageLinksDirectory.h"

#include "AttributeDirectoryCookie.h"


namespace {

class EmptyAttributeDirectoryCookie : public AttributeDirectoryCookie {
public:
	virtual status_t Close()
	{
		return B_OK;
	}

	virtual status_t Read(dev_t volumeID, ino_t nodeID, struct dirent* buffer,
		size_t bufferSize, uint32* _count)
	{
		*_count = 0;
		return B_OK;
	}

	virtual status_t Rewind()
	{
		return B_OK;
	}
};


}	// unnamed namespace


PackageLinksDirectory::PackageLinksDirectory()
	:
	Directory(0)
		// the ID needs to be assigned later, when added to a volume
{
	bigtime_t timeMicros = real_time_clock_usecs();

	fModifiedTime.tv_sec = timeMicros / 1000000;
	fModifiedTime.tv_nsec = (timeMicros % 1000000) * 1000;
}


PackageLinksDirectory::~PackageLinksDirectory()
{
}


status_t
PackageLinksDirectory::Init(Directory* parent, const char* name)
{
	return Directory::Init(parent, name);
}


mode_t
PackageLinksDirectory::Mode() const
{
	return S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
}


uid_t
PackageLinksDirectory::UserID() const
{
	return 0;
}


gid_t
PackageLinksDirectory::GroupID() const
{
	return 0;
}


timespec
PackageLinksDirectory::ModifiedTime() const
{
	return fModifiedTime;
}


off_t
PackageLinksDirectory::FileSize() const
{
	return 0;
}


status_t
PackageLinksDirectory::OpenAttributeDirectory(
	AttributeDirectoryCookie*& _cookie)
{
	AttributeDirectoryCookie* cookie
		= new(std::nothrow) EmptyAttributeDirectoryCookie;
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


status_t
PackageLinksDirectory::OpenAttribute(const char* name, int openMode,
	AttributeCookie*& _cookie)
{
	return B_ENTRY_NOT_FOUND;
}
