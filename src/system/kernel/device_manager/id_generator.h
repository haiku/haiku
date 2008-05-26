/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-04, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef ID_GENERATOR_H
#define ID_GENERATOR_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

int32 dm_create_id(const char *name);
status_t dm_free_id(const char *name, uint32 id);

void dm_init_id_generator(void);

#ifdef __cplusplus
}
#endif

#endif	/* ID_GENERATOR_H */
