/*
 *	SiS 7018, Trident 4D Wave DX/NX, Acer Lab M5451 Sound Driver.
 *	Copyright (c) 2002, 2008-2011 S.Zharski <imker@gmx.li>
 *	Distributed under the terms of the MIT license.
 *
 */
#ifndef _SiS7018_SETTINGS_H_
#define _SiS7018_SETTINGS_H_


#include <driver_settings.h>

#include "Driver.h"


#ifdef _countof
#warning "_countof(...) WAS ALREADY DEFINED!!! Remove local definition!"
#undef _countof
#endif
#define _countof(array)(sizeof(array) / sizeof(array[0]))


void load_settings();
void release_settings();

void SiS7018_trace(bool force, const char *func, const char *fmt, ...);

#ifdef TRACE
#undef TRACE
#endif
#define	TRACE(x...) SiS7018_trace(false, __func__, x)

#ifdef ERROR
#undef ERROR
#endif
#define ERROR(x...) SiS7018_trace(true, __func__, x)


#endif //_SiS7018_SETTINGS_H_

