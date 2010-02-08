/*
 * Copyright 2010, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef MALLOC_DEBUG_H
#define MALLOC_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

extern status_t heap_debug_start_wall_checking(int msInterval);
extern status_t heap_debug_stop_wall_checking();

extern void heap_debug_set_paranoid_validation(bool enabled);
extern void heap_debug_validate_heaps();
extern void heap_debug_validate_walls();

extern void heap_debug_dump_allocations(bool statsOnly, thread_id thread);
extern void heap_debug_dump_heaps(bool dumpAreas, bool dumpBins);

#ifdef __cplusplus
}
#endif

#endif /* MALLOC_DEBUG_H */
