// Entry.cpp

#include "Directory.h"
#include "Entry.h"
#include "FDManager.h"
#include "Path.h"
#include "String.h"
#include "Volume.h"
#include "VolumeManager.h"

// #pragma mark -

// Entry

// constructor
Entry::Entry(Volume* volume, Directory* directory, const char* name, Node* node)
	: fVolume(volume),
	  fDirectory(directory),
	  fName(name),
	  fNode(node),
	  fDirEntryLink()
{
}

// destructor
Entry::~Entry()
{
}

// InitCheck
status_t
Entry::InitCheck() const
{
	return (fName.GetLength() > 0 ? B_OK : B_NO_MEMORY);
}

// GetVolume
Volume*
Entry::GetVolume() const
{
	return fVolume;
}

// GetDirectory
Directory*
Entry::GetDirectory() const
{
	return fDirectory;
}

// GetEntryRef
NoAllocEntryRef
Entry::GetEntryRef() const
{
	return NoAllocEntryRef(fVolume->GetID(), fDirectory->GetID(),
		fName.GetString());
}

// GetVolumeID
dev_t
Entry::GetVolumeID() const
{
	return fVolume->GetID();
}

// GetDirectoryID
ino_t
Entry::GetDirectoryID() const
{
	return fDirectory->GetID();
}

// GetName
const char*
Entry::GetName() const
{
	return fName.GetString();
}

// GetNode
Node*
Entry::GetNode() const
{
	return fNode;
}

// GetPath
status_t
Entry::GetPath(Path* path)
{
	return VolumeManager::GetDefault()->GetPath(this, path);
}

// Exists
bool
Entry::Exists() const
{
	NoAllocEntryRef entryRef(GetEntryRef());
	BEntry bEntry;
	return (FDManager::SetEntry(&bEntry, &entryRef) == B_OK && bEntry.Exists());
}

// IsActualEntry
bool
Entry::IsActualEntry() const
{
	return (fName.GetLength() > 0 && fName != "." && fName != "..");
}
