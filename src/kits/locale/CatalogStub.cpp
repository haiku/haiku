/*
 * Copyright 2010-2016, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * Distributed under the terms of the MIT License.
 */


#include <Catalog.h>
#include <LocaleRoster.h>

#include <locks.h>


static int32 sCatalogInitOnce = INIT_ONCE_UNINITIALIZED;


BCatalog*
BLocaleRoster::GetCatalog()
{
	static BCatalog sCatalog;

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

