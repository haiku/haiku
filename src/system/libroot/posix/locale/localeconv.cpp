/*
 * Copyright 2002-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 * 		Daniel Reinhold, danielre@users.sf.net
 * 		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <PosixLocaleConv.h>

#ifndef _KERNEL_MODE
#include <locale.h>
#include "LocaleBackend.h"

using BPrivate::Libroot::GetCurrentLocaleBackend;
using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleBackendData;
#endif


extern "C" struct lconv*
localeconv(void)
{
#ifndef _KERNEL_MODE
	LocaleBackend* backend = GetCurrentLocaleBackend();
	if (backend != NULL)
		return const_cast<lconv*>(backend->LocaleConv());
#endif

	return &BPrivate::Libroot::gPosixLocaleConv;
}


#ifndef _KERNEL_MODE
extern "C" struct lconv*
localeconv_l(locale_t l)
{
	LocaleBackendData* locale = (LocaleBackendData*)l;
	LocaleBackend* backend = locale->backend;

	if (backend != NULL)
		return const_cast<lconv*>(backend->LocaleConv());

	return &BPrivate::Libroot::gPosixLocaleConv;
}
#endif