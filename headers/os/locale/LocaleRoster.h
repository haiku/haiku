/*
 * Copyright 2003-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _LOCALE_ROSTER_H_
#define _LOCALE_ROSTER_H_


#include <String.h>


class BLanguage;
class BLocale;
class BCollator;
class BCountry;
class BCatalog;
class BCatalogAddOn;
class BMessage;
class BTimeZone;

struct entry_ref;

namespace BPrivate {
	class EditableCatalog;
}


enum {
	B_LOCALE_CHANGED	= '_LCC',
};


class BLocaleRoster {
	public:
		BLocaleRoster();
		~BLocaleRoster();

		status_t GetSystemCatalog(BCatalogAddOn **catalog) const;
		status_t GetDefaultCollator(BCollator **collator) const;
		status_t GetDefaultLanguage(BLanguage **language) const;
		status_t GetDefaultCountry(BCountry **contry) const;
		status_t GetDefaultTimeZone(BTimeZone**timezone) const;

		void SetDefaultCountry(BCountry *) const;
		void SetDefaultTimeZone(const char* zone) const;
		void UpdateSettings(BMessage* newSettings);

		status_t GetLanguage(const char* languageCode, BLanguage** _language)
			const;

		status_t GetPreferredLanguages(BMessage *) const;
		status_t SetPreferredLanguages(BMessage *);
			// the message contains one or more 'language'-string-fields
			// which contain the language-name(s)

		status_t GetInstalledLanguages(BMessage *) const;
			// the message contains one or more 'language'-string-fields
			// which contain the language-name(s)

		status_t GetAvailableCountries(BMessage *) const;

		status_t GetInstalledCatalogs(BMessage *, const char* sigPattern = NULL,
			const char* langPattern = NULL,	int32 fingerprint = 0) const;
			// the message contains...

		BCatalog* GetCatalog();
			// Get the catalog for the calling image (that needs to link with
			// liblocalestub.a)

		static const char *kCatLangAttr;
		static const char *kCatSigAttr;
		static const char *kCatFingerprintAttr;

		static const char *kCatManagerMimeType;
		static const char *kCatEditorMimeType;

		static const char *kEmbeddedCatAttr;
		static int32 kEmbeddedCatResId;

	private:
		BCatalog* GetCatalog(BCatalog* catalog, vint32* catalogInitStatus);

		BCatalogAddOn* LoadCatalog(const char *signature,
			const char *language = NULL, int32 fingerprint = 0);
		BCatalogAddOn* LoadEmbeddedCatalog(entry_ref *appOrAddOnRef);
		status_t UnloadCatalog(BCatalogAddOn *addOn);

		BCatalogAddOn* CreateCatalog(const char *type,
			const char *signature, const char *language);

		friend class BCatalog;
		friend class BCatalogStub;
		friend class BPrivate::EditableCatalog;
		friend status_t get_add_on_catalog(BCatalog*, const char *);
};

#endif	/* _LOCALE_ROSTER_H_ */
