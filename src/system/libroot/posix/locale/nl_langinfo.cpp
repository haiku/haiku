/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <langinfo.h>

#include "LocaleBackend.h"

#include <PosixLanginfo.h>


using BPrivate::Libroot::gLocaleBackend;
using BPrivate::Libroot::gPosixLanginfo;


extern "C" char*
nl_langinfo(nl_item item)
{
	if (item < 0 || item >= _NL_LANGINFO_LAST)
		return const_cast<char*>("");

	if (gLocaleBackend != NULL)
		return const_cast<char*>(gLocaleBackend->GetLanginfo(item));

	return const_cast<char*>(gPosixLanginfo[item]);
}
