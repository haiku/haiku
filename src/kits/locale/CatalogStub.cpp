/*
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>.
 * Distributed under the terms of the MIT License.
 */


#ifndef __CATALOG_STUB_H__
#define __CATALOG_STUB_H__


#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>


BCatalog BCatalogStub::sCatalog;
vint32 BCatalogStub::sCatalogInitOnce = false;


/* static */ BCatalog*
BCatalogStub::GetCatalog()
{
	#if (__GNUC__ < 3)
		asm volatile(".hidden GetCatalog__12BCatalogStub");
	#else
		asm volatile(".hidden _ZN12BCatalogStub10GetCatalogEv");
	#endif

	return be_locale_roster->GetCatalog(&sCatalog, &sCatalogInitOnce);
}


/* static */ void
BCatalogStub::ForceReload()
{
	sCatalogInitOnce = false;
}


#endif
