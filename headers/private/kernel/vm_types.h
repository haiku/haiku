/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_TYPES_H
#define _KERNEL_VM_TYPES_H


#include <arch/vm_types.h>
#include <arch/vm_translation_map.h>
#include <condition_variable.h>
#include <kernel.h>
#include <lock.h>
#include <util/DoublyLinkedQueue.h>

#include <sys/uio.h>


// Enables the vm_page::queue, i.e. it is tracked which queue the page should
// be in.
//#define DEBUG_PAGE_QUEUE	1

// Enables extra debug fields in the vm_page used to track page transitions
// between caches.
//#define DEBUG_PAGE_CACHE_TRANSITIONS	1

// Enables a global list of all vm_cache structures.
//#define DEBUG_CACHE_LIST 1

// uncomment to build in swap support
#define ENABLE_SWAP_SUPPORT 1

#ifdef __cplusplus

#include <util/SplayTree.h>

class AsyncIOCallback;
struct vm_page_mapping;
struct VMCache;
typedef DoublyLinkedListLink<vm_page_mapping> vm_page_mapping_link;

typedef struct vm_page_mapping {
	vm_page_mapping_link page_link;
	vm_page_mapping_link area_link;
	struct vm_page *page;
	struct vm_area *area;
} vm_page_mapping;

class DoublyLinkedPageLink {
	public:
		inline vm_page_mapping_link *operator()(vm_page_mapping *element) const
		{
			return &element->page_link;
		}

		inline const vm_page_mapping_link *operator()(const vm_page_mapping *element) const
		{
			return &element->page_link;
		}
};

class DoublyLinkedAreaLink {
	public:
		inline vm_page_mapping_link *operator()(vm_page_mapping *element) const
		{
			return &element->area_link;
		}

		inline const vm_page_mapping_link *operator()(const vm_page_mapping *element) const
		{
			return &element->area_link;
		}
};

typedef class DoublyLinkedQueue<vm_page_mapping, DoublyLinkedPageLink> vm_page_mappings;
typedef class DoublyLinkedQueue<vm_page_mapping, DoublyLinkedAreaLink> vm_area_mappings;

typedef uint32 page_num_t;

struct vm_page {
	struct vm_page		*queue_prev;
	struct vm_page		*queue_next;

	addr_t				physical_page_number;

	VMCache				*cache;
	page_num_t			cache_offset;
							// in page size units

	SplayTreeLink<vm_page>	cache_link;
	vm_page					*cache_next;

	vm_page_mappings	mappings;

#ifdef DEBUG_PAGE_QUEUE
	void*				queue;
#endif

#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
	uint32	debug_flags;
	struct vm_page *collided_page;
#endif

	uint8				type : 2;
	uint8				state : 3;

	uint8				is_cleared : 1;
		// is currently only used in vm_page_allocate_page_run()
	uint8				busy_writing : 1;
	uint8				merge_swap : 1;
		// used in VMAnonymousCache::Merge()

	int8				usage_count;
	uint16				wired_count;
};

enum {
	PAGE_TYPE_PHYSICAL = 0,
	PAGE_TYPE_DUMMY,
	PAGE_TYPE_GUARD
};

enum {
	PAGE_STATE_ACTIVE = 0,
	PAGE_STATE_INACTIVE,
	PAGE_STATE_BUSY,
	PAGE_STATE_MODIFIED,
	PAGE_STATE_FREE,
	PAGE_STATE_CLEAR,
	PAGE_STATE_WIRED,
	PAGE_STATE_UNUSED
};

enum {
	CACHE_TYPE_RAM = 0,
	CACHE_TYPE_VNODE,
	CACHE_TYPE_DEVICE,
	CACHE_TYPE_NULL
};

struct vm_dummy_page : vm_page {
	ConditionVariable	busy_condition;
};

struct VMCachePagesTreeDefinition {
	typedef page_num_t KeyType;
	typedef	vm_page NodeType;

	static page_num_t GetKey(const NodeType* node)
	{
		return node->cache_offset;
	}

	static SplayTreeLink<NodeType>* GetLink(NodeType* node)
	{
		return &node->cache_link;
	}

	static int Compare(page_num_t key, const NodeType* node)
	{
		return key == node->cache_offset ? 0
			: (key < node->cache_offset ? -1 : 1);
	}

	static NodeType** GetListLink(NodeType* node)
	{
		return &node->cache_next;
	}
};

typedef IteratableSplayTree<VMCachePagesTreeDefinition> VMCachePagesTree;

struct VMCache {
public:
	VMCache();
	virtual ~VMCache();

	status_t Init(uint32 cacheType);

	virtual	void		Delete();

			bool		Lock()
							{ return mutex_lock(&fLock) == B_OK; }
			bool		TryLock()
							{ return mutex_trylock(&fLock) == B_OK; }
			bool		SwitchLock(mutex* from)
							{ return mutex_switch_lock(from, &fLock) == B_OK; }
			void		Unlock();
			void		AssertLocked()
							{ ASSERT_LOCKED_MUTEX(&fLock); }

			void		AcquireRefLocked();
			void		AcquireRef();
			void		ReleaseRefLocked();
			void		ReleaseRef();
			void		ReleaseRefAndUnlock()
							{ ReleaseRefLocked(); Unlock(); }

			vm_page*	LookupPage(off_t offset);
			void		InsertPage(vm_page* page, off_t offset);
			void		RemovePage(vm_page* page);

			void		AddConsumer(VMCache* consumer);

			status_t	InsertAreaLocked(vm_area* area);
			status_t	RemoveArea(vm_area* area);

			status_t	WriteModified();
			status_t	SetMinimalCommitment(off_t commitment);
			status_t	Resize(off_t newSize);

			status_t	FlushAndRemoveAllPages();

			// for debugging only
			mutex*		GetLock()
							{ return &fLock; }
			int32		RefCount() const
							{ return fRefCount; }

	// backing store operations
	virtual	status_t	Commit(off_t size);
	virtual	bool		HasPage(off_t offset);

	virtual	status_t	Read(off_t offset, const iovec *vecs, size_t count,
							size_t *_numBytes);
	virtual	status_t	Write(off_t offset, const iovec *vecs, size_t count,
							uint32 flags, size_t *_numBytes);
	virtual	status_t	WriteAsync(off_t offset, const iovec* vecs,
							size_t count, size_t numBytes, uint32 flags,
							AsyncIOCallback* callback);

	virtual	status_t	Fault(struct vm_address_space *aspace, off_t offset);

	virtual	void		Merge(VMCache* source);

	virtual	status_t	AcquireUnreferencedStoreRef();
	virtual	void		AcquireStoreRef();
	virtual	void		ReleaseStoreRef();

private:
	inline	bool		_IsMergeable() const;

			void		_MergeWithOnlyConsumer();
			void		_RemoveConsumer(VMCache* consumer);


public:
	struct vm_area		*areas;
	struct list_link	consumer_link;
	struct list			consumers;
		// list of caches that use this cache as a source
	VMCachePagesTree	pages;
	VMCache				*source;
	off_t				virtual_base;
	off_t				virtual_end;
	off_t				committed_size;
		// TODO: Remove!
	uint32				page_count;
	uint32				temporary : 1;
	uint32				scan_skip : 1;
	uint32				type : 6;

#if DEBUG_CACHE_LIST
	struct VMCache*		debug_previous;
	struct VMCache*		debug_next;
#endif

private:
	int32				fRefCount;
	mutex				fLock;
};

typedef VMCache vm_cache;
	// TODO: Remove!


#if DEBUG_CACHE_LIST
extern vm_cache* gDebugCacheList;
#endif


class VMCacheFactory {
public:
	static	status_t	CreateAnonymousCache(VMCache*& cache,
							bool canOvercommit, int32 numPrecommittedPages,
							int32 numGuardPages, bool swappable);
	static	status_t	CreateVnodeCache(VMCache*& cache, struct vnode* vnode);
	static	status_t	CreateDeviceCache(VMCache*& cache, addr_t baseAddress);
	static	status_t	CreateNullCache(VMCache*& cache);
};


struct vm_area {
	char				*name;
	area_id				id;
	addr_t				base;
	addr_t				size;
	uint32				protection;
	uint16				wiring;
	uint16				memory_type;

	VMCache				*cache;
	vint32				no_cache_change;
	off_t				cache_offset;
	uint32				cache_type;
	vm_area_mappings	mappings;
	uint8				*page_protections;

	struct vm_address_space *address_space;
	struct vm_area		*address_space_next;
	struct vm_area		*cache_next;
	struct vm_area		*cache_prev;
	struct vm_area		*hash_next;
};

#endif	// __cplusplus

enum {
	VM_ASPACE_STATE_NORMAL = 0,
	VM_ASPACE_STATE_DELETION
};

struct vm_address_space {
	struct vm_area		*areas;
	struct vm_area		*area_hint;
	rw_lock				lock;
	addr_t				base;
	addr_t				size;
	int32				change_count;
	vm_translation_map	translation_map;
	team_id				id;
	int32				ref_count;
	int32				fault_count;
	int32				state;
	struct vm_address_space *hash_next;
};

#endif	/* _KERNEL_VM_TYPES_H */
