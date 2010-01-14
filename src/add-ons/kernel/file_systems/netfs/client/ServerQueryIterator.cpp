// ServerQueryIterator.cpp

#include "ServerQueryIterator.h"

#include <new>

// constructor
ServerQueryIterator::ServerQueryIterator(Volume* volume)
	: QueryIterator(volume),
	  fRemoteCookie(-1),
	  fShareVolumeIDs(NULL),
	  fShareVolumeCount(0),
	  fShareVolumeIndex(0)
{
}

// destructor
ServerQueryIterator::~ServerQueryIterator()
{
}

// SetRemoteCookie
void
ServerQueryIterator::SetRemoteCookie(int32 cookie)
{
	fRemoteCookie = cookie;
}

// GetRemoteCookie
int32
ServerQueryIterator::GetRemoteCookie() const
{
	return fRemoteCookie;
}

// SetEntry
status_t
ServerQueryIterator::SetEntry(const int32* shareVolumeIDs,
	int32 shareVolumeCount, const NodeInfo& dirInfo,
	const EntryInfo& entryInfo)
{
	UnsetEntry();

	if (!shareVolumeIDs || shareVolumeCount <= 0)
		return B_BAD_VALUE;

	// copy volume IDs
	if (shareVolumeCount <= IN_OBJECT_ID_COUNT)
		fShareVolumeIDs = fInObjectIDs;
	else
		fShareVolumeIDs = new(std::nothrow) int32[shareVolumeCount];
	if (!fShareVolumeIDs)
		return B_NO_MEMORY;
	fShareVolumeCount = shareVolumeCount;
	memcpy(fShareVolumeIDs, shareVolumeIDs, shareVolumeCount * 4);

	// copy entry name
	if (!fEntryName.SetTo(entryInfo.name.GetString())) {
		UnsetEntry();
		return B_NO_MEMORY;
	}

	fDirectoryInfo = dirInfo;
	fEntryInfo = entryInfo;
	fEntryInfo.name.SetTo(fEntryName.GetString());

	return B_OK;
}

// UnsetEntry
void
ServerQueryIterator::UnsetEntry()
{
	if (fShareVolumeIDs && fShareVolumeIDs != fInObjectIDs)
		delete[] fShareVolumeIDs;
	fShareVolumeIDs = NULL;

	fShareVolumeCount = 0;
	fShareVolumeIndex = 0;
	fEntryName.Unset();
	fEntryInfo.name.SetTo(NULL);
}

// GetShareVolumeIDs
const int32*
ServerQueryIterator::GetShareVolumeIDs() const
{
	return fShareVolumeIDs;
}

// CountShareVolumes
int32
ServerQueryIterator::CountShareVolumes() const
{
	return fShareVolumeCount;
}

// GetDirectoryInfo
const NodeInfo&
ServerQueryIterator::GetDirectoryInfo() const
{
	return fDirectoryInfo;
}

// GetEntryInfo
const EntryInfo&
ServerQueryIterator::GetEntryInfo() const
{
	return fEntryInfo;
}

// HasNextShareVolumeID
bool
ServerQueryIterator::HasNextShareVolumeID() const
{
	return (fShareVolumeIDs && fShareVolumeIndex < fShareVolumeCount);
}

// NextShareVolumeID
int32
ServerQueryIterator::NextShareVolumeID()
{
	if (!fShareVolumeIDs || fShareVolumeIndex >= fShareVolumeCount)
		return B_ENTRY_NOT_FOUND;

	return fShareVolumeIDs[fShareVolumeIndex++];
}
