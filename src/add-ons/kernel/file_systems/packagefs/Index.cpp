/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Index.h"

#include "DebugSupport.h"
#include "Directory.h"
#include "Node.h"
#include "IndexImpl.h"


// #pragma mark - Index


Index::Index()
	:
	fVolume(NULL),
	fName(NULL),
	fType(0),
	fKeyLength(0),
	fFixedKeyLength(true)
{
}


Index::~Index()
{
	free(fName);
}


status_t
Index::Init(Volume* volume, const char* name, uint32 type, bool fixedKeyLength,
	size_t keyLength)
{
	fName = strdup(name);
	if (fName == NULL)
		return B_NO_MEMORY;

	fVolume = volume;
	fType = type;
	fKeyLength = keyLength;
	fFixedKeyLength = fixedKeyLength;

	return B_OK;
}


bool
Index::GetIterator(IndexIterator& iterator)
{
	AbstractIndexIterator* actualIterator = InternalGetIterator();
	iterator.SetIterator(actualIterator);

	return actualIterator != NULL;
}


bool
Index::Find(const void* key, size_t length, IndexIterator& iterator)
{
	AbstractIndexIterator* actualIterator
		= key != NULL ? InternalFind(key, length) : NULL;
	iterator.SetIterator(actualIterator);

	return actualIterator != NULL;
}


void
Index::Dump()
{
	D(
		PRINT("Index: `%s', type: %" B_PRIx32 "\n", Name(), Type());
		IndexIterator it;
		if (GetIterator(it)) {
			while (Node* node = it.Next()) {
				PRINT("  node: `%s', dir: %" B_PRIdINO "\n", node->Name(),
					node->Parent()->ID());
			}
		}
	)
}


// #pragma mark - IndexIterator


IndexIterator::IndexIterator()
	:
	fIterator(NULL)
{
}


IndexIterator::~IndexIterator()
{
	SetIterator(NULL);
}


bool
IndexIterator::HasNext() const
{
	return fIterator != NULL && fIterator->HasNext();
}


Node*
IndexIterator::Next()
{
	return fIterator != NULL ? fIterator->Next(NULL, NULL) : NULL;
}


Node*
IndexIterator::Next(void* buffer, size_t* _keyLength)
{
	return fIterator != NULL ? fIterator->Next(buffer, _keyLength) : NULL;
}


status_t
IndexIterator::Suspend()
{
	return fIterator != NULL ? fIterator->Suspend() : B_BAD_VALUE;
}


status_t
IndexIterator::Resume()
{
	return fIterator != NULL ? fIterator->Resume() : B_BAD_VALUE;
}


void
IndexIterator::SetIterator(AbstractIndexIterator* iterator)
{
	delete fIterator;
	fIterator = iterator;
}


// #pragma mark - AbstractIndexIterator


AbstractIndexIterator::AbstractIndexIterator()
{
}


AbstractIndexIterator::~AbstractIndexIterator()
{
}


status_t
AbstractIndexIterator::Suspend()
{
	return B_OK;
}


status_t
AbstractIndexIterator::Resume()
{
	return B_OK;
}

