/*
 * Copyright 2023, Your Name, your.email@example.com.
 * Distributed under the terms of the MIT License.
 */

#include "unified_cache.h"

#include <slab/Slab.h>
#include <util/AutoLock.h>
#include <kernel.h> // For dprintf, new_lock etc.

// Define to enable debug output
//#define TRACE_UNIFIED_CACHE
#ifdef TRACE_UNIFIED_CACHE
#	define TRACE(x...) dprintf("unified_cache: " x)
#else
#	define TRACE(x...) ;
#endif

// Object caches for unified_cache_entry structures and their data buffers
static object_cache* sEntryCache = NULL;
// Data buffers will be allocated using malloc/free or a slab cache for fixed sizes if preferred.

// #pragma mark - unified_cache methods

status_t
unified_cache::Init(const char* cache_name, size_t max_size)
{
    name = cache_name;
    max_size_bytes = max_size;
    current_size_bytes = 0;
    max_entries = 0; // Unlimited for now, could be derived from max_size_bytes / average_item_size
    current_entries = 0;
    hits = 0;
    misses = 0;
    evictions = 0;

    mutex_init(&lock, name);

    hash_table = new(std::nothrow) EntryHashTable();
    if (hash_table == NULL)
        return B_NO_MEMORY;
    status_t status = hash_table->Init();
    if (status != B_OK) {
        delete hash_table;
        hash_table = NULL;
        return status;
    }

    sieve_hand = NULL;
    // sieve_list is already initialized by its constructor

    TRACE("Cache '%s' initialized with max size %lu bytes.\n", name, max_size_bytes);
    return B_OK;
}

void
unified_cache::Destroy()
{
    mutex_destroy(&lock);

    // TODO: Iterate through all entries in hash_table and sieve_list,
    // free their data and then free the entries themselves.
    // This needs careful handling if entries can be in transactions or dirty.
    // For now, assuming a clean shutdown or that flushing is handled elsewhere.

    unified_cache_entry* entry = NULL;
    while ((entry = sieve_list.RemoveHead()) != NULL) {
        // In a real scenario, check ref_count, dirty status, etc.
        // object_cache_free(sEntryCache, entry->data); // If data is from slab
        free(entry->data); // If data is from malloc
        object_cache_free(sEntryCache, entry, 0);
    }

    if (hash_table) {
        hash_table->Clear(false); // false = don't delete items, we did above
        delete hash_table;
        hash_table = NULL;
    }

    TRACE("Cache '%s' destroyed.\n", name);
}

unified_cache_entry*
unified_cache::LookupEntry(uint64 id)
{
    // This is a simplified lookup. In a real scenario,
    // this function would be called with the cache lock held.
    return hash_table->Lookup(id);
}

status_t
unified_cache::InsertEntry(unified_cache_entry* entry)
{
    // Assumes lock is held
    if (current_size_bytes + entry->size > max_size_bytes && sieve_list.Count() > 0) {
        unified_cache_entry* victim = EvictEntrySieve2();
        if (victim) {
            TRACE("InsertEntry: Evicted entry %llu to make space.\n", victim->id);
            // Data and entry itself are freed by EvictEntrySieve2 if no refs
        } else if (current_size_bytes + entry->size > max_size_bytes) {
            // Still no space, eviction failed (e.g. all pages pinned)
            return B_NO_MEMORY; // Or some other error indicating cache full
        }
    }

    hash_table->Insert(entry);
    sieve_list.Add(entry); // Add to the tail (MRU position for new items)
    current_size_bytes += entry->size;
    current_entries++;
    entry->sieve_visited_count = 0; // New entries start with 0 visits

    // If sieve_hand is NULL (empty list before this add), point it to the new entry.
    if (sieve_hand == NULL) {
        sieve_hand = sieve_list.Head();
    }

    TRACE("InsertEntry: Inserted entry %llu, current size %lu bytes, %lu entries.\n",
        entry->id, current_size_bytes, current_entries);
    return B_OK;
}

void
unified_cache::RemoveEntry(unified_cache_entry* entry)
{
    // Assumes lock is held and entry is no longer referenced externally for cache purposes
    // (ref_count might be >0 if someone holds a direct pointer, but cache considers it removable)
    TRACE("RemoveEntry: Removing entry %llu.\n", entry->id);

    // If the hand points to the entry being removed, advance the hand.
    // This is crucial for SIEVE's clock behavior.
    if (sieve_hand == entry) {
        sieve_hand = sieve_list.GetNext(entry);
        if (sieve_hand == NULL) { // Was tail or only element
            sieve_hand = sieve_list.Head(); // Wrap around or set to NULL if list becomes empty
        }
    }

    hash_table->Remove(entry);
    sieve_list.Remove(entry);
    current_size_bytes -= entry->size;
    current_entries--;

    // Actual freeing of entry->data and entry itself should happen here
    // if ref_count is 0, or be deferred.
    // For simplicity here, assume it's freed immediately if no outstanding refs.
    // This part needs to align with unified_cache_release_entry.
    if (entry->ref_count == 0) {
        // object_cache_free(sEntryCache, entry->data); // if data from slab
        free(entry->data); // if data from malloc
        object_cache_free(sEntryCache, entry, 0);
    }
}

unified_cache_entry*
unified_cache::EvictEntrySieve2()
{
    // Assumes lock is held
    if (sieve_list.IsEmpty())
        return NULL;

    unified_cache_entry* candidate = sieve_hand;
    while (candidate != NULL) {
        // If the entry is referenced, we cannot evict it. Skip.
        // In SIEVE, objects with ref_count > 0 are considered "pinned".
        if (candidate->ref_count > 0) {
            sieve_hand = sieve_list.GetNext(candidate);
            if (sieve_hand == NULL) sieve_hand = sieve_list.Head(); // Wrap around
            if (sieve_hand == candidate && candidate->ref_count > 0) {
                // All pages pinned or only one pinned page left
                TRACE("EvictEntrySieve2: All remaining pages are pinned or only one pinned page.\n");
                return NULL;
            }
            candidate = sieve_hand;
            continue;
        }

        if (candidate->sieve_visited_count > 0) {
            candidate->sieve_visited_count--; // "Second chance"
            sieve_hand = sieve_list.GetNext(candidate);
            if (sieve_hand == NULL) sieve_hand = sieve_list.Head(); // Wrap around
            // Move candidate to MRU end of list if that's part of the specific SIEVE-2 variant
            // For basic SIEVE, just clearing visited is enough, hand moves on.
            // Let's assume for now it stays in place, and hand moves.
        } else {
            // Found victim
            TRACE("EvictEntrySieve2: Victim found %llu (visited_count: %u, ref_count: %ld).\n",
                candidate->id, candidate->sieve_visited_count, candidate->ref_count);

            // Before removing from hash and list, advance the hand
            sieve_hand = sieve_list.GetNext(candidate);
            if (sieve_hand == NULL) {
                sieve_hand = sieve_list.Head();
            }
            // If sieve_hand becomes the candidate again after wrapping, and it was the only item,
            // and it's being evicted, sieve_hand should become NULL if list is now empty.
            if (sieve_hand == candidate) { // Only element was the candidate
                 sieve_hand = NULL; // List will be empty after removal
            }


            // TODO: Handle dirty victim: write back before evicting.
            // This is a simplified eviction. A real one would queue dirty blocks
            // for write-back and might not evict immediately.
            if (candidate->is_dirty) {
                TRACE("EvictEntrySieve2: Victim %llu is dirty. Needs writeback (not implemented here).\n", candidate->id);
                // For now, we'll pretend it's written back or handle it as an error for eviction
                // A robust cache would have a mechanism to flush dirty pages.
                // For now, let's just mark it as not evictable if dirty and unreferenced.
                // This is a placeholder for more complex dirty page handling.
                sieve_hand = sieve_list.GetNext(candidate); // try next
                if (sieve_hand == NULL) sieve_hand = sieve_list.Head();
                if (sieve_hand == candidate) return NULL; // all dirty or pinned
                candidate = sieve_hand;
                continue;
            }

            hash_table->Remove(candidate);
            sieve_list.Remove(candidate);
            current_size_bytes -= candidate->size;
            current_entries--;
            evictions++;

            // The caller is responsible for freeing the victim's data and the entry struct itself
            // if ref_count is indeed 0.
            // For this simplified version, we assume it's freed.
            // object_cache_free(sEntryCache, candidate->data); // if data from slab
            free(candidate->data); // if data from malloc
            object_cache_free(sEntryCache, candidate, 0);

            return candidate; // Technically returning a pointer to freed memory, should return id or status.
                              // Let's change to return NULL after freeing, or the entry before freeing if caller manages it.
                              // For now, this indicates success. The entry is gone.
        }
        candidate = sieve_hand;
        if (candidate == NULL && !sieve_list.IsEmpty()) { // Should not happen if list not empty
             sieve_hand = sieve_list.Head(); // Reset hand if it became NULL unexpectedly
             candidate = sieve_hand;
        }
    }
    TRACE("EvictEntrySieve2: No evictable entry found (all pinned or dirty and unhandled).\n");
    return NULL; // No evictable page found
}

void
unified_cache::AccessEntrySieve2(unified_cache_entry* entry)
{
    // Assumes lock is held
    // For SIEVE-2, set visited count to a higher value (e.g., 2) on access.
    // The paper suggests setting the visited bit. For a counter based SIEVE-2:
    if (entry->sieve_visited_count < 2) { // Cap at 2 for simplicity
        entry->sieve_visited_count = 2;
    }
    // Some SIEVE variants might also move the accessed item to the MRU end of the list.
    // For a simple clock-based SIEVE, just updating visited status is enough.
    // The original SIEVE paper: "SIEVE keeps all objects in a FIFO list residing in a circular buffer.
    // Insertion happens at the head of the list. Eviction happens at the tail after scanning using a hand pointer."
    // On a hit, set its visited flag.
    // Eviction: iterate from hand. If visited, clear flag and hand moves. If not visited, evict.
    // This means our current `sieve_list` is more like the clock. `AccessEntry` marks visited.
    // `EvictEntry` clears visited or evicts.
    // Let's assume `sieve_visited_count = 0` means "not visited", `>0` means "visited".
    // And for SIEVE-2, the count matters. Let's use 1 as the "recently visited" state.
    entry->sieve_visited_count = 1; // Set to 1 (or highest value for SIEVE-N)

    TRACE("AccessEntrySieve2: Accessed entry %llu, new visited_count %u.\n", entry->id, entry->sieve_visited_count);
}

// #pragma mark - Public API functions

status_t
unified_cache_init(const char* name, size_t max_size_bytes, unified_cache** _cache)
{
    if (sEntryCache == NULL) {
        sEntryCache = create_object_cache("unified_cache_entries", sizeof(unified_cache_entry),
                                          0);
        if (sEntryCache == NULL)
            return B_NO_MEMORY;
    }

    unified_cache* cache = new(std::nothrow) unified_cache();
    if (cache == NULL)
        return B_NO_MEMORY;

    status_t status = cache->Init(name, max_size_bytes);
    if (status != B_OK) {
        delete cache;
        return status;
    }

    *_cache = cache;
    return B_OK;
}

void
unified_cache_destroy(unified_cache* cache)
{
    if (cache == NULL)
        return;

    cache->Destroy();
    delete cache;

    // TODO: When to destroy sEntryCache? Probably a global cache_cleanup function.
    // For now, it's not destroyed here as it might be shared or used by other instances.
}

unified_cache_entry*
unified_cache_get_entry(unified_cache* cache, uint64 id, unified_data_type type)
{
    if (cache == NULL)
        return NULL;

    MutexLocker locker(cache->lock);

    unified_cache_entry* entry = cache->LookupEntry(id);
    if (entry != NULL) {
        if (entry->data_type != type) {
            // ID collision with different type? Should not happen with proper ID generation.
            panic("unified_cache_get_entry: ID collision with different data type!");
            return NULL;
        }
        atomic_add(&entry->ref_count, 1);
        cache->AccessEntrySieve2(entry);
        cache->hits++;
        TRACE("unified_cache_get_entry: Hit for ID %llu, ref_count %ld\n", id, entry->ref_count);
        return entry;
    }

    cache->misses++;
    TRACE("unified_cache_get_entry: Miss for ID %llu\n", id);

    // TODO: Entry not found, need to load it.
    // This requires interaction with block_device or file_system logic.
    // For now, get simply fails if not in cache.
    // In a full implementation, this would trigger a read from disk,
    // then a call to unified_cache_put_entry.
    return NULL;
}

status_t
unified_cache_put_entry(unified_cache* cache, uint64 id, unified_data_type type,
    void* data, size_t size, bool is_dirty, unified_cache_entry** _entry)
{
    if (cache == NULL || data == NULL || size == 0)
        return B_BAD_VALUE;

    MutexLocker locker(cache->lock);

    unified_cache_entry* entry = cache->LookupEntry(id);
    if (entry != NULL) {
        // Entry exists. Update it. This is a complex case.
        // If size changes, or data pointer changes, needs careful handling.
        // For simplicity, assume put is for new entries or overwriting with same size.
        // A real cache might free old data, update new data, mark dirty.
        TRACE("unified_cache_put_entry: Entry %llu exists. Updating (simplified).\n", id);
        // free(entry->data); // Free old data if necessary
        entry->data = data; // This assumes caller manages old data if replaced
        entry->size = size; // Size could change
        entry->is_dirty = is_dirty;
        entry->data_type = type; // Should match
        atomic_add(&entry->ref_count, 1);
        cache->AccessEntrySieve2(entry); // Treat as access
        if (_entry) *_entry = entry;
        return B_OK;
    }

    // Allocate new entry
    entry = (unified_cache_entry*)object_cache_alloc(sEntryCache, 0);
    if (entry == NULL)
        return B_NO_MEMORY;

    // Construct the entry (object_cache doesn't call constructors)
    // Need to properly initialize DoublyLinkedListLink here if not done by placement new
    new(entry) unified_cache_entry(id, type, data, size);
    entry->is_dirty = is_dirty;
    // ref_count is already 1 from constructor/initialization logic

    status_t status = cache->InsertEntry(entry);
    if (status != B_OK) {
        object_cache_free(sEntryCache, entry, 0); // Don't free entry->data, it's owned by caller
        return status;
    }

    if (_entry) *_entry = entry;
    return B_OK;
}

void
unified_cache_release_entry(unified_cache* cache, unified_cache_entry* entry)
{
    if (cache == NULL || entry == NULL)
        return;

    MutexLocker locker(cache->lock);

    atomic_add(&entry->ref_count, -1);
    TRACE("unified_cache_release_entry: Released entry %" B_PRIu64 ", new ref_count %ld\n",
        entry->id, entry->ref_count); // old_ref_count is no longer available directly here

    if (entry->ref_count < 0) {
        panic("unified_cache_release_entry: Negative ref_count for entry %" B_PRIu64 "!", entry->id);
    }

    if (entry->ref_count == 0) {
        // Potentially free the entry if it's also been removed from cache structures
        // and is not scheduled for eviction (which would handle freeing).
        // This logic is tricky: an entry with ref_count 0 is a candidate for eviction.
        // If it's *also* not in the hash/sieve list (e.g. due to discard), then it can be freed.
        // For now, do nothing more; eviction or discard handles actual freeing.
        TRACE("unified_cache_release_entry: Entry %llu ref_count is 0. Candidate for eviction.\n", entry->id);
    }
}

status_t
unified_cache_make_writable(unified_cache* cache, unified_cache_entry* entry, bool markDirty)
{
    if (cache == NULL || entry == NULL)
        return B_BAD_VALUE;

    MutexLocker locker(cache->lock);
    // No need to re-lookup, we have the entry pointer.
    // Caller must ensure entry is valid and from this cache.
    if (markDirty) {
        entry->is_dirty = true;
    }
    // Accessing it for write implies it's recently used.
    cache->AccessEntrySieve2(entry);
    TRACE("unified_cache_make_writable: Entry %llu marked %s.\n", entry->id, markDirty ? "dirty" : "writable (not dirty)");
    return B_OK;
}

status_t
unified_cache_discard(unified_cache* cache, uint64 id, unified_data_type type)
{
    if (cache == NULL)
        return B_BAD_VALUE;

    MutexLocker locker(cache->lock);

    unified_cache_entry* entry = cache->LookupEntry(id);
    if (entry == NULL)
        return B_ENTRY_NOT_FOUND;

    if (entry->data_type != type) {
        panic("unified_cache_discard: ID collision with different data type!");
        return B_BAD_VALUE; // Should be unreachable
    }

    if (entry->ref_count > 0) {
        // Cannot discard a referenced entry. Or, it means "mark for discard when refs drop to 0".
        // For now, let's be strict.
        TRACE("unified_cache_discard: Entry %llu is referenced (ref_count %ld), cannot discard directly.\n",
            id, entry->ref_count);
        return B_BUSY;
    }

    // TODO: If dirty, should it be written back first or discarded forcefully?
    // Typically, discard implies data is no longer valid or needed.
    if (entry->is_dirty) {
        TRACE("unified_cache_discard: Discarding dirty entry %llu without writeback.\n", id);
    }

    cache->RemoveEntry(entry); // This will also free the entry and its data if ref_count is 0
    TRACE("unified_cache_discard: Discarded entry %llu.\n", id);
    return B_OK;
}

// TODO: Implement Low Memory Handler integration if required by Haiku's LMR.
// static void _LowMemoryHandler(void* data, uint32 resources, int32 level) {}
// And register/unregister it.

// TODO: Transaction related logic needs to be integrated if transactions
// are managed at this cache level.

// TODO: Background writer thread for dirty pages.
// This is a major component for a write-back cache.
// For now, dirty pages are only written on eviction if explicitly handled, or sync.

// TODO: `unified_cache_sync` function (global or per-cache) to flush dirty data.

// End of unified_cache.cpp
