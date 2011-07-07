/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef INDEX_H
#define INDEX_H


#include <string.h>

#include <SupportDefs.h>

#include <util/khash.h>
#include <util/OpenHashTable.h>


class AbstractIndexIterator;
class IndexIterator;
class Node;
class Volume;


static const size_t kMaxIndexKeyLength = 256;


class Index {
public:
								Index();
	virtual						~Index();

			status_t			Init(Volume* volume, const char* name,
									uint32 type, bool fixedKeyLength,
									size_t keyLength = 0);

			Volume*				GetVolume() const		{ return fVolume; }

			const char*			Name() const			{ return fName; }
			uint32				Type() const			{ return fType; }
			bool				HasFixedKeyLength() const
									{ return fFixedKeyLength; }
			size_t				KeyLength() const		{ return fKeyLength; }

	virtual	int32				CountEntries() const = 0;

			bool				GetIterator(IndexIterator* iterator);
			bool				Find(const void* key, size_t length,
									IndexIterator* iterator);

			Index*&				IndexHashLink()
									{ return fHashLink; }

			// debugging
			void				Dump();

protected:
	virtual	AbstractIndexIterator* InternalGetIterator() = 0;
	virtual	AbstractIndexIterator* InternalFind(const void* key,
									size_t length) = 0;

protected:
			Index*				fHashLink;
			Volume*				fVolume;
			char*				fName;
			uint32				fType;
			size_t				fKeyLength;
			bool				fFixedKeyLength;
};


class IndexIterator {
public:
								IndexIterator();
								IndexIterator(Index* index);
								~IndexIterator();

			bool				HasNext() const;
			Node*				Next();
			Node*				Next(void* buffer, size_t* _keyLength);

			status_t			Suspend();
			status_t			Resume();

private:
			void				SetIterator(AbstractIndexIterator* iterator);

private:
			friend class Index;

private:
			AbstractIndexIterator* fIterator;
};


// #pragma mark - IndexHashDefinition


struct IndexHashDefinition {
	typedef const char*	KeyType;
	typedef	Index		ValueType;

	size_t HashKey(const char* key) const
	{
		return key != NULL ? hash_hash_string(key) : 0;
	}

	size_t Hash(const Index* value) const
	{
		return HashKey(value->Name());
	}

	bool Compare(const char* key, const Index* value) const
	{
		return strcmp(value->Name(), key) == 0;
	}

	Index*& GetLink(Index* value) const
	{
		return value->IndexHashLink();
	}
};


typedef BOpenHashTable<IndexHashDefinition> IndexHashTable;


#endif	// INDEX_H
