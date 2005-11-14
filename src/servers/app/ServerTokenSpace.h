/*
 * Copyright 2005, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef	SERVER_TOKEN_SPACE_H
#define	SERVER_TOKEN_SPACE_H


#include <TokenSpace.h>


using BPrivate::BTokenSpace;

const int32 kCursorToken = 3;
const int32 kBitmapToken = 4;
const int32 kPictureToken = 5;


extern BTokenSpace gTokenSpace;

#endif	/* SERVER_TOKEN_SPACE_H */
