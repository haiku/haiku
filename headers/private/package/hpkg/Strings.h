/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef STRINGS_H
#define STRINGS_H


#include <util/OpenHashTable.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


uint32	hash_string(const char* string);


struct CachedString {
	char*			string;
	int32			index;
	uint32			usageCount;
	CachedString*	next;	// hash table link

	CachedString()
		:
		string(NULL),
		index(-1),
		usageCount(1)
	{
	}

	~CachedString()
	{
		free(string);
	}

	bool Init(const char* string)
	{
		this->string = strdup(string);
		if (this->string == NULL)
			return false;

		return true;
	}
};


struct CachedStringHashDefinition {
	typedef const char*		KeyType;
	typedef	CachedString	ValueType;

	size_t HashKey(const char* key) const
	{
		return hash_string(key);
	}

	size_t Hash(const CachedString* value) const
	{
		return HashKey(value->string);
	}

	bool Compare(const char* key, const CachedString* value) const
	{
		return strcmp(value->string, key) == 0;
	}

	CachedString*& GetLink(CachedString* value) const
	{
		return value->next;
	}
};


typedef BOpenHashTable<CachedStringHashDefinition> CachedStringTable;


struct CachedStringUsageGreater {
	bool operator()(const CachedString* a, const CachedString* b)
	{
		return a->usageCount > b->usageCount;
	}
};


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// STRINGS_H
