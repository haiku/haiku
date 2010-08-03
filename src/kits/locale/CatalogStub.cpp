/*
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>.
 * Distributed under the terms of the MIT License.
 */


#include <Catalog.h>
#include <LocaleRoster.h>


static BCatalog sCatalog;
static vint32 sCatalogInitOnce = false;


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
		sCatalogInitOnce = false;
	}
}

