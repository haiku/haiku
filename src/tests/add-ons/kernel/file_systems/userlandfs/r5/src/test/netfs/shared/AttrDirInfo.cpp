// AttrDirInfo.cpp

#include "AttrDirInfo.h"

// ShowAround
void
AttributeInfo::ShowAround(RequestMemberVisitor* visitor)
{
	visitor->Visit(this, name);
	visitor->Visit(this, info.type);
	visitor->Visit(this, info.size);
	visitor->Visit(this, data);
}


// #pragma mark -

// constructor
AttrDirInfo::AttrDirInfo()
	: revision(-1),
	  isValid(false)
{
}

// ShowAround
void
AttrDirInfo::ShowAround(RequestMemberVisitor* visitor)
{
	visitor->Visit(this, isValid);
	if (isValid) {
		visitor->Visit(this, revision);
		visitor->Visit(this, attributeInfos);
	}
}

// Flatten
status_t
AttrDirInfo::Flatten(RequestFlattener* flattener)
{
	flattener->WriteBool(isValid);
	if (isValid) {
		flattener->Visit(this, revision);
		flattener->Visit(this, attributeInfos);
	}

	return flattener->GetStatus();
}

// Unflatten
status_t
AttrDirInfo::Unflatten(RequestUnflattener* unflattener)
{
	if (unflattener->ReadBool(isValid) == B_OK && isValid) {
		unflattener->Visit(this, revision);
		unflattener->Visit(this, attributeInfos);
	}

	return unflattener->GetStatus();
}

