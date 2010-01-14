// VirtualDir.cpp

#include "VirtualDir.h"

#include <string.h>

#include <AutoDeleter.h>

// constructor
VirtualDirEntry::VirtualDirEntry()
	: fName(),
	  fNode(NULL)
{
}

// destructor
VirtualDirEntry::~VirtualDirEntry()
{
}

// SetTo
status_t
VirtualDirEntry::SetTo(const char* name, Node* node)
{
	if (!name || strlen(name) == 0 || !node)
		return B_BAD_VALUE;

	if (!fName.SetTo(name))
		return B_NO_MEMORY;
	fNode = node;

	return B_OK;
}

// GetName
const char*
VirtualDirEntry::GetName() const
{
	return fName.GetString();
}

// GetNode
Node*
VirtualDirEntry::GetNode() const
{
	return fNode;
}


// #pragma mark -

// constructor
VirtualDirIterator::VirtualDirIterator()
	: fDirectory(NULL),
	  fCurrentEntry(NULL),
	  fState(STATE_DOT)
{
}

// destructor
VirtualDirIterator::~VirtualDirIterator()
{
	SetDirectory(NULL);
}

// SetDirectory
void
VirtualDirIterator::SetDirectory(VirtualDir* directory, bool onlyChildren)
{
	// unset the old directory
	if (fDirectory)
		fDirectory->RemoveDirIterator(this);

	// set the new directory
	fDirectory = directory;
	if (fDirectory) {
		fDirectory->AddDirIterator(this);

		if (onlyChildren) {
			fCurrentEntry = fDirectory->GetFirstEntry();
			fState = STATE_OTHERS;
		} else {
			fCurrentEntry = NULL;
			fState = STATE_DOT;
		}
	}
}

// GetCurrentEntry
bool
VirtualDirIterator::GetCurrentEntry(const char** name, Node** node)
{
	if (!fDirectory)
		return false;
	switch (fState) {
		case STATE_DOT:
			*name = ".";
			*node = fDirectory;
			return true;
		case STATE_DOT_DOT:
			*name = "..";
			*node = fDirectory->GetParent();
			if (!*node)
				*node = fDirectory;
			return true;
		default:
			if (!fCurrentEntry)
				return false;
			*name = fCurrentEntry->GetName();
			*node = fCurrentEntry->GetNode();
			return true;
	}
}

// GetCurrentEntry
VirtualDirEntry*
VirtualDirIterator::GetCurrentEntry() const
{
	return fCurrentEntry;
}

// NextEntry
void
VirtualDirIterator::NextEntry()
{
	if (!fDirectory)
		return;
	switch (fState) {
		case STATE_DOT:
			fState = STATE_DOT_DOT;
			break;
		case STATE_DOT_DOT:
			fState = STATE_OTHERS;
			fCurrentEntry = fDirectory->GetFirstEntry();
			break;
		default:
			if (fCurrentEntry)
				fCurrentEntry = fDirectory->GetNextEntry(fCurrentEntry);
			break;
	}
}

// Rewind
void
VirtualDirIterator::Rewind()
{
	fState = STATE_DOT;
	fCurrentEntry = NULL;
}


// #pragma mark -

// constructor
VirtualDir::VirtualDir(Volume* volume, vnode_id nodeID)
	: Node(volume, nodeID),
	  fParent(NULL),
	  fCreationTime(0),
	  fEntries(),
	  fEntryList(),
	  fIterators()
{
}

// destructor
VirtualDir::~VirtualDir()
{
	while (VirtualDirEntry* entry = GetFirstEntry())
		RemoveEntry(entry->GetName());
}

// InitCheck
status_t
VirtualDir::InitCheck() const
{
	return fEntries.InitCheck();
}

// SetParent
void
VirtualDir::SetParent(VirtualDir* parent)
{
	fParent = parent;
}

// GetParent
VirtualDir*
VirtualDir::GetParent() const
{
	return fParent;
}

// GetCreationTime
time_t
VirtualDir::GetCreationTime() const
{
	return fCreationTime;
}

// AddEntry
status_t
VirtualDir::AddEntry(const char* name, Node* child)
{
	if (!name || !child || fEntries.ContainsKey(name))
		return B_BAD_VALUE;

	// create an entry
	VirtualDirEntry* entry = new(std::nothrow) VirtualDirEntry;
	if (!entry)
		return B_NO_MEMORY;
	ObjectDeleter<VirtualDirEntry> entryDeleter(entry);
	status_t error = entry->SetTo(name, child);
	if (error != B_OK)
		return error;

	// add the entry
	error = fEntries.Put(name, entry);
	if (error != B_OK)
		return error;
	fEntryList.Insert(entry);
	entryDeleter.Detach();

	// TODO: That's not so nice. Check whether better to move the fParent
	// property to Node.
	if (VirtualDir* childDir = dynamic_cast<VirtualDir*>(child))
		childDir->SetParent(this);

	return error;
}

// RemoveEntry
Node*
VirtualDir::RemoveEntry(const char* name)
{
	if (!name)
		return NULL;

	Node* child = NULL;
	VirtualDirEntry* entry = fEntries.Remove(name);
	if (entry) {
		child = entry->GetNode();
		// update the directory iterators pointing to the removed entry
		for (VirtualDirIterator* iterator = fIterators.First();
			 iterator;
			 iterator = fIterators.GetNext(iterator)) {
			if (iterator->GetCurrentEntry() == entry)
				iterator->NextEntry();
		}

		// remove the entry completely
		fEntryList.Remove(entry);
		delete entry;

		// TODO: See AddEntry().
		if (VirtualDir* childDir = dynamic_cast<VirtualDir*>(child))
			childDir->SetParent(NULL);
	}
	return child;
}

// GetEntry
VirtualDirEntry*
VirtualDir::GetEntry(const char* name) const
{
	if (!name)
		return NULL;

	return fEntries.Get(name);
}

// GetChildNode
Node*
VirtualDir::GetChildNode(const char* name) const
{
	if (VirtualDirEntry* entry = GetEntry(name))
		return entry->GetNode();
	return NULL;
}

// GetFirstEntry
VirtualDirEntry*
VirtualDir::GetFirstEntry() const
{
	return fEntryList.First();
}

// GetNextEntry
VirtualDirEntry*
VirtualDir::GetNextEntry(VirtualDirEntry* entry) const
{
	if (!entry)
		return NULL;
	return fEntryList.GetNext(entry);
}

// AddDirIterator
void
VirtualDir::AddDirIterator(VirtualDirIterator* iterator)
{
	if (!iterator)
		return;

	fIterators.Insert(iterator);
}

// RemoveDirIterator
void
VirtualDir::RemoveDirIterator(VirtualDirIterator* iterator)
{
	if (!iterator)
		return;

	fIterators.Remove(iterator);
}
