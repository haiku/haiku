/*
 * Copyright 2022, Trung Nguyen, trungnt282910@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <new>
#include <errno.h>
#include <locale.h>
#include <strings.h>

#include <ErrnoMaintainer.h>

#include "LocaleBackend.h"
#include "LocaleInternal.h"


using BPrivate::Libroot::gGlobalLocaleBackend;
using BPrivate::Libroot::gGlobalLocaleDataBridge;
using BPrivate::Libroot::GetCurrentLocaleInfo;
using BPrivate::Libroot::GetLocalesFromEnvironment;
using BPrivate::Libroot::LocaleBackend;
using BPrivate::Libroot::LocaleBackendData;
using BPrivate::Libroot::LocaleDataBridge;


extern "C" locale_t
duplocale(locale_t l)
{
    LocaleBackendData* locObj = (LocaleBackendData*)l;

    LocaleBackendData* newObj = new (std::nothrow) LocaleBackendData;
    if (newObj == NULL) {
        errno = ENOMEM;
        return (locale_t)0;
    }

    LocaleBackend* backend = (l == LC_GLOBAL_LOCALE) ?
        gGlobalLocaleBackend : (LocaleBackend*)locObj->backend;

    if (backend == NULL) {
        newObj->backend = NULL;
        return (locale_t)newObj;
    }

    // Check if everything is set to "C" or "POSIX",
    // and avoid making a backend.
    const char* localeDescription = backend->SetLocale(LC_ALL, NULL);

    if ((strcasecmp(localeDescription, "POSIX") == 0)
        || (strcasecmp(localeDescription, "C") == 0)) {
        newObj->backend = NULL;
        return (locale_t)newObj;
    }

    BPrivate::ErrnoMaintainer errnoMaintainer;

    LocaleBackend*& newBackend = newObj->backend;
    LocaleDataBridge*& newDataBridge = newObj->databridge;
    newBackend = LocaleBackend::CreateBackend();

    if (newBackend == NULL) {
        errno = ENOMEM;
        delete newObj;
        return (locale_t)0;
    }

    newDataBridge = new (std::nothrow) LocaleDataBridge(false);

    if (newDataBridge == NULL) {
        errno = ENOMEM;
        delete newBackend;
        delete newObj;
        return (locale_t)0;
    }

    newBackend->Initialize(newDataBridge);

    // Skipping LC_ALL. Asking for LC_ALL would force the backend
    // to query each other value once, anyway.
    for (int lc = 1; lc <= LC_LAST; ++lc) {
        newBackend->SetLocale(lc, backend->SetLocale(lc, NULL));
    }

    return (locale_t)newObj;
}


extern "C" void
freelocale(locale_t l)
{
    LocaleBackendData* locobj = (LocaleBackendData*)l;

    if (locobj->backend) {
        LocaleBackend::DestroyBackend(locobj->backend);
        LocaleDataBridge* databridge = locobj->databridge;
        delete databridge;
    }
    delete locobj;
}


extern "C" locale_t
newlocale(int category_mask, const char* locale, locale_t base)
{
	if (((category_mask | LC_ALL_MASK) != LC_ALL_MASK) || (locale == NULL)) {
		errno = EINVAL;
		return (locale_t)0;
	}

	bool newObject = false;
	LocaleBackendData* localeObject = (LocaleBackendData*)base;

	if (localeObject == NULL) {
		localeObject = new (std::nothrow) LocaleBackendData;
		if (localeObject == NULL) {
			errno = ENOMEM;
			return (locale_t)0;
		}
		localeObject->magic = LOCALE_T_MAGIC;
		localeObject->backend = NULL;
		localeObject->databridge = NULL;

		newObject = true;
	}

	LocaleBackend*& backend = localeObject->backend;
	LocaleDataBridge*& databridge = localeObject->databridge;

	const char* locales[LC_LAST + 1];
	for (int lc = 0; lc <= LC_LAST; lc++)
		locales[lc] = NULL;

	if (*locale == '\0') {
		if (category_mask == LC_ALL_MASK) {
			GetLocalesFromEnvironment(LC_ALL, locales);
		} else {
			for (int lc = 1; lc <= LC_LAST; ++lc) {
				if (category_mask & (1 << (lc - 1))) {
					GetLocalesFromEnvironment(lc, locales);
				}
			}
		}
	} else {
		if (category_mask == LC_ALL_MASK) {
			locales[LC_ALL] = locale;
		}
		for (int lc = 1; lc <= LC_LAST; ++lc) {
			if (category_mask & (1 << (lc - 1))) {
				locales[lc] = locale;
			}
		}
	}

	if (backend == NULL) {
		// for any locale other than POSIX/C, we try to activate the ICU
		// backend
		bool needBackend = false;
		for (int lc = 0; lc <= LC_LAST; lc++) {
			if (locales[lc] != NULL && strcasecmp(locales[lc], "POSIX") != 0
					&& strcasecmp(locales[lc], "C") != 0) {
				needBackend = true;
				break;
			}
		}
		if (needBackend) {
			backend = LocaleBackend::CreateBackend();
			if (backend == NULL) {
				errno = ENOMEM;
				if (newObject) {
					delete localeObject;
				}
				return (locale_t)0;
			}
			databridge = new (std::nothrow) LocaleDataBridge(false);
			if (databridge == NULL) {
				errno = ENOMEM;
				delete backend;
				if (newObject) {
					delete localeObject;
				}
				return (locale_t)0;
			}
			backend->Initialize(databridge);
		}
	}

	BPrivate::ErrnoMaintainer errnoMaintainer;

	if (backend != NULL) {
		for (int lc = 0; lc <= LC_LAST; lc++) {
			if (locales[lc] != NULL) {
				locale = backend->SetLocale(lc, locales[lc]);
				if (lc == LC_ALL) {
					// skip the rest, LC_ALL overrides
					break;
				}
			}
		}
	}

	return (locale_t)localeObject;
}


extern "C" locale_t
uselocale(locale_t newLoc)
{
    locale_t oldLoc = (locale_t)GetCurrentLocaleInfo();
    if (oldLoc == NULL) {
        oldLoc = LC_GLOBAL_LOCALE;
    }

    if (newLoc != (locale_t)0) {
        // Avoid expensive TLS reads with a local variable.
        locale_t appliedLoc = oldLoc;

        if (newLoc == LC_GLOBAL_LOCALE) {
            appliedLoc = NULL;
        } else {
            if (((LocaleBackendData*)newLoc)->magic != LOCALE_T_MAGIC) {
                errno = EINVAL;
                return (locale_t)0;
            }
            appliedLoc = newLoc;
        }

        SetCurrentLocaleInfo((LocaleBackendData*)appliedLoc);

        if (appliedLoc != NULL) {
            ((LocaleBackendData*)appliedLoc)->databridge->ApplyToCurrentThread();
        } else {
            gGlobalLocaleDataBridge.ApplyToCurrentThread();
        }
    }

    return oldLoc;
}
