// jcache.c

#ifdef OPT_JAMFILE_CACHE_EXT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jam.h"
#include "jcache.h"
#include "filesys.h"
#include "hash.h"
#include "lists.h"
#include "newstr.h"
#include "pathsys.h"
#include "parse.h"
#include "rules.h"
#include "search.h"
#include "variable.h"

///////////////
// string_list
//

typedef struct string_list {
	char**	strings;	// null terminated array of strings
	int		count;		// number of strings in the array, not counting the
						// terminating null
	int		capacity;	// current capacity of the string array
	int		block_size;	// granularity (number of entries) to be used for
						// resizing the array
} string_list;

// string list prototypes
static string_list* new_string_list(int block_size);
static void delete_string_list(string_list* list);
static int resize_string_list(string_list *list, int size);
static int push_string(string_list *list, char *string);
static char* pop_string(string_list *list);


// file_read_line
/*!	\brief Reads a line from the supplied file and writes it to the supplied
		   buffer.

	If the line end in a LF, it is chopped off.

	\param file The file.
	\param value The pointer to where the read value shall be written.
	\return \c ~0, if a number could be read, 0 otherwise.
*/
static
int
file_read_line(FILE *file, char *buffer, int bufferSize)
{
	int len;

	if (!fgets(buffer, bufferSize, file))
		return 0;

	len = strlen(buffer);
	if (len > 0 && buffer[len - 1] == '\n')
		buffer[len - 1] = '\0';

	return 1;
}

// file_read_line_long
/*!	\brief Reads a line from the supplied file and interprets it as long value.

	This is almost equivalent to \code fscanf(file, "%ld", value) \endcode,
	save that fscanf() seems to eat all following LFs, while this function
	only reads one.

	\param file The file.
	\param value The pointer to where the read value shall be written.
	\return \c ~0, if a number could be read, 0 otherwise.
*/
static
int
file_read_line_long(FILE *file, long *value)
{
	char buffer[32];
	int result;

	result = file_read_line(file, buffer, sizeof(buffer));
	if (!result)
		return result;

	if (sscanf(buffer, "%ld\n", value) != 1)
		return 0;

	return 1;
}

// new_string_list
/*!	\brief Creates a new string_list.
	\param block_size Granularity (number of entries) to be used for
		   resizing the string array.
	\return Pointer to the newly allocated string_list, or 0 when out of
			memory.
*/
static
string_list*
new_string_list(int block_size)
{
	string_list *list = (string_list*)malloc(sizeof(string_list));
	if (list) {
		list->strings = 0;
		list->count = 0;
		list->capacity = 0;
		if (block_size <= 0)
			block_size = 5;
		list->block_size = block_size;
		if (!resize_string_list(list, 0)) {
			free(list);
			list = 0;
		}
	}
	return list;
}

// delete_string_list
/*!	\brief Deletes a string_list formerly allocated with new_string_list.

	All strings the list contains are freed as well.

	\param list The string_list to be deleted.
*/
static
void
delete_string_list(string_list* list)
{
	if (list) {
		if (list->strings) {
			int i = 0;
			for (i = 0; i < list->count; i++) {
				if (list->strings[i])
					free(list->strings[i]);
			}
			free(list->strings);
		}
		free(list);
	}
}

// resize_string_list
/*!	\brief Resizes the string array of a string_list.

	\note This functions is for internal use only.

	\param list The string_list to be resized.
	\param size Number of entries the list shall be able to contain.
	\return \c !0, if everything went fine, 0, if an error occured (out of
			memory).
*/
static
int
resize_string_list(string_list *list, int size)
{
	int result = 0;
	if (list) {
		// capacity must be at least size + 1 and a multiple of the block size
		int newCapacity = (size + list->block_size)
			/ list->block_size * list->block_size;
		if (newCapacity == list->capacity)
			result = !0;
		else {
			char** newStrings = (char**)realloc(list->strings,
												newCapacity * sizeof(char*));
			if (newStrings) {
				result = !0;
				list->strings = newStrings;
				list->capacity = newCapacity;
			}
		}
	}
	return result;
}

// push_string
/*!	\brief Appends a string to a string_list.

	The list's string array is resized, if necessary and null terminated.

	\param list The string_list.
	\param string The string to be appended.
	\return \c !0, if everything went fine, 0, if an error occured (out of
			memory).
*/
static
int
push_string(string_list *list, char *string)
{
	int result = 0;
	if (list) {
		result = resize_string_list(list, list->count + 1);
		if (result) {
			list->strings[list->count] = string;
			list->count++;
			list->strings[list->count] = 0;	// null terminate
		}
	}
	return result;
}

// pop_string
/*!	\brief Removes the last string from a string_list.

	The list's string array is resized, if necessary and null terminated.
	The caller takes over ownership of the removed string and is responsible
	for freeing it.

	\param list The string_list.
	\return The removed string, if everything went fine, 0 otherwise.
*/
static
char*
pop_string(string_list *list)
{
	char* string = 0;
	if (list && list->count > 0) {
		list->count--;
		string = list->strings[list->count];
		list->strings[list->count] = 0;
		resize_string_list(list, list->count);
	}
	return string;
}


///////////////////
// jamfile caching
//

// the jamfile cache
typedef struct jamfile_cache {
	struct hash*	entries;	// hash table of jcache_entrys
	string_list*	filenames;	// entry filenames
	char*			cache_file;	// name of the cache file
} jamfile_cache;

// a cache entry for an include file
typedef struct jcache_entry {
	char*			filename;	// name of the file
	time_t			time;		// time stamp of the file
	string_list*	strings;	// contents of the file
	int				used;		// whether this cache entry has been used
} jcache_entry;

// pointer to the jamfile cache
static jamfile_cache* jamfileCache = 0;

// jamfile cache prototypes
static jamfile_cache* new_jamfile_cache(void);
static void delete_jamfile_cache(jamfile_cache* cache);
static int init_jcache_entry(jcache_entry* entry, char *filename, time_t time,
							 int used);
static void cleanup_jcache_entry(jcache_entry* entry);
static int add_jcache_entry(jamfile_cache* cache, jcache_entry* entry);
static jcache_entry* find_jcache_entry(jamfile_cache* cache, char* filename);
static string_list* read_file(const char *filename, string_list* list);
static int read_jcache(jamfile_cache* cache, char* filename);
static int write_jcache(jamfile_cache* cache);
static char* jcache_name(void);
static jamfile_cache* get_jcache(void);

// new_jamfile_cache
/*!	\brief Creates a new jamfile_cache.
	\return A pointer to the newly allocated jamfile_cache, or 0, if out of
			memory.
*/
static
jamfile_cache*
new_jamfile_cache(void)
{
	jamfile_cache *cache = (jamfile_cache*)malloc(sizeof(jamfile_cache));
	if (cache) {
		cache->entries = hashinit(sizeof(jcache_entry), "jcache");
		cache->filenames = new_string_list(100);
		cache->cache_file = 0;
		if (!cache->entries || !cache->filenames) {
			delete_jamfile_cache(cache);
			cache = 0;
		}
	}
	return cache;
}

// delete_jamfile_cache
/*!	\brief Deletes a jamfile_cache formerly allocated with new_jamfile_cache.
	\param cache The jamfile_cache to be deleted.
*/
static
void
delete_jamfile_cache(jamfile_cache* cache)
{
	if (cache) {
		if (cache->entries)
			hashdone(cache->entries);
		delete_string_list(cache->filenames);
		free(cache->cache_file);
	}
}

// init_jcache_entry
/*!	\brief Initializes a pre-allocated jcache_entry.
	\param entry The jcache_entry to be initialized.
	\param filename The name of the include file to be associated with the
		   entry.
	\param time The time stamp of the include file to be associated with the
		   entry.
	\param used Whether or not the entry shall be marked used.
	\return \c !0, if everything went fine, 0, if an error occured (out of
			memory).
*/
static
int
init_jcache_entry(jcache_entry* entry, char *filename, time_t time, int used)
{
	int result = 0;
	if (entry) {
		result = !0;
		entry->filename = (char*)malloc(strlen(filename) + 1);
		if (entry->filename)
			strcpy(entry->filename, filename);
		entry->time = time;
		entry->strings = new_string_list(100);
		entry->used = used;
		// cleanup on error
		if (!entry->filename || !entry->strings) {
			cleanup_jcache_entry(entry);
			result = 0;
		}
	}
	return result;
}

// cleanup_jcache_entry
/*!	\brief De-initializes a jcache_entry.

	All resources associated with the entry, save the memory for the entry
	structure itself, are freed.

	\param entry The jcache_entry to be de-initialized.
*/
static
void
cleanup_jcache_entry(jcache_entry* entry)
{
	if (entry) {
		if (entry->filename)
			free(entry->filename);
		if (entry->strings)
			delete_string_list(entry->strings);
	}
}

// add_jcache_entry
/*!	\brief Adds a jcache_entry to a jamfile_cache.
	\param cache The jamfile_cache.
	\param entry The jcache_entry to be added.
	\return \c !0, if everything went fine, 0, if an error occured (out of
			memory).
*/
static
int
add_jcache_entry(jamfile_cache* cache, jcache_entry* entry)
{
	int result = 0;
	if (cache && entry) {
		result = push_string(cache->filenames, entry->filename);
		if (result) {
			result = hashenter(cache->entries, (HASHDATA**)&entry);
			if (!result)
				pop_string(cache->filenames);
		}
	}
	return result;
}

// find_jcache_entry
/*!	\brief Looks up jcache_entry in a jamfile_cache.
	\param cache The jamfile_cache.
	\param filename The name of the include file for whose jcache_entry shall
	 	   be retrieved.
	\return A pointer to the found jcache_entry, or 0, if the cache does not
			contain an entry for the specified filename.
*/
static
jcache_entry*
find_jcache_entry(jamfile_cache* cache, char* filename)
{
	jcache_entry _entry;
	jcache_entry* entry = &_entry;
	entry->filename = filename;
	if (!hashcheck(cache->entries, (HASHDATA**)&entry))
		entry = 0;
	return entry;
}

// read_file
/*!	\brief Reads in a text file and returns its contents as a string_list.

	The function strips leading white spaces from each line and omits empty
	or comment lines.

	If a string_list is supplied via \a list the file's content is appended
	to this list, otherwise a new string_list is allocated.

	\param filename The name of the file to be read in.
	\param list Pointer to a pre-allocated string_list. May be 0.
	\return A pointer to the string_list containing the contents of the file,
			or 0, if an error occured.
*/
static
string_list*
read_file(const char *filename, string_list* list)
{
	int result = 0;
	FILE *file = 0;
	string_list *allocatedList = 0;
	// open file
	if ((file = fopen(filename, "r")) != 0
		&& (list || (list = allocatedList = new_string_list(100)) != 0)) {
		// read the file
		char buffer[513];
		result = !0;
		while (result && fgets(buffer, sizeof(buffer) - 1, file)) {
			char* line = buffer;
			int len = 0;
			char *string = 0;
			// skip leading white spaces
			while (*line == ' ' || *line == '\t' || *line == '\n')
				line++;
			// make empty and comment lines simple new-line lines
			if (!*line || *line == '#') {
				line[0] = '\n';
				line[1] = '\0';
			}
			len = strlen(line);
			// make sure, the line ends in a LF
			if (line[len - 1] != '\n') {
				line[len] = '\n';
				len++;
				line[len] = '\0';
			}
			// copy it
			string = (char*)malloc(len + 1);
			if (string) {
				strcpy(string, line);
				result = push_string(list, string);
			} else
				result = 0;
		}
		// close the file
		fclose(file);
	} else
		perror(filename);
	// cleanup on error
	if (!result) {
		delete_string_list(allocatedList);
		list = 0;
	}
	return list;
}

// read_jcache
/*!	\brief Reads a jamfile_cache from a file.

	Only cache entries for files, that don't have an entry in \a cache yet, are
	added to it.

	\param cache The jamfile_cache the cache stored in the file shall be added
		   to.
	\param filename The name of the file containing the cache to be read.
	\return \c !0, if everything went fine, 0, if an error occured.
*/
static
int
read_jcache(jamfile_cache* cache, char* filename)
{
	int result = 0;
	if (cache && filename) {
		// open file
		FILE *file = 0;
		cache->cache_file = filename;
		if ((file = fopen(filename, "r")) != 0) {
			// read the file
			char buffer[512];
			long count = 0;
			int i;
			result = !0;
			// read number of cache entries
			result = file_read_line_long(file, &count);
			// read the cache entries
			for (i = 0; result && i < count; i++) {
				char entryname[PATH_MAX];
				long lineCount = 0;
				time_t time = 0;
				jcache_entry entry = { 0, 0, 0 };
				// entry name, time and line count
				if (file_read_line(file, entryname, sizeof(entryname))
					&& strlen(entryname) > 0
					&& file_read_line_long(file, &time)
					&& file_read_line_long(file, &lineCount)
					&& (init_jcache_entry(&entry, entryname, time, 0)) != 0) {
					// read the lines
					int j;
					for (j = 0; result && j < lineCount; j++) {
						if (fgets(buffer, sizeof(buffer), file)) {
							char *string = (char*)malloc(strlen(buffer) + 1);
							if (string) {
								strcpy(string, buffer);
								result = push_string(entry.strings, string);
							} else
								result = 0;
						} else {
							fprintf(stderr, "warning: Invalid jamfile cache: "
								"Unexpected end of file.\n");
							result = 0;
						}
					}
				} else {
					fprintf(stderr, "warning: Invalid jamfile cache: "
						"Failed to read file info.\n");
					result = 0;
				}
				if (result) {
					// add only, if there's no entry for that file yet
					if (find_jcache_entry(cache, entry.filename))
						cleanup_jcache_entry(&entry);
					else
						result = add_jcache_entry(cache, &entry);
				}
				// cleanup on error
				if (!result)
					cleanup_jcache_entry(&entry);
			}
			// close the file
			fclose(file);
		} // else: Couldn't open cache file. Don't worry.
	}
	return result;
}

// write_jcache
/*!	\brief Writes a jamfile_cache into a file.
	\param cache The jamfile_cache that shall be stored in the file.
	\param filename The name of the file the cache shall be stored in.
	\return \c !0, if everything went fine, 0, if an error occured.
*/
static
int
write_jcache(jamfile_cache* cache)
{
	int result = 0;
	if (cache && cache->cache_file) {
		// open file
		FILE *file = 0;
		if ((file = fopen(cache->cache_file, "w")) != 0) {
			int count = cache->filenames->count;
			int i;
			// write number of cache entries
			result = (fprintf(file, "%d\n", count) > 0);
			// write the cache entries
			for (i = 0; result && i < count; i++) {
				char* entryname = cache->filenames->strings[i];
				jcache_entry* entry = find_jcache_entry(cache, entryname);
				// entry name, time and line count
				if (!entry) {
					result = 0;
				} else if (!entry->strings || !entry->used) {
					// just skip the entry, if it is not loaded or not used
				} else if (fprintf(file, "%s\n", entryname) > 0
					&& (fprintf(file, "%ld\n", entry->time) > 0)
					&& (fprintf(file, "%d\n", entry->strings->count) > 0)) {
					int j;
					// the lines
					for (j = 0; result && j < entry->strings->count; j++) {
						result = (fprintf(file,
										  entry->strings->strings[j]) > 0);
					}
				} else
					result = 0;
			}
			// close the file
			fclose(file);
		}
	}
	return result;
}

// jcache_name
/*!	\brief Returns the name of the file containing the global jamfile_cache.

	The returned filename is the path to the target stored in the jam variable
	\c JCACHEFILE. The string does not need to be freed.

	\return A pointer to the jamfile cache file, or 0, if the jam variable is
			not set yet, or an error occured.
*/
static
char*
jcache_name(void)
{
	static char* name = 0;
	if (!name) {
		LIST *jcachevar = var_get("JCACHEFILE");

		if (jcachevar) {
			TARGET *t = bindtarget( jcachevar->string );

			pushsettings( t->settings );
			t->boundname = search( t->name, &t->time );
			popsettings( t->settings );

			if (t->boundname) {
				name = (char*)copystr(t->boundname);
			}
		}
	}
	return name;
}

// get_jcache
/*!	\brief Returns a pointer to the global jamfile_cache.

	The cache is being lazy-allocated.

	\return A pointer to the global jamfile_cache, or 0, if an error occured.
*/
static
jamfile_cache*
get_jcache(void)
{
	if (!jamfileCache)
		jamfileCache = new_jamfile_cache();
	if (jamfileCache && !jamfileCache->cache_file) {
		char* filename = jcache_name();
		if (filename)
			read_jcache(jamfileCache, filename);
	}
	return jamfileCache;
}

// jcache_init
/*!	\brief To be called before using the global jamfile_cache.

	Does nothing currently. The global jamfile_cache is lazy-allocated by
	get_jcache().
*/
void
jcache_init(void)
{
}

// jcache_done
/*!	\brief To be called when done with the global jamfile_cache.

	Writes the cache to the specified cache file.
*/
void
jcache_done(void)
{
	jamfile_cache* cache = get_jcache();
	if (cache) {
		write_jcache(cache);
		delete_jamfile_cache(cache);
		jamfileCache = 0;
	}
}

// jcache
/*!	\brief Returns the contents of an include file as a null terminated string
		   array.

	If the file is cached and the respective entry is not obsolete, the cached
	string array is returned, otherwise the file is read in.

	The caller must not free the returned string array or any of the contained
	strings.

	\param filename The name of the include file.
	\return A pointer to a null terminated string array representing the
			contents of the specified file, or 0, if an error occured.
*/
char**
jcache(char *_filename)
{
	char** strings = 0;
	jamfile_cache* cache = get_jcache();
	time_t time;
	// normalize the filename
    char _normalizedPath[PATH_MAX];
    char *filename = normalize_path(_filename, _normalizedPath,
    	sizeof(_normalizedPath));
    if (!filename)
    	filename = _filename;
	// get file time
	if (!cache)
		return 0;
	if (file_time(filename, &time) == 0) {
		// lookup file in cache
		jcache_entry* entry = find_jcache_entry(cache, filename);
		if (entry) {
			// in cache
			entry->used = !0;
			if (entry->time == time && entry->strings) {
				// up to date
				strings = entry->strings->strings;
			} else {
				// obsolete
				delete_string_list(entry->strings);
				entry->strings = read_file(filename, 0);
				entry->time = time;
				strings = entry->strings->strings;
			}
		} else {
			// not in cache
			jcache_entry newEntry;
			entry = &newEntry;
			init_jcache_entry(entry, filename, time, !0);
			if (read_file(filename, entry->strings)) {
				if (add_jcache_entry(cache, entry))
					strings = entry->strings->strings;
			}
			// cleanup on error
			if (!strings)
				cleanup_jcache_entry(entry);
		}
	} else
		perror(filename);
	return strings;
}

#endif	// OPT_JAMFILE_CACHE_EXT
