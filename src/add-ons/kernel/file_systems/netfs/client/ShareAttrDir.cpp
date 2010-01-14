// ShareAttrDir.cpp

#include <new>

#include <stdlib.h>
#include <string.h>

#include <Node.h>

#include "AttrDirInfo.h"
#include "AutoDeleter.h"
#include "ShareAttrDir.h"

// compare_attributes
//
// NULL is considered the maximum
static
int
compare_attributes(const Attribute* a, const Attribute* b)
{
	if (a == b)
		return 0;
	if (!a)
		return 1;
	if (!b)
		return -1;

	return strcmp(a->GetName(), b->GetName());
}

// compare_attributes
static
int
compare_attributes(const void* _a, const void* _b)
{
	return compare_attributes(*(const Attribute**)_a, *(const Attribute**)_b);
}

// compare_iterators
static
int
compare_iterators(const ShareAttrDirIterator* a, const ShareAttrDirIterator* b)
{
	return compare_attributes(a->GetCurrentAttribute(),
		b->GetCurrentAttribute());
}

// compare_iterators
static
int
compare_iterators(const void* _a, const void* _b)
{
	return compare_iterators(*(const ShareAttrDirIterator**)_a,
		*(const ShareAttrDirIterator**)_b);
}

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
ShareAttrDir::ShareAttrDir()
	: fAttributes(),
	  fRevision(-1),
	  fUpToDate(false)
{
}

// destructor
ShareAttrDir::~ShareAttrDir()
{
	ClearAttrDir();
}

// Init
status_t
ShareAttrDir::Init(const AttrDirInfo& dirInfo)
{
	if (!dirInfo.isValid)
		return B_BAD_VALUE;

	// get the attributes
	Attribute** attributes = NULL;
	int32 count = 0;
	status_t error = _GetAttributes(dirInfo, attributes, count);
	if (error != B_OK)
		return error;
	ArrayDeleter<Attribute*> _(attributes);

	// add the attributes
	for (int32 i = 0; i < count; i++)
		fAttributes.Insert(attributes[i]);

	fRevision = dirInfo.revision;
	fUpToDate = true;

	return B_OK;
}

// Update
status_t
ShareAttrDir::Update(const AttrDirInfo& dirInfo,
	DoublyLinkedList<ShareAttrDirIterator>* iterators)
{
	if (!dirInfo.isValid)
		return B_BAD_VALUE;

	if (fRevision >= dirInfo.revision)
		return B_OK;

	// allocate an array for the old attributes
	int32 oldCount = fAttributes.Size();
	Attribute** oldAttributes = new(std::nothrow) Attribute*[oldCount];
	if (!oldAttributes)
		return B_NO_MEMORY;
	ArrayDeleter<Attribute*> _(oldAttributes);

	// get the new attributes
	Attribute** newAttributes = NULL;
	int32 newCount = 0;
	status_t error = _GetAttributes(dirInfo, newAttributes, newCount);
	if (error != B_OK)
		return error;
	ArrayDeleter<Attribute*> _2(newAttributes);

	// sort the iterators
	int32 iteratorCount = (iterators ? iterators->Count() : 0);
	if (iteratorCount > 0) {
		// allocate an array
		ShareAttrDirIterator** _iterators
			= new(std::nothrow) ShareAttrDirIterator*[iteratorCount];
		if (!_iterators)
			return B_NO_MEMORY;
		ArrayDeleter<ShareAttrDirIterator*> _3(_iterators);

		// move the iterators
		for (int32 i = 0; i < iteratorCount; i++) {
			ShareAttrDirIterator* iterator = iterators->First();
			_iterators[i] = iterator;
			iterators->Remove(iterator);
		}

		// sort them
		qsort(_iterators, iteratorCount, sizeof(ShareAttrDirIterator*),
			compare_iterators);

		// move them back into the list
		for (int32 i = 0; i < iteratorCount; i++)
			iterators->Insert(_iterators[i]);
	}

	// remove the old attributes
	for (int32 i = 0; i < oldCount; i++) {
		Attribute* attribute = fAttributes.GetFirst();
		oldAttributes[i] = attribute;
		fAttributes.Remove(attribute);
	}

	// add the new attributes
	int32 oldIndex = 0;
	int32 newIndex = 0;
	ShareAttrDirIterator* iterator = (iterators ? iterators->First() : NULL);
	while (oldIndex < oldCount || newIndex < newCount) {
		Attribute* oldAttr = (oldCount > 0 ? oldAttributes[oldIndex] : NULL);
		Attribute* newAttr = (newCount > 0 ? newAttributes[newIndex] : NULL);
		int cmp = compare_attributes(oldAttr, newAttr);
		if (cmp < 0) {
			// oldAttr is obsolete: move all iterators pointing to it to the
			// next new attribute
			while (iterator && iterator->GetCurrentAttribute() == oldAttr) {
				iterator->SetCurrentAttribute(newAttr);
				iterator = iterators->GetNext(iterator);
			}
			oldIndex++;
		} else if (cmp > 0) {
			// newAttr is new
			fAttributes.Insert(newAttr);
			newIndex++;
		} else {
			// oldAttr == newAttr
			fAttributes.Insert(newAttr);
			oldIndex++;
			newIndex++;

			// move the attributes pointing to this attribute
			while (iterator && iterator->GetCurrentAttribute() == oldAttr) {
				iterator->SetCurrentAttribute(newAttr);
				iterator = iterators->GetNext(iterator);
			}
		}
	}

	// delete the old attributes
	for (int32 i = 0; i < oldCount; i++)
		Attribute::DeleteAttribute(oldAttributes[i]);

	fRevision = dirInfo.revision;
	fUpToDate = true;

	return B_OK;
}

// SetRevision
void
ShareAttrDir::SetRevision(int64 revision)
{
	fRevision = revision;
}

// GetRevision
int64
ShareAttrDir::GetRevision() const
{
	return fRevision;
}

// SetUpToDate
void
ShareAttrDir::SetUpToDate(bool upToDate)
{
	fUpToDate = upToDate;
}

// IsUpToDate
bool
ShareAttrDir::IsUpToDate() const
{
	return fUpToDate;
}

// ClearAttrDir
void
ShareAttrDir::ClearAttrDir()
{
	while (Attribute* attribute = GetFirstAttribute())
		RemoveAttribute(attribute);
}

// AddAttribute
status_t
ShareAttrDir::AddAttribute(const char* name, const attr_info& info,
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
ShareAttrDir::RemoveAttribute(const char* name)
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
ShareAttrDir::RemoveAttribute(Attribute* attribute)
{
	if (!attribute)
		return;

	fAttributes.Remove(attribute);
	Attribute::DeleteAttribute(attribute);
}

// GetAttribute
Attribute*
ShareAttrDir::GetAttribute(const char* name) const
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
ShareAttrDir::GetFirstAttribute() const
{
	return fAttributes.GetFirst();
}

// GetNextAttribute
Attribute*
ShareAttrDir::GetNextAttribute(Attribute* attribute) const
{
	return (attribute ? fAttributes.GetNext(attribute) : NULL);
}

// _GetAttributes
status_t
ShareAttrDir::_GetAttributes(const AttrDirInfo& dirInfo,
	Attribute**& _attributes, int32& _count)
{
	if (!dirInfo.isValid)
		return B_BAD_VALUE;

	int32 count = dirInfo.attributeInfos.CountElements();
	const AttributeInfo* attrInfos = dirInfo.attributeInfos.GetElements();

	// allocate an attribute array
	Attribute** attributes = NULL;
	if (count > 0) {
		attributes = new(std::nothrow) Attribute*[count];
		if (!attributes)
			return B_NO_MEMORY;
		memset(attributes, 0, sizeof(Attribute*) * count);
	}

	status_t error = B_OK;
	for (int32 i = 0; i < count; i++) {
		const AttributeInfo& attrInfo = attrInfos[i];

		// create the attribute
		const void* data = attrInfo.data.GetData();
		if (data && attrInfo.data.GetSize() != attrInfo.info.size)
			data = NULL;
		error = Attribute::CreateAttribute(attrInfo.name.GetString(),
			attrInfo.info, data, attributes + i);
		if (error != B_OK)
			break;
	}

	// cleanup on error
	if (error != B_OK) {
		for (int32 i = 0; i < count; i++) {
			if (Attribute* attribute = attributes[i])
				Attribute::DeleteAttribute(attribute);
		}
		delete[] attributes;

		return error;
	}

	// sort the attribute array
	if (count > 0)
		qsort(attributes, count, sizeof(Attribute*), compare_attributes);

	_attributes = attributes;
	_count = count;
	return B_OK;
}

