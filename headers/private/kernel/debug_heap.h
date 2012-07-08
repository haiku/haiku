/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_HEAP_H
#define _KERNEL_DEBUG_HEAP_H

#include <debug.h>


struct DebugAllocPool;
typedef struct DebugAllocPool debug_alloc_pool;


#ifdef __cplusplus
extern "C" {
#endif

debug_alloc_pool*	create_debug_alloc_pool();
void				delete_debug_alloc_pool(debug_alloc_pool* pool);
void*				debug_malloc(size_t size);
void*				debug_calloc(size_t num, size_t size);
void				debug_free(void* address);
void				debug_heap_init();

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

struct kdebug_alloc_t {};
extern const kdebug_alloc_t kdebug_alloc;

inline void*
operator new(size_t size, const kdebug_alloc_t&) throw()
{
	return debug_malloc(size);
}

namespace DebugAlloc {
	template<typename Type>
	inline void
	destroy(Type* object)
	{
		if (object != NULL) {
			object->~Type();
			debug_free(object);
				// NOTE: Doesn't work for multiple inheritence!
		}
	}
}

struct DebugAllocPoolScope {
	DebugAllocPoolScope()
	{
		fPool = create_debug_alloc_pool();
	}

	~DebugAllocPoolScope()
	{
		delete_debug_alloc_pool(fPool);
	}

private:
	DebugAllocPool*	fPool;
};

#endif	// __cplusplus

#endif	/* _KERNEL_DEBUG_HEAP_H */
