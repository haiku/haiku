/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <BlockMap.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


static const off_t kSize = 262144;	// 4096 * 262144 == 1 GB of file size
static const int32 kLoops = 4096;	// 4096 * 4096 == 16 MB in cache (completely arbitrary which will never be the case)


int32
offset_in_array(off_t *array, int32 maxSize, off_t offset)
{
	for (int32 i = 0; i < maxSize; i++) {
		if (array[i] == offset)
			return i;
	}

	return B_ENTRY_NOT_FOUND;
}


int
main(int argc, char **argv)
{
	BlockMap map(kSize);

	status_t status = map.InitCheck();
	if (status != B_OK) {
		fprintf(stderr, "map creation failed: %s\n", strerror(status));
		return 1;
	}

	printf("Adding %ld offsets from 0 to %Ld...\n", kLoops, kSize);

	off_t offsets[kLoops];
	for (int32 i = 0; i < kLoops; i++) {
		off_t offset;
		do {
			offset = rand() * kSize / RAND_MAX;
		} while (offset_in_array(offsets, i, offset) >= B_OK);

		offsets[i] = offset;
		map.Set(offsets[i], i + 1);
	}

	printf("Testing all offsets in the map...\n");

	for (off_t offset = 0; offset < kSize; offset++) {
		// look for offset in array
		int32 index = offset_in_array(offsets, kLoops, offset);

		addr_t address;
		if (map.Get(offset, address) == B_OK) {
			if (index >= 0) {
				if (address != (uint32)index + 1)
					fprintf(stderr, "  offset %Ld contains wrong address %lx, should have been %lx\n", offset, address, index + 1);
			} else
				fprintf(stderr, "  offset %Ld found in map but never added\n", offset);
		} else {
			if (index >= 0)
				fprintf(stderr, "  offset %Ld not found in map but should be there\n", offset);
		}
	}

	printf("Remove last offset...\n");

	// remove last offset
	map.Remove(offsets[kLoops - 1]);
	addr_t address;
	if (map.Get(offsets[kLoops - 1], address) == B_OK)
		fprintf(stderr, "  Removing offset %Ld failed (got address %lx)!\n", offsets[kLoops - 1], address);

	// remove offsets in the middle
	off_t start = kSize / 4;
	off_t num = kSize / 2;
	off_t end = start + num;

	printf("Remove all offsets from %Ld to %Ld (and add bounds check offsets)...\n", start, end);

	bool startWall[3] = {false};
	for (int32 i = 0; i < 3; i++) {
		if (offset_in_array(offsets, kLoops, start - 1 + i) < B_OK) {
			startWall[i] = true;
			map.Set(start - 1 + i, i + 1);
		}
	}

	bool endWall[3] = {false};
	for (int32 i = 0; i < 3; i++) {
		if (offset_in_array(offsets, kLoops, end - 1 + i) < B_OK) {
			endWall[i] = true;
			map.Set(end - 1 + i, i + 1);
		}
	}

	map.Remove(start, num);

	printf("Testing all offsets in the map...\n");

	for (off_t offset = 0; offset < kSize; offset++) {
		// look for offset in array
		int32 index = offset_in_array(offsets, kLoops - 1, offset);

		addr_t address;
		if (map.Get(offset, address) == B_OK) {
			if (offset >= start && offset < end) {
				fprintf(stderr, "  should not contain removed offset %Ld:%lx!\n", offset, address);
			} else if (index >= 0) {
				if (address != (uint32)index + 1)
					fprintf(stderr, "  offset %Ld contains wrong address %lx, should have been %lx\n", offset, address, index + 1);
			} else {
				if (offset >= start -1 && offset <= start + 1) {
					// start && start + 1 must not be in the map anymore
					if (offset >= start && offset <= start + 1)
						fprintf(stderr, "  start wall %Ld in map\n", offset);
				} else if (offset >= end - 1 && offset <= end + 1) {
					// end - 1 must not be in the map anymore
					if (offset == end - 1)
						fprintf(stderr, "  end wall %Ld in map\n", offset);
				} else
					fprintf(stderr, "  offset %Ld found in map but never added\n", offset);
			}
		} else {
			if (index >= 0 && (offset < start || offset >= end))
				fprintf(stderr, "  offset %Ld not found in map but should be there\n", offset);

			if (offset == start - 1 && startWall[offset - start - 1])
				fprintf(stderr, "  start wall %Ld not in map\n", offset);
			else if (offset >= end && offset <= end + 1 && endWall[offset - end - 1])
				fprintf(stderr, "  end wall %Ld not in map\n", offset);
		}
	}

	return 0;
}
