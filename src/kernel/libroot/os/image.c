/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <image.h>

#include "syscalls.h"
#include "user_runtime.h"


void __init__image(struct uspace_program_args const *);

static struct rld_export const *gRuntimeLinker;


image_id
load_add_on(char const *name)
{
	return gRuntimeLinker->load_add_on(name, 0);
}


status_t
unload_add_on(image_id id)
{
	return gRuntimeLinker->unload_add_on(id);
}


status_t
get_image_symbol(image_id id, char const *symbolName, int32 symbolType, void **_location)
{
	return gRuntimeLinker->get_image_symbol(id, symbolName, symbolType, _location);
}


status_t
get_nth_image_symbol(image_id id, int32 num, char *nameBuffer, int32 *_nameLength,
	int32 *_symbolType, void **_location)
{
	return gRuntimeLinker->get_nth_image_symbol(id, num, nameBuffer, _nameLength, _symbolType, _location);
}


status_t
_get_image_info(image_id id, image_info *info, size_t infoSize)
{
	return sys_get_image_info(id, info, infoSize);
}


status_t
_get_next_image_info(team_id team, int32 *cookie, image_info *info, size_t infoSize)
{
	return sys_get_next_image_info(team, cookie, info, infoSize);
}


void
__init__image(struct uspace_program_args const *args)
{
	gRuntimeLinker = args->rld_export;
}
