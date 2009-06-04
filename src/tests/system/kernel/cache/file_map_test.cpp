/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>

#include <fs_cache.h>
#include <fs_interface.h>

#include <file_cache.h>


#define MAX_VECS	32


class Map {
public:
	Map(const char* name, off_t size);
	~Map();

	void SetTo(const char* name, off_t size);

	Map& Add(off_t offset, off_t length, off_t diskOffset);
	Map& Clear();
	Map& SetSize(off_t size);
	void Invalidate(off_t start, off_t size);
	void SetMode(uint32 mode);
	void Test();

	status_t GetFileMap(off_t offset, off_t length, file_io_vec* vecs,
		size_t* _vecCount);

private:
	void _Error(const char* format, ...);
	void _Verbose(const char* format, ...);
	int32 _IndexFor(off_t offset);

	const char*	fName;
	uint32		fTest;
	void*		fMap;
	off_t		fOffsets[MAX_VECS];
	file_io_vec	fVecs[MAX_VECS];
	file_io_vec	fTestVecs[MAX_VECS];
	uint32		fCount;
	uint32		fTestCount;
	off_t		fTestOffset;
	off_t		fTestLength;
	off_t		fSize;
};


static bool sVerbose;


Map::Map(const char* name, off_t size)
	:
	fName(NULL),
	fMap(NULL),
	fCount(0),
	fSize(0)
{
	SetTo(name, size);
}


Map::~Map()
{
	file_map_delete(fMap);
}


void
Map::SetTo(const char* name, off_t size)
{
	file_map_delete(fMap);

	fMap = file_map_create((dev_t)this, 0, size);
	if (fMap == NULL)
		_Error("Creating file map failed.");

	fName = name;
	fSize = size;
	fCount = 0;
	fTest = 0;

	printf("Running %s\n", fName);
}


Map&
Map::Add(off_t offset, off_t length, off_t diskOffset)
{
	_Verbose("  Add(): offset %lld, length %lld, diskOffset %lld", offset,
		length, diskOffset);

	if (fCount < MAX_VECS) {
		fOffsets[fCount] = offset;
		fVecs[fCount].offset = diskOffset;
		fVecs[fCount].length = length;
		fCount++;
	}

	return *this;
}


Map&
Map::Clear()
{
	_Verbose("  Clear()");
	fCount = 0;
	return *this;
}


Map&
Map::SetSize(off_t size)
{
	_Verbose("  SetSize(): size %lld", size);
	file_map_set_size(fMap, size);
	fSize = size;
	return *this;
}


void
Map::Invalidate(off_t start, off_t size)
{
	_Verbose("  Invalidate(): start %lld, size %lld", start, size);
	file_map_invalidate(fMap, start, size);
}


void
Map::SetMode(uint32 mode)
{
	file_map_set_mode(fMap, mode);
}


void
Map::Test()
{
	printf("  Test %lu\n", ++fTest);

	for (off_t offset = 0; offset < fSize; offset += 256) {
		fTestOffset = offset;
		fTestLength = 256;
		fTestCount = MAX_VECS;
		status_t status = file_map_translate(fMap, offset, fTestLength,
			fTestVecs, &fTestCount, 0);
		if (status != B_OK) {
			_Error("file_map_translate(offset %lld) failed: %s", offset,
				strerror(status));
		}

		int32 index = _IndexFor(offset);
		if (index < 0)
			_Error("index for offset %lld not found!", offset);
		
		off_t diff = offset - fOffsets[index];

		if (fTestVecs[0].length > fSize - diff) {
			_Error("size too large: got %lld, size is %lld",
				fTestVecs[0].length, fSize);
		}
		if (fTestVecs[0].offset != fVecs[index].offset + diff) {
			_Error("offset mismatch: got %lld, should be %lld",
				fTestVecs[0].offset, fVecs[index].offset + diff);
		}
	}

	fTestCount = 0;
}


status_t
Map::GetFileMap(off_t offset, off_t length, file_io_vec* vecs,
	size_t* _vecCount)
{
	int32 index = _IndexFor(offset);
	if (index < 0)
		_Error("No vec for offset %lld\n", offset);

	_Verbose("    GetFileMap(): offset: %lld, length: %lld, index %ld", offset,
		length, index);

	uint32 count = 0;

	while (length > 0) {
		if (count >= *_vecCount)
			return B_BUFFER_OVERFLOW;
		if ((uint32)index >= fCount)
			break;

		off_t diff = offset - fOffsets[index];
		vecs[count].offset = fVecs[index].offset + diff;
		vecs[count].length = fVecs[index].length - diff;
		_Verbose("      [%lu] offset %lld, length %lld", count,
			vecs[count].offset, vecs[count].length);

		length -= vecs[count].length;
		offset += vecs[count].length;
		index++;
		count++;
	}

	*_vecCount = count;
	return B_OK;
}


void
Map::_Error(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	fprintf(stderr, "ERROR %s: ", fName);
	vfprintf(stderr, format, args);
	fputc('\n', stderr);

	va_end(args);
	
	fprintf(stderr, "  size %lld\n", fSize);

	for (uint32 i = 0; i < fCount; i++) {
		fprintf(stderr, "  [%lu] offset %lld, length %lld, disk offset %lld\n",
			i, fOffsets[i], fVecs[i].length, fVecs[i].offset);
	}

	if (fTestCount > 0) {
		fprintf(stderr, "got for offset %lld, length %lld:\n",
			fTestOffset, fTestLength);
	}

	for (uint32 i = 0; i < fTestCount; i++) {
		fprintf(stderr, "  [%lu] offset %lld, length %lld\n",
			i, fTestVecs[i].offset, fTestVecs[i].length);
	}

	fflush(stderr);

	debugger("file map error");
	exit(1);
}


void
Map::_Verbose(const char* format, ...)
{
	if (!sVerbose)
		return;

	va_list args;
	va_start(args, format);

	vprintf(format, args);
	putchar('\n');

	va_end(args);
	fflush(stdout);
}


int32
Map::_IndexFor(off_t offset)
{
	for (uint32 i = 0; i < fCount; i++) {
		if (offset >= fOffsets[i] && offset < fOffsets[i] + fVecs[i].length)
			return i;
	}

	return -1;
}


//	#pragma mark - VFS support functions


extern "C" status_t
vfs_get_file_map(struct vnode* vnode, off_t offset, uint32 length,
	file_io_vec* vecs, size_t* _vecCount)
{
	Map* map = (Map*)vnode;
	return map->GetFileMap(offset, length, vecs, _vecCount);
}


extern "C" status_t
vfs_lookup_vnode(dev_t mountID, ino_t vnodeID, struct vnode** _vnode)
{
	*_vnode = (struct vnode*)mountID;
	return B_OK;
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	file_map_init();
	sVerbose = true;

	Map map("shrink1", 4096);
	map.Add(0, 1024, 4096).Add(1024, 3072, 8192);
	map.Test();
	map.SetSize(0).Clear();
	map.Test();
	map.Add(0, 8192, 1000).SetSize(7777);
	map.Test();

	map.SetTo("shrink2", 8888);
	map.Add(0, 10000, 3330000);
	map.Test();
	map.SetSize(0);
	map.SetSize(4444);
	map.Clear();
	map.Add(0, 5000, 2220000);
	map.Test();

	map.SetTo("shrink3", 256000);
	map.Add(0, 98304, 188074464);
	map.Add(98304, 38912, 189057024);
	map.Add(137216, 118784, 189177856);
	map.Test();
	map.SetSize(0);
	map.Test();

	return 0;
}
