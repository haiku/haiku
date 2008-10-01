/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CAM_DEBUG_H
#define _CAM_DEBUG_H

#include <Debug.h>

/* allow overriding ANSI color */
#ifndef CD_COL
#define CD_COL "34"
#endif

#define CH "\033[" CD_COL "mWebcam::%s::%s"
#define CT "\033[0m\n", __BASE_FILE__, __FUNCTION__

#endif
