// AttributeDirectory.cpp

#include <new>

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>
#include <Node.h>

#include "AttributeDirectory.h"

// small data size
static const int32 kSmallAttributeSize = 8;

// constructor
Attribute::Attribute(const char* name, const attr_info& info,
	const void* data)
	: fInfo(info)
{
	char* nameBuffer = fDataAndName;

	// copy data, if any
	if (data) {
		nameBuffer += info.size;
		memcpy(fDataAndName, data, info.size);

		// store a negative size to indicate we also have the data
		fInfo.size = -info.size;
	}

	// copy the name
	strcpy(nameBuffer, name);
}

// destructor
Attribute::~Attribute()
{
}

// CreateAttribute
status_t
Attribute::CreateAttribute(const char* name, const attr_info& info,
	const void* data, Attribute** attribute)
{
	if (!name || !attribute)
		return B_BAD_VALUE;

	// compute the size
	int32 nameLen = strlen(name);
	int32 size = sizeof(Attribute) + nameLen;
	if (data)
		size += info.size;

	void* buffer = malloc(size);
	if (!buffer)
		return B_NO_MEMORY;

	*attribute = new(buffer) Attribute(name, info, data);
	return B_OK;
}

// DeleteAttribute
void
Attribute::DeleteAttribute(Attribute* attribute)
{
	if (attribute) {
		attribute->~Attribute();
		free(attribute);
	}
}

// GetName
const char*
Attribute::GetName() const
{
	return (fInfo.size >= 0 ? fDataAndName : fDataAndName - fInfo.size);
}

// GetInfo
void
Attribute::GetInfo(attr_info* info) const
{
	if (info) {
		info->type = fInfo.type;
		info->size = GetSize();
	}
}

// GetType
uint32
Attribute::GetType() const
{
	return fInfo.type;
}

// GetSize
off_t
Attribute::GetSize() const
{
	return (fInfo.size >= 0 ? fInfo.size : -fInfo.size);
}

// GetData
const void*
Attribute::GetData() const
{
	return (fInfo.size >= 0 ? NULL : fDataAndName);
}


// #pragma mark -

// constructor
AttributeDirectory::AttributeDirectory()
	: fAttributes(),
	  fStatus(ATTRIBUTE_DIRECTORY_NOT_LOADED)
{
}

// destructor
AttributeDirectory::~AttributeDirectory()
{
	ClearAttrDir();
}

// GetAttrDirStatus
uint32
AttributeDirectory::GetAttrDirStatus() const
{
	return fStatus;
}

// IsAttrDirValid
bool
AttributeDirectory::IsAttrDirValid() const
{
	return (fStatus == ATTRIBUTE_DIRECTORY_VALID);
}

// LoadAttrDir
status_t
AttributeDirectory::LoadAttrDir()
{
	// nothing to do, if already loaded
	if (fStatus == ATTRIBUTE_DIRECTORY_VALID)
		return B_OK;

	// if we tried earlier and the attr dir was too big, we fail again
	if (fStatus == ATTRIBUTE_DIRECTORY_TOO_BIG)
		return B_ERROR;

	// open the node
	BNode node;
	status_t error = OpenNode(node);
	if (error != B_OK)
		return error;

	// iterate through the attribute directory
	char name[B_ATTR_NAME_LENGTH];
	while (node.GetNextAttrName(name) == B_OK) {
		// get the attribute data
		attr_info info;
		char data[kSmallAttributeSize];
		bool dataLoaded = false;
		error = _LoadAttribute(node, name, info, data, dataLoaded);
		if (error != B_OK)
			break;

		// create the attribute
		error = AddAttribute(name, info, (dataLoaded ? data : NULL));
	}

	if (error != B_OK)
		ClearAttrDir();
// TODO: Enforce maximum attribute directory size.

	return error;
}

// ClearAttrDir
void
AttributeDirectory::ClearAttrDir()
{
	while (Attribute* attribute = GetFirstAttribute())
		RemoveAttribute(attribute);
}

// AddAttribute
status_t
AttributeDirectory::AddAttribute(const char* name, const attr_info& info,
	const void* data)
{
	if (!name || GetAttribute(name))
		return B_BAD_VALUE;

	// create the attribute
	Attribute* attribute;
	status_t error = Attribute::CreateAttribute(name, info, data, &attribute);
	if (error != B_OK)
		return error;

	// add the attribute
	fAttributes.Insert(attribute);

	return B_OK;
}

// RemoveAttribute
bool
AttributeDirectory::RemoveAttribute(const char* name)
{
	if (!name)
		return false;

	for (SLList<Attribute>::Iterator it = fAttributes.GetIterator();
		 it.HasNext();) {
		Attribute* attribute = it.Next();
		if (strcmp(attribute->GetName(), name) == 0) {
			it.Remove();
			Attribute::DeleteAttribute(attribute);
			return true;
		}
	}

	return false;
}

// RemoveAttribute
void
AttributeDirectory::RemoveAttribute(Attribute* attribute)
{
	if (!attribute)
		return;

	fAttributes.Remove(attribute);
	Attribute::DeleteAttribute(attribute);
}

// UpdateAttribute
status_t
AttributeDirectory::UpdateAttribute(const char* name, bool* removed,
	attr_info* _info, const void** _data)
{
	if (!name || !removed)
		return B_BAD_VALUE;

	// open the node
	BNode node;
	status_t error = OpenNode(node);
	if (error != B_OK) {
		ClearAttrDir();
		if (fStatus == ATTRIBUTE_DIRECTORY_VALID)
			fStatus = ATTRIBUTE_DIRECTORY_NOT_LOADED;
		return error;
	}

	// get the attribute data
	attr_info info;
	char data[kSmallAttributeSize];
	bool dataLoaded = false;
	error = _LoadAttribute(node, name, info,
		(fStatus == ATTRIBUTE_DIRECTORY_VALID ? data : NULL), dataLoaded);
	if (error == B_OK) {
		if (fStatus == ATTRIBUTE_DIRECTORY_VALID) {
			// remove the attribute
			Attribute* previous = NULL;
			for (SLList<Attribute>::Iterator it = fAttributes.GetIterator();
				 it.HasNext();) {
				Attribute* attribute = it.Next();
				if (strcmp(attribute->GetName(), name) == 0) {
					it.Remove();
					Attribute::DeleteAttribute(attribute);
					break;
				}
				previous = attribute;
			}

// TODO: Enforce the maximum attr dir size.
			// re-create the attribute
			Attribute* attribute;
			error = Attribute::CreateAttribute(name, info, data,
				&attribute);
			if (error == B_OK) {
				// add the attribute
				fAttributes.InsertAfter(previous, attribute);

				// return the desired info
				if (_info)
					attribute->GetInfo(_info);
				if (_data)
					*_data = attribute->GetData();
				*removed = false;
			}
		} else if (error == B_OK) {
			if (_info)
				*_info = info;
			if (_data)
				*_data = NULL;
			*removed = false;
		}
	} else {
		*removed = true;
		RemoveAttribute(name);
		error = B_OK;
	}

	// clean up on error
	if (error != B_OK) {
		ClearAttrDir();
		if (fStatus == ATTRIBUTE_DIRECTORY_VALID)
			fStatus = ATTRIBUTE_DIRECTORY_NOT_LOADED;
	}

	return error;
}

// GetAttribute
Attribute*
AttributeDirectory::GetAttribute(const char* name) const
{
	if (!name)
		return NULL;

	for (SLList<Attribute>::ConstIterator it = fAttributes.GetIterator();
		 it.HasNext();) {
		Attribute* attribute = it.Next();
		if (strcmp(attribute->GetName(), name) == 0)
			return attribute;
	}

	return false;
}

// GetFirstAttribute
Attribute*
AttributeDirectory::GetFirstAttribute() const
{
	return fAttributes.GetFirst();
}

// GetNextAttribute
Attribute*
AttributeDirectory::GetNextAttribute(Attribute* attribute) const
{
	return (attribute ? fAttributes.GetNext(attribute) : NULL);
}

// _LoadAttribute
status_t
AttributeDirectory::_LoadAttribute(BNode& node, const char* name,
	attr_info& info, void* data, bool& dataLoaded)
{
	// stat the attribute
	status_t error = node.GetAttrInfo(name, &info);
	if (error != B_OK)
		return error;

	// if the data are small enough, read them
	if (data && info.size <= kSmallAttributeSize) {
		ssize_t bytesRead = node.ReadAttr(name, info.type, 0, data,
			info.size);
		dataLoaded = (bytesRead == info.size);
	}

	return B_OK;
}

