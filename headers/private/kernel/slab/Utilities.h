/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_UTILITIES_H_
#define _SLAB_UTILITIES_H_

#include <slab/Base.h>
#include <slab/MergedStrategy.h>


template<typename Type, typename Backend>
class TypedCache : public Cache< MergedLinkCacheStrategy<Backend> > {
public:
	typedef MergedLinkCacheStrategy<Backend> Strategy;
	typedef Cache<Strategy> BaseType;

	TypedCache(const char *name, size_t alignment)
		: BaseType(name, sizeof(Type), alignment, _ConstructObject,
			_DestructObject, this) {}
	virtual ~TypedCache() {}

	Type *Alloc(uint32_t flags) { return (Type *)BaseType::AllocateObject(flags); }
	void Free(Type *object) { BaseType::ReturnObject(object); }

private:
	static status_t _ConstructObject(void *cookie, void *object)
	{
		return ((TypedCache *)cookie)->ConstructObject((Type *)object);
	}

	static void _DestructObject(void *cookie, void *object)
	{
		((TypedCache *)cookie)->DestructObject((Type *)object);
	}

	virtual status_t ConstructObject(Type *object) { return B_OK; }
	virtual void DestructObject(Type *object) {}
};

#endif
