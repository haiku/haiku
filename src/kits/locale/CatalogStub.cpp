/*
 * Copyright 2010-2014, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * Distributed under the terms of the MIT License.
 */


#include <Catalog.h>
#include <LocaleRoster.h>

#include <locks.h>


static BCatalog sCatalog;
static int32 sCatalogInitOnce = INIT_ONCE_UNINITIALIZED;


BCatalog*
BLocaleRoster::GetCatalog()
{
	#if (__GNUC__ < 3)
		asm volatile(".hidden GetCatalog__13BLocaleRoster");
	#else
		asm volatile(".hidden _ZN13BLocaleRoster10GetCatalogEv");
	#endif

	return _GetCatalog(&sCatalog, &sCatalogInitOnce);
}


namespace BPrivate{
	void ForceUnloadCatalog()
	{
		sCatalogInitOnce = INIT_ONCE_UNINITIALIZED;
	}
}

