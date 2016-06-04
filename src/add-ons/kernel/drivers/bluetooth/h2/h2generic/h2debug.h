/*
 * Copyright 2015-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 */
#ifndef _H2DEBUG_H
#define _H2DEBUG_H


#include <KernelExport.h>


#ifdef DEBUG
#   define TRACE(x...) dprintf("h2generic: " x)
#else
#   define TRACE(x...) ;
#endif

#define ERROR(x...) dprintf("h2generic: " x)
#define CALLED(x...) TRACE("h2generic: CALLED %s\n", __PRETTY_FUNCTION__)


#endif /* _H2DEBUG_H */
