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

#define NODE_UNREGISTERED_ID	-2

#define SHADOW_TIMESOURCE_CONTROL_PORT	-333
#define IS_SHADOW_TIMESOURCE(_node)		((_node).node > 0 && (_node).port == SHADOW_TIMESOURCE_CONTROL_PORT)

#define SYSTEM_TIMESOURCE_CONTROL_PORT	-666
#define IS_SYSTEM_TIMESOURCE(_node)		((_node).node > 0 && (_node).port == SYSTEM_TIMESOURCE_CONTROL_PORT)

#define NODE_KIND_USER_MASK 			0x00000000FFFFFFFFLL
#define NODE_KIND_COMPARE_MASK 			0x000000007FFFFFFFLL
#define NODE_KIND_NO_REFCOUNTING		0x0000000080000000LL
#define NODE_KIND_SHADOW_TIMESOURCE		0x0000000100000000LL
#define NODE_KIND_SYSTEM_TIMESOURCE		0x0000000200000000LL

#define ROUND_UP_TO_PAGE(size)			(((size) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1))

namespace BPrivate { namespace media {
	extern team_id team;
} } // BPrivate::media

using namespace BPrivate::media;

#endif
