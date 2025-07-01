/*
 * Copyright 2023, Your Name, your.email@example.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_UNIFIED_CACHE_H
#define _KERNEL_UNIFIED_CACHE_H

#include <kernel.h>
#include <vm/vm.h> // For vm_page, if directly used, or types like phys_addr_t
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h> // Or a similar hash table implementation

// Forward declarations
struct unified_cache_entry;

enum unified_data_type {
    BLOCK_DATA,
    FILE_PAGE_DATA
};

// Represents a single entry in the cache
struct unified_cache_entry {
    uint64                  id;          // Unique identifier (e.g., block_num or vnode_id:page_offset)
    unified_data_type       data_type;   // Type of data stored
    void*                   data;        // Pointer to the cached data
    size_t                  size;        // Size of the data in bytes
    bool                    is_dirty;    // True if modified and needs write-back
    int32                   ref_count;   // Reference count for this entry

    // SIEVE-2 related fields
    // 'sieve_hand' is implicitly managed by the list/clock position.
    // For SIEVE-2, we need a way to distinguish between newly inserted,
    // once-visited, and twice-visited (or more) pages. A simple counter is effective.
    // 0: Can be evicted
    // 1: Visited once (recently accessed)
    // 2: Visited twice or more (hot page)
    uint8                   sieve_visited_count;

    // Linking for hash table
    unified_cache_entry*    next_in_hash;

    // Linking for SIEVE-2 eviction list (doubly linked list)
    DoublyLinkedListLink<unified_cache_entry> sieve_link;

    // TODO: Add fields for transaction management if needed at this level

    // Constructor
    unified_cache_entry(uint64 _id, unified_data_type _type, void* _data, size_t _size)
        : id(_id), data_type(_type), data(_data), size(_size), is_dirty(false),
          ref_count(1), sieve_visited_count(0), next_in_hash(NULL) {
    }

    // For OpenHashTable
    unified_cache_entry*& HashLink() { return next_in_hash; }
    const uint64& HashKey() const { return id; }
};

// The main unified cache structure
struct unified_cache {
    struct EntryHashTableDefinition {
        typedef uint64                  KeyType;
        typedef unified_cache_entry     ValueType;

        size_t HashKey(KeyType key) const {
            // A simple hash function, can be improved
            return key ^ (key >> 16);
        }
        size_t Hash(ValueType* value) const {
            return HashKey(value->id);
        }
        bool Compare(KeyType key, ValueType* value) const {
            return value->id == key;
        }
        ValueType*& GetLink(ValueType* value) const {
            return value->next_in_hash;
        }
    };

    typedef BOpenHashTable<EntryHashTableDefinition> EntryHashTable;
    EntryHashTable*         hash_table;

    // SIEVE-2 uses a clock-like structure. A doubly linked list can serve this.
    // New entries are added to one end (e.g., tail), and the "hand" scans from the other (e.g., head).
    typedef DoublyLinkedList<unified_cache_entry,
        DoublyLinkedListMemberGetLink<unified_cache_entry,
            &unified_cache_entry::sieve_link> > SieveList;
    SieveList               sieve_list;
    unified_cache_entry*    sieve_hand; // Pointer to the current "hand" in the sieve_list

    mutex                   lock;
    const char*             name;

    size_t                  max_size_bytes;
    size_t                  current_size_bytes;
    size_t                  max_entries; // Optional: limit entry count too
    size_t                  current_entries;

    // Statistics
    uint64                  hits;
    uint64                  misses;
    uint64                  evictions;

    status_t                Init(const char* cache_name, size_t max_size);
    void                    Destroy();

    unified_cache_entry*    LookupEntry(uint64 id);
    status_t                InsertEntry(unified_cache_entry* entry);
    void                    RemoveEntry(unified_cache_entry* entry);
    unified_cache_entry*    EvictEntrySieve2(); // Core SIEVE-2 eviction
    void                    AccessEntrySieve2(unified_cache_entry* entry); // Update on hit

    // TODO: Low memory handling
};

// Public API functions (prototypes)
status_t unified_cache_init(const char* name, size_t max_size_bytes, unified_cache** _cache);
void unified_cache_destroy(unified_cache* cache);

unified_cache_entry* unified_cache_get_entry(unified_cache* cache, uint64 id, unified_data_type type);
    // Returns a referenced entry, or NULL if not found (after trying to load)

status_t unified_cache_put_entry(unified_cache* cache, uint64 id, unified_data_type type,
    void* data, size_t size, bool is_dirty, unified_cache_entry** _entry = NULL);
    // Puts data into the cache, returns a referenced entry

void unified_cache_release_entry(unified_cache* cache, unified_cache_entry* entry);

status_t unified_cache_make_writable(unified_cache* cache, unified_cache_entry* entry, bool markDirty = true);
status_t unified_cache_discard(unified_cache* cache, uint64 id, unified_data_type type);

// Helper to combine vnode_id and page_offset into a single uint64 id
static inline uint64
file_page_id(ino_t vnode_id, off_t page_offset)
{
    // Assuming page_offset is page-aligned, so lower bits of page_offset are 0.
    // Max vnode_id needs to be considered. A common way is to shift one.
    // Let's assume page_offset is actually page index.
    return ((uint64)vnode_id << 32) | (uint64)(page_offset / B_PAGE_SIZE);
}

#endif // _KERNEL_UNIFIED_CACHE_H
