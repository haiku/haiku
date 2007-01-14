/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */

#include "GlobalData.h"

int				gFd;
SharedInfo		* gSi;
area_id			gSharedArea;
int				gAccelerantIsClone;
thread_id		gUpdateThread = B_ERROR;
volatile int	gUpdateThreadDie;
