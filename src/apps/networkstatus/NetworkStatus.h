/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef NETWORK_STATUS_H
#define NETWORK_STATUS_H


#include <image.h>


extern const char* kSignature;
extern const char* kDeskbarItemName;

status_t our_image(image_info& image);

#endif	// NETWORK_STATUS_H
