// EntryRef.cpp

#include "EntryRef.h"
#include "String.h"

// constructor
EntryRef::EntryRef()
	: entry_ref()
{
}

// constructor
EntryRef::EntryRef(dev_t volumeID, ino_t nodeID, const char* name)
	: entry_ref(volumeID, nodeID, name)
{
}

// constructor
EntryRef::EntryRef(const entry_ref& ref)
	: entry_ref(ref)
{
}

// InitCheck
status_t
EntryRef::InitCheck() const
{
	if (device < 0 || directory < 0)
		return B_NO_INIT;
	return (name ? B_OK : B_NO_MEMORY);
}

// GetHashCode
uint32
EntryRef::GetHashCode() const
{
	uint32 hash = device;
	hash = 17 * hash + (uint32)(directory >> 32);
	hash = 17 * hash + (uint32)directory;
	hash = 17 * hash + string_hash(name);
	return hash;
}


// #pragma mark -

// constructor
NoAllocEntryRef::NoAllocEntryRef()
	: EntryRef()
{
}

// constructor
NoAllocEntryRef::NoAllocEntryRef(dev_t volumeID, ino_t nodeID, const char* name)
	: EntryRef()
{
	device = volumeID;
	directory = nodeID;
	this->name = const_cast<char*>(name);
}

// constructor
NoAllocEntryRef::NoAllocEntryRef(const entry_ref& ref)
	: EntryRef()
{
	device = ref.device;
	directory = ref.directory;
	this->name = ref.name;
}

// destructor
NoAllocEntryRef::~NoAllocEntryRef()
{
	name = NULL;
}

// =
NoAllocEntryRef&
NoAllocEntryRef::operator=(const entry_ref& ref)
{
	device = ref.device;
	directory = ref.directory;
	this->name = ref.name;
	return *this;
}

