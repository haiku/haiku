/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	VM helper functions.

	Important assumption: get_memory_map must combine adjacent
	physical pages, so contignous memory always leads to a S/G
	list of length one.
*/

#include "KernelExport_ext.h"
#include "wrapper.h"

#include <string.h>

#include <algorithm>


/**	get sg list of iovec
 *	TBD: this should be moved to somewhere in kernel
 */

status_t
get_iovec_memory_map(iovec *vec, size_t vec_count, size_t vec_offset, size_t len,
	physical_entry *map, uint32 max_entries, uint32 *num_entries, size_t *mapped_len)
{
	uint32 cur_idx;
	size_t left_len;

	SHOW_FLOW(3, "vec_count=%" B_PRIuSIZE ", vec_offset=%" B_PRIuSIZE ", len=%"
		B_PRIuSIZE ", max_entries=%" B_PRIu32, vec_count, vec_offset, len,
		max_entries);

	// skip iovec blocks if needed
	while (vec_count > 0 && vec_offset > vec->iov_len) {
		vec_offset -= vec->iov_len;
		--vec_count;
		++vec;
	}

	for (left_len = len, cur_idx = 0; left_len > 0 && vec_count > 0
		&& cur_idx < max_entries;) {
		char *range_start;
		size_t range_len;
		status_t res;
		uint32 cur_num_entries, cur_mapped_len;
		uint32 tmp_idx;

		SHOW_FLOW( 3, "left_len=%d, vec_count=%d, cur_idx=%" B_PRIu32,
			(int)left_len, (int)vec_count, cur_idx );

		// map one iovec
		range_start = (char *)vec->iov_base + vec_offset;
		range_len = std::min(vec->iov_len - vec_offset, left_len);

		SHOW_FLOW( 3, "range_start=%" B_PRIxADDR ", range_len=%" B_PRIxSIZE,
			(addr_t)range_start, range_len );

		vec_offset = 0;

		cur_num_entries = max_entries - cur_idx;
		if ((res = get_memory_map_etc(B_CURRENT_TEAM, range_start, range_len,
				&map[cur_idx], &cur_num_entries)) != B_OK) {
			// according to docu, no error is ever reported - argh!
			SHOW_ERROR(1, "invalid io_vec passed (%s)", strerror(res));
			return res;
		}

		cur_mapped_len = 0;

		for (tmp_idx = cur_idx; tmp_idx < cur_idx + cur_num_entries; ++tmp_idx)
			cur_mapped_len += map[tmp_idx].size;

		if (cur_mapped_len == 0) {
			panic("get_memory_map() returned empty list; left_len=%d, idx=%"
				B_PRIu32 "/%" B_PRIu32, (int)left_len, cur_idx, max_entries);
			SHOW_ERROR(2, "get_memory_map() returned empty list; left_len=%d, "
				"idx=%" B_PRIu32 "/%" B_PRIu32, (int)left_len, cur_idx,
				max_entries);
			return B_ERROR;
		}

		SHOW_FLOW( 3, "cur_num_entries=%" B_PRIu32 ", cur_mapped_len=%x",
			cur_num_entries, (int)cur_mapped_len );

		// try to combine with previous sg block
		if (cur_num_entries > 0 && cur_idx > 0
			&& map[cur_idx].address
				== map[cur_idx - 1].address + map[cur_idx - 1].size) {
			SHOW_FLOW0( 3, "combine with previous chunk" );
			map[cur_idx - 1].size += map[cur_idx].size;
			memcpy(&map[cur_idx], &map[cur_idx + 1],
				(cur_num_entries - 1) * sizeof(map[0]));
			--cur_num_entries;
		}

		cur_idx += cur_num_entries;
		left_len -= cur_mapped_len;

		// advance iovec if current one is described completely
		if (cur_mapped_len == range_len) {
			++vec;
			--vec_count;
		}
	}

	*num_entries = cur_idx;
	*mapped_len = len - left_len;

	SHOW_FLOW( 3, "num_entries=%" B_PRIu32 ", mapped_len=%" B_PRIxSIZE,
		*num_entries, *mapped_len);

	return B_OK;
}

