/*
 * Copyright 2015-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef _BTDEBUG_H
#define _BTDEBUG_H


#ifdef DEBUG
#   define TRACE(x...) dprintf("bt: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) dprintf("bt: " x)
#define CALLED(x...) TRACE("bt: CALLED %s\n", __PRETTY_FUNCTION__)


#endif /* _BTDEBUG_H */
