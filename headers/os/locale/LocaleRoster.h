/*
 * Copyright 2003-2011, Haiku. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _LOCALE_ROSTER_H_
#define _LOCALE_ROSTER_H_


#include <Entry.h>
#include <String.h>


class BBitmap;
class BCatalog;
class BCollator;
class BCountry;
class BFormattingConventions;
class BLanguage;
class BLocale;
class BMessage;
class BTimeZone;


enum {
	B_LOCALE_CHANGED = '_LCC',
};


class BLocaleRoster {
public:
								BLocaleRoster();
								~BLocaleRoster();

	static	BLocaleRoster*		Default();

			status_t			GetDefaultTimeZone(BTimeZone* timezone) const;

			status_t			GetLanguage(const char* languageCode,
									BLanguage** _language) const;

			status_t			GetPreferredLanguages(BMessage* message) const;

			status_t			GetAvailableLanguages(BMessage* message) const;
			status_t			GetAvailableCountries(
									BMessage* timeZones) const;
			status_t			GetAvailableTimeZones(
									BMessage* timeZones) const;
			status_t			GetAvailableTimeZonesWithRegionInfo(
									BMessage* timeZonesWithRegonInfo) const;
			status_t			GetAvailableTimeZonesForCountry(
									BMessage* message,
									const char* countryCode) const;

			status_t			GetFlagIconForCountry(BBitmap* flagIcon,
									const char* countryCode);
			status_t			GetFlagIconForLanguage(BBitmap* flagIcon,
									const char* languageCode);

			status_t			GetAvailableCatalogs(BMessage* message,
									const char* sigPattern = NULL,
									const char* langPattern = NULL,
									int32 fingerprint = 0) const;
									// the message contains...

			status_t			Refresh();
									// Refresh the internal data from the
									// settings file(s)

			BCatalog*			GetCatalog();
									// Get the catalog for the calling image
									// (that needs to link with liblocalestub.a)

			bool				IsFilesystemTranslationPreferred() const;

			status_t			GetLocalizedFileName(BString& localizedFileName,
									const entry_ref& ref,
									bool traverse = false);

	static	const char*			kCatLangAttr;
	static	const char*			kCatSigAttr;
	static	const char*			kCatFingerprintAttr;

	static	const char*			kEmbeddedCatAttr;
	static	int32				kEmbeddedCatResId;

private:
	static	BCatalog*			_GetCatalog(BCatalog* catalog,
									vint32* catalogInitStatus);

			status_t			_PrepareCatalogEntry(const entry_ref& ref,
									BString& signature, BString& context,
									BString& string, bool traverse);
};


#endif	// _LOCALE_ROSTER_H_
