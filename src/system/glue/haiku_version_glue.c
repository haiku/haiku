/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <SupportDefs.h>

#include <image_defs.h>


// Haiku API and ABI versions -- we compile those into shared objects so that
// the runtime loader can discriminate the versions and enable compatibility
// work-arounds, if necessary.

uint32 B_SHARED_OBJECT_HAIKU_VERSION_VARIABLE = B_HAIKU_VERSION;
uint32 B_SHARED_OBJECT_HAIKU_ABI_VARIABLE = B_HAIKU_ABI;
