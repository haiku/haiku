/*
 * Copyright 2010, Adrien Destugues <pulkomandy@pulkomandy.ath.cx>.
 * Distributed under the terms of the MIT License.
 */


#ifndef __CATALOG_STUB_H__
#define __CATALOG_STUB_H__


#define B_TRANSLATE_USE_NEW_MACROS


#include <Catalog.h>
#include <Locale.h>
#include <LocaleRoster.h>

BCatalog BCatalogStub::sCatalog;
vint32 BCatalogStub::sCatalogInitOnce = false;


/* static */ BCatalog*
BCatalogStub::GetCatalog()
{
	return be_locale_roster->GetCatalog(&sCatalog, &sCatalogInitOnce);
}


#endif
