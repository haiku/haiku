/*
 * Copyright (c) 2003 Marcus Overhagen.
 * All Rights Reserved.
 *
 * This file may be used under the terms of the MIT License.
 */

#ifndef _MEDIA_MISC_H_
#define _MEDIA_MISC_H_

#define IS_INVALID_NODE(_node) 			((_node).node <= 0 || (_node).port <= 0)
#define IS_INVALID_NODEID(_id) 			((_id) <= 0)
#define IS_INVALID_SOURCE(_src)			((_src).port <= 0)
#define IS_INVALID_DESTINATION(_dest) 	((_dest).port <= 0)

#define ROUND_UP_TO_PAGE(size)			(((size) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1))

#endif
