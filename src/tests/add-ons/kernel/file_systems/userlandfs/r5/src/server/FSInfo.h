// FSInfo.h

#ifndef USERLAND_FS_FS_INFO_H
#define USERLAND_FS_FS_INFO_H

#include <new>

#include <string.h>

#include <Message.h>

#include "Port.h"
#include "String.h"

namespace UserlandFS {

// FSInfo
class FSInfo {
public:
	FSInfo()
		: fInfos(NULL),
		  fCount(0)
	{
	}

	FSInfo(const char* fsName, const Port::Info* infos, int32 count)
		: fName(),
		  fInfos(NULL),
		  fCount(0)
	{
		SetTo(fsName, infos, count);
	}

	FSInfo(const BMessage* message)
		: fName(),
		  fInfos(NULL),
		  fCount(0)
	{
		SetTo(message);
	}

	FSInfo(const FSInfo& other)
		: fName(),
		  fInfos(NULL),
		  fCount(0)
	{
		SetTo(other.GetName(), other.fInfos, other.fCount);
	}

	~FSInfo()
	{
		Unset();
	}

	status_t SetTo(const char* fsName, const Port::Info* infos, int32 count)
	{
		Unset();
		if (!fsName || !infos || count <= 0)
			return B_BAD_VALUE;
		if (!fName.SetTo(fsName))
			return B_NO_MEMORY;
		fInfos = new(nothrow) Port::Info[count];
		if (!fInfos)
			return B_NO_MEMORY;
		memcpy(fInfos, infos, sizeof(Port::Info) * count);
		fCount = count;
		return B_OK;
	}

	status_t SetTo(const BMessage* message)
	{
		Unset();
		if (!message)
			return B_BAD_VALUE;
		const void* infos;
		ssize_t size;
		const char* fsName;
		if (message->FindData("infos", B_RAW_TYPE, &infos, &size) != B_OK
			|| size < 0 || message->FindString("fsName", &fsName) != B_OK) {
			return B_BAD_VALUE;
		}
		return SetTo(fsName, (const Port::Info*)infos,
			size / sizeof(Port::Info));
	}

	void Unset()
	{
		fName.Unset();
		delete[] fInfos;
		fInfos = NULL;
		fCount = 0;
	}

	const char* GetName() const
	{
		return fName.GetString();
	}

	Port::Info* GetInfos() const
	{
		return fInfos;
	}

	int32 CountInfos() const
	{
		return fCount;
	}

	int32 GetSize() const
	{
		return fCount * sizeof(Port::Info);
	}

	status_t Archive(BMessage* archive)
	{
		if (!fName.GetString() || !fInfos)
			return B_NO_INIT;
		status_t error = archive->AddString("fsName", fName.GetString());
		if (error != B_OK)
			return error;
		return archive->AddData("infos", B_RAW_TYPE, fInfos,
			fCount * sizeof(Port::Info));
	}

private:
	String			fName;
	Port::Info*		fInfos;
	int32			fCount;
};

}	// namespace UserlandFS

using UserlandFS::FSInfo;

#endif	// USERLAND_FS_FS_INFO_H
