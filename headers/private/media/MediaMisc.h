/*
 * Copyright (c) 2003 Marcus Overhagen.
 * All Rights Reserved.
 *
 * This file may be used under the terms of the MIT License.
 */
#ifndef _MEDIA_MISC_H_
#define _MEDIA_MISC_H_


// Used by Haiku apps to make media services notifications
void
progress_startup(int stage,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie);

void
progress_shutdown(int stage,
	bool (*progress)(int stage, const char* message, void* cookie),
	void* cookie);


#define IS_INVALID_NODE(_node) 			((_node).node <= 0 || (_node).port <= 0)
#define IS_INVALID_NODEID(_id) 			((_id) <= 0)
#define IS_INVALID_SOURCE(_src)			((_src).port <= 0)
#define IS_INVALID_DESTINATION(_dest) 	((_dest).port <= 0)

#define NODE_JUST_CREATED_ID			-1
#define NODE_UNREGISTERED_ID			-2
#define NODE_SYSTEM_TIMESOURCE_ID		1

#define BAD_MEDIA_SERVER_PORT			-222
#define BAD_MEDIA_ADDON_SERVER_PORT		-444

#define IS_SYSTEM_TIMESOURCE(_node)		((_node).node == NODE_SYSTEM_TIMESOURCE_ID)

#define NODE_KIND_USER_MASK 			0x00000000FFFFFFFFULL
#define NODE_KIND_COMPARE_MASK 			0x000000007FFFFFFFULL
#define NODE_KIND_NO_REFCOUNTING		0x0000000080000000ULL
#define NODE_KIND_SHADOW_TIMESOURCE		0x0000000100000000ULL

#define ROUND_UP_TO_PAGE(size)			(((size) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1))

#define MEDIA_SERVER_PORT_NAME			"__media_server_port"
#define MEDIA_ADDON_SERVER_PORT_NAME	"__media_addon_server_port"

#define BUFFER_TO_RECLAIM				0x20000000
#define BUFFER_MARKED_FOR_DELETION		0x40000000


extern const char *B_MEDIA_ADDON_SERVER_SIGNATURE;

namespace BPrivate { namespace media {
	extern team_id team;
} } // BPrivate::media

using namespace BPrivate::media;

#endif
