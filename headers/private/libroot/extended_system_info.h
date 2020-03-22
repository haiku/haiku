/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBROOT_EXTENDED_SYSTEM_INFO_H
#define _LIBROOT_EXTENDED_SYSTEM_INFO_H


/* This is C++ only API. */
#ifdef __cplusplus


#include <OS.h>


namespace BPrivate {


class KMessage;


status_t get_extended_team_info(team_id teamID, uint32 flags, KMessage& info);


}	// namespace BPrivate


#endif	/* __cplusplus */


#endif	/* _LIBROOT_EXTENDED_SYSTEM_INFO_H */
