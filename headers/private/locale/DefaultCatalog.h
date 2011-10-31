/*
 * Copyright 2009, Adrien Destugues, pulkomandy@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DEFAULT_CATALOG_H_
#define _DEFAULT_CATALOG_H_


#include <DataIO.h>
#include <HashMapCatalog.h>
#include <String.h>


class BFile;

namespace BPrivate {


/*
 * The implementation of the Locale Kit's standard catalog-type.
 * Currently it only maps CatKey to a BString (the translated string),
 * but the value-type might change to add support for shortcuts and/or
 * graphical data (button-images and the like).
 */
class DefaultCatalog : public BHashMapCatalog {
	public:
		DefaultCatalog(const entry_ref &catalogOwner, const char *language,
			uint32 fingerprint);
				// constructor for normal use
		DefaultCatalog(entry_ref *appOrAddOnRef);
				// constructor for embedded catalog
		DefaultCatalog(const char *path, const char *signature,
			const char *language);
				// constructor for editor-app

		~DefaultCatalog();


		// implementation for editor-interface:
		status_t ReadFromFile(const char *path = NULL);
		status_t ReadFromAttribute(entry_ref *appOrAddOnRef);
		status_t ReadFromResource(entry_ref *appOrAddOnRef);
		status_t WriteToFile(const char *path = NULL);
		status_t WriteToAttribute(entry_ref *appOrAddOnRef);
		status_t WriteToResource(entry_ref *appOrAddOnRef);

		status_t SetRawString(const CatKey& key, const char *translated);
		void SetSignature(const entry_ref &catalogOwner);

		static BCatalogAddOn *Instantiate(const entry_ref& catalogOwner,
			const char *language, uint32 fingerprint);
		static BCatalogAddOn *Create(const char *signature,
			const char *language);

		static const uint8 kDefaultCatalogAddOnPriority;
		static const char *kCatMimeType;

	private:
		status_t Flatten(BDataIO *dataIO);
		status_t Unflatten(BDataIO *dataIO);
		void UpdateAttributes(BFile& catalogFile);

		mutable BString		fPath;
};

extern "C" status_t
default_catalog_get_available_languages(BMessage* availableLanguages,
	const char* sigPattern, const char* langPattern = NULL,
	int32 fingerprint = 0);

}	// namespace BPrivate

using namespace BPrivate;

#endif	/* _DEFAULT_CATALOG_H_ */
