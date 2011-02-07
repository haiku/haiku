/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__STRINGS_H_
#define _PACKAGE__HPKG__PRIVATE__STRINGS_H_


#include <new>

#include <util/OpenHashTable.h>


namespace BPackageKit {

namespace BHPKG {

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


struct StringCache : public CachedStringTable {
	StringCache();
	~StringCache();

	CachedString* Get(const char* value);

};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__STRINGS_H_
