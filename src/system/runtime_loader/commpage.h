/*
 * Copyright 2025, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef COMMPAGE_H
#define COMMPAGE_H

#include <image.h>


status_t commpage_reinit_after_fork();

// Caller must hold the lock when calling this function.
status_t get_nearest_commpage_symbol_at_address_locked(void* address,
	image_id* _imageID, char** _imagePath, char** _imageName,
	char** _symbolName, int32* _type, void** _location, bool* _exactMatch);


#endif	// COMMPAGE_H
