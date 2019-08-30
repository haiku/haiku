/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef ATTRIBUTE_INDEX_IMPL_H
#define ATTRIBUTE_INDEX_IMPL_H

#include "AttributeIndex.h"

// AttributeIndexImpl
class AttributeIndexImpl : public AttributeIndex {
public:
	AttributeIndexImpl(Volume *volume, const char *name, uint32 type,
					   size_t keyLength);
	virtual ~AttributeIndexImpl();

	virtual int32 CountEntries() const;

	virtual status_t Changed(Attribute *attribute,
							 const uint8 *oldKey, size_t oldLength);

private:
	virtual status_t Added(Attribute *attribute);
	virtual bool Removed(Attribute *attribute);

protected:
	virtual AbstractIndexEntryIterator *InternalGetIterator();
	virtual AbstractIndexEntryIterator *InternalFind(const uint8 *key,
													 size_t length);

private:
	class Iterator;
	class IteratorList;
	class AttributeTree;

	class PrimaryKey;
	class GetPrimaryKey;
	class PrimaryKeyCompare;

	friend class Iterator;

private:
	void _AddIterator(Iterator *iterator);
	void _RemoveIterator(Iterator *iterator);

private:
	AttributeTree	*fAttributes;
	IteratorList	*fIterators;
};

#endif	// ATTRIBUTE_INDEX_IMPL_H
