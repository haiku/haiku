/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include <SupportDefs.h>


namespace Utility {
	bool IsReadOnlyVolume(dev_t device);
	bool IsReadOnlyVolume(const char* path);

	status_t BlockMedia(const char* path, bool block);
}


#endif // UTILITY_H
