// ShareAttrDirIterator.cpp

#include "ShareAttrDirIterator.h"

#include "ShareAttrDir.h"

// constructor
ShareAttrDirIterator::ShareAttrDirIterator()
	:
	fAttrDir(NULL),
	fCurrentAttribute(NULL)
{
}

// destructor
ShareAttrDirIterator::~ShareAttrDirIterator()
{
}

// SetAttrDir
void
ShareAttrDirIterator::SetAttrDir(ShareAttrDir* attrDir)
{
	fAttrDir = attrDir;
	fCurrentAttribute = (fAttrDir ? fAttrDir->GetFirstAttribute() : NULL);
}

// SetCurrentAttribute
void
ShareAttrDirIterator::SetCurrentAttribute(Attribute* attribute)
{
	fCurrentAttribute = attribute;
}

// GetCurrentAttribute
Attribute*
ShareAttrDirIterator::GetCurrentAttribute() const
{
	return fCurrentAttribute;
}

// NextAttribute
Attribute*
ShareAttrDirIterator::NextAttribute()
{
	if (fAttrDir && fCurrentAttribute)
		fCurrentAttribute = fAttrDir->GetNextAttribute(fCurrentAttribute);
	return fCurrentAttribute;
}

// Rewind
void
ShareAttrDirIterator::Rewind()
{
	fCurrentAttribute = (fAttrDir ? fAttrDir->GetFirstAttribute() : NULL);
}

