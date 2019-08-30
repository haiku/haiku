/*
 * Copyright 2003, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef AREA_UTILS_H
#define AREA_UTILS_H

namespace AreaUtils {

	void *calloc(size_t nmemb, size_t size);
	void free(void *ptr);
	void *malloc(size_t size);
	void *realloc(void * ptr, size_t size);

};

#endif	// AREA_UTILS_H
