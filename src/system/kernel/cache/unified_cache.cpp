/*
 * Copyright 2023, Your Name, your.email@example.com.
 * Distributed under the terms of the MIT License.
 */

#include "unified_cache.h"

#include <slab/Slab.h>
#include <util/AutoLock.h>
#include <kernel.h> // For dprintf, new_lock etc.
#include <inttypes.h> // For B_PRIu64 if not implicitly included

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
        // This simplified Destroy assumes entries are no longer referenced and can be freed.
        // Data is assumed to be malloc'd.
        if (entry->data != NULL)
            free(entry->data);
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
    while (current_size_bytes + entry->size > max_size_bytes && !sieve_list.IsEmpty()) {
        status_t evictStatus = EvictEntrySieve2();
        if (evictStatus == B_OK) {
            TRACE("InsertEntry: Evicted an entry to make space.\n");
            // Loop again to check size or if list became empty
        } else if (evictStatus == B_ENTRY_NOT_FOUND || evictStatus == B_BUSY) {
            // No suitable victim found (all pinned, or all visited and got a second chance)
            // or all remaining are dirty and writeback isn't implemented/immediate.
            TRACE("InsertEntry: Eviction failed (no suitable victim or all dirty/pinned), current size %lu, entry size %lu, max %lu\n",
                current_size_bytes, entry->size, max_size_bytes);
            if (current_size_bytes + entry->size > max_size_bytes)
                return B_NO_MEMORY; // Still no space
            break; // Exit loop if no victim found, proceed to insert if space allows
        } else {
            // Some other error during eviction
             TRACE("InsertEntry: Eviction failed with error %s.\n", strerror(evictStatus));
            return evictStatus;
        }
    }
    if (current_size_bytes + entry->size > max_size_bytes) {
        // Final check if list became empty or eviction wasn't enough/possible
        return B_NO_MEMORY;
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

    TRACE("InsertEntry: Inserted entry %\" B_PRIu64 \", current size %lu bytes, %lu entries.\n",
        entry->id, current_size_bytes, current_entries);
    return B_OK;
}

void
unified_cache::RemoveEntry(unified_cache_entry* entry)
{
    // Assumes lock is held and entry is no longer referenced externally for cache purposes
    // (ref_count might be >0 if someone holds a direct pointer, but cache considers it removable)
    TRACE("RemoveEntry: Removing entry %\" B_PRIu64 \".\n", entry->id);

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
        // If ref_count is 0, this entry is not externally referenced and can be freed.
        // Data is assumed to be malloc'd.
        if (entry->data != NULL)
            free(entry->data);
        object_cache_free(sEntryCache, entry, 0);
    }
}

status_t
unified_cache::EvictEntrySieve2()
{
    // Assumes lock is held
    if (sieve_list.IsEmpty())
        return B_ENTRY_NOT_FOUND; // Or B_OK if "nothing to evict" is not an error

    unified_cache_entry* candidate = sieve_hand;
    unified_cache_entry* originalHand = sieve_hand;
    bool loopedOnce = false;

    while (candidate != NULL) {
        // If the entry is referenced, we cannot evict it. Skip.
        // In SIEVE, objects with ref_count > 0 are considered "pinned".
        if (candidate->ref_count > 0) {
            sieve_hand = sieve_list.GetNext(candidate);
            if (sieve_hand == NULL) sieve_hand = sieve_list.Head(); // Wrap around
            if (sieve_hand == originalHand && loopedOnce && candidate->ref_count > 0) {
                // Made a full loop and the original hand (which is current candidate) is still pinned
                TRACE("EvictEntrySieve2: Full loop, all remaining pages are pinned or only one pinned page.\n");
                return B_ENTRY_NOT_FOUND; // Or B_BUSY if more appropriate
            }
            candidate = sieve_hand;
            if (candidate == originalHand)
                loopedOnce = true;
            continue;
        }

        if (candidate->sieve_visited_count > 0) {
            candidate->sieve_visited_count--; // "Second chance"
            sieve_hand = sieve_list.GetNext(candidate);
            if (sieve_hand == NULL) sieve_hand = sieve_list.Head(); // Wrap around
        } else {
            // Found victim
            TRACE("EvictEntrySieve2: Victim found %\" B_PRIu64 \" (visited_count: %u, ref_count: %ld).\n",
                candidate->id, candidate->sieve_visited_count, candidate->ref_count);

            // TODO: Handle dirty victim: write back before evicting.
            if (candidate->is_dirty) {
                TRACE("EvictEntrySieve2: Victim %\" B_PRIu64 \" is dirty. Skipping for now (needs writeback).\n", candidate->id);
                sieve_hand = sieve_list.GetNext(candidate);
                if (sieve_hand == NULL) sieve_hand = sieve_list.Head();
                if (sieve_hand == originalHand && loopedOnce) {
                     TRACE("EvictEntrySieve2: Full loop, all remaining unpinned pages are dirty.\n");
                     return B_IO_ERROR; // Indicate dirty pages blocking eviction
                }
                candidate = sieve_hand;
                if (candidate == originalHand)
                    loopedOnce = true;
                continue; // Try next
            }

            // Advance the hand *before* removing the candidate
            unified_cache_entry* nextHand = sieve_list.GetNext(candidate);
            if (nextHand == NULL) {
                nextHand = sieve_list.Head();
            }
             // If candidate is the only item, nextHand might become candidate itself.
            // After removing candidate, if list becomes empty, hand should be NULL.
            // If list not empty but nextHand was candidate (only item), hand should point to new head.

            hash_table->Remove(candidate);
            sieve_list.Remove(candidate); // This will adjust head/tail of sieve_list

            if (sieve_list.IsEmpty()) {
                sieve_hand = NULL;
            } else if (sieve_hand == candidate) {
                // If the hand was pointing to the victim, and it was the only item,
                // sieve_list.GetNext(candidate) would be NULL, then sieve_list.Head()
                // would be NULL after removal.
                // If it was not the only item, nextHand is valid.
                sieve_hand = nextHand;
                 // If nextHand was candidate (single item list), and now it's removed,
                 // sieve_list.Head() is the correct new hand (or NULL if empty).
                if (sieve_hand == candidate) sieve_hand = sieve_list.Head();

            }
            // If sieve_hand was not the candidate, it remains valid unless candidate was its prev/next.
            // The DoublyLinkedList handles this internally. We just need to ensure sieve_hand is valid.
            // The most robust is to set it relative to the list state after removal.
            // This simplified hand update should be:
            // if (sieve_hand == candidate) {
            //     sieve_hand = nextHandIfAnyOrListHead;
            // } // else hand is fine
            // The current sieve_hand = nextHand (calculated before removal) is mostly okay,
            // but if candidate was the tail, nextHand became Head. If Head was candidate, it's tricky.
            // Let's reset based on list state:
            if (!sieve_list.IsEmpty() && (sieve_hand == candidate || !sieve_list.Contains(sieve_hand))) {
                 sieve_hand = sieve_list.Head(); // Default to head if hand is invalid or was victim
            } else if (sieve_list.IsEmpty()) {
                 sieve_hand = NULL;
            }


            current_size_bytes -= candidate->size;
            current_entries--;
            evictions++;

            if (candidate->data != NULL)
                free(candidate->data);
            object_cache_free(sEntryCache, candidate, 0);

            return B_OK;
        }
        candidate = sieve_hand;
        if (candidate == originalHand) {
            if (loopedOnce) {
                // Made a full circle, and all pages were either pinned or got a second chance (their count > 0 initially)
                TRACE("EvictEntrySieve2: Full scan, no immediately evictable entry found this pass.\n");
                return B_ENTRY_NOT_FOUND; // Or B_WOULD_BLOCK if we expect counts to drop
            }
            loopedOnce = true;
        }
        if (candidate == NULL && !sieve_list.IsEmpty()) {
             sieve_hand = sieve_list.Head();
             candidate = sieve_hand;
             if(candidate == originalHand) loopedOnce = true; // Avoid infinite loop on single item list
        }
    }
    TRACE("EvictEntrySieve2: No evictable entry found (e.g. list became empty during scan, or logic error).\n");
    return B_ENTRY_NOT_FOUND;
}

void
unified_cache::AccessEntrySieve2(unified_cache_entry* entry)
{
    // Assumes lock is held
    // For SIEVE-2, on access, set visited count to the "hot" value (e.g., 2).
    // This indicates it has been recently accessed.
    // New entries start at 0 (evictable).
    // During eviction scan, count is decremented if > 0. Evicted when 0.
    entry->sieve_visited_count = 2;

    TRACE("AccessEntrySieve2: Accessed entry %\" B_PRIu64 \", new visited_count %u.\n", entry->id, entry->sieve_visited_count);
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
        TRACE("unified_cache_get_entry: Hit for ID %\" B_PRIu64 \", ref_count %ld\n", id, entry->ref_count);
        return entry;
    }

    cache->misses++;
    TRACE("unified_cache_get_entry: Miss for ID %\" B_PRIu64 \".\n", id);

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
        TRACE("unified_cache_put_entry: Entry %\" B_PRIu64 \" exists. Updating.\n", id);
        if (entry->data != data) { // Only free if new data pointer is different
            if (entry->data != NULL)
                free(entry->data); // Free old data; assumes cache owned it.
            entry->data = data;
        }
        // Update size and dirty status
        cache->current_size_bytes -= entry->size;
        cache->current_size_bytes += size;
        entry->size = size;
        entry->is_dirty = is_dirty;
        // entry->data_type should match, assert or check if necessary
        if (entry->data_type != type) {
            panic("unified_cache_put_entry: ID collision with different data type on update!");
            // Or return B_BAD_TYPE;
        }
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

    atomic_add(&entry->ref_count, -1); // Removed storing old_ref_count
    TRACE("unified_cache_release_entry: Released entry %\" B_PRIu64 \", new ref_count %ld\n",
        entry->id, entry->ref_count);

    if (entry->ref_count < 0) {
        panic("unified_cache_release_entry: Negative ref_count for entry %" B_PRIu64 "!", entry->id);
    }

    if (entry->ref_count == 0) {
        // Potentially free the entry if it's also been removed from cache structures
        // and is not scheduled for eviction (which would handle freeing).
        // This logic is tricky: an entry with ref_count 0 is a candidate for eviction.
        // If it's *also* not in the hash/sieve list (e.g. due to discard), then it can be freed.
        // For now, do nothing more; eviction or discard handles actual freeing.
        TRACE("unified_cache_release_entry: Entry %\" B_PRIu64 \" ref_count is 0. Candidate for eviction.\n", entry->id);
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
    TRACE("unified_cache_make_writable: Entry %\" B_PRIu64 \" marked %s.\n", entry->id, markDirty ? "dirty" : "writable (not dirty)");
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
        TRACE("unified_cache_discard: Entry %\" B_PRIu64 \" is referenced (ref_count %ld), cannot discard directly.\n",
            id, entry->ref_count);
        return B_BUSY;
    }

    // TODO: If dirty, should it be written back first or discarded forcefully?
    // Typically, discard implies data is no longer valid or needed.
    if (entry->is_dirty) {
        TRACE("unified_cache_discard: Discarding dirty entry %\" B_PRIu64 \" without writeback.\n", id);
    }

    cache->RemoveEntry(entry); // This will also free the entry and its data if ref_count is 0
    TRACE("unified_cache_discard: Discarded entry %\" B_PRIu64 \".\n", id);
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
