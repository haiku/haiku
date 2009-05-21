/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ELF_VERSIONING_H
#define ELF_VERSIONING_H

#include "runtime_loader_private.h"


status_t	init_image_version_infos(image_t* image);
status_t	check_needed_image_versions(image_t* image);


#endif	// ELF_VERSIONING_H
