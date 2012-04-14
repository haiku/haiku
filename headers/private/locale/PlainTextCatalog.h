/*
 * Copyright 2009, Adrien Destugues, pulkomandy@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PLAINTEXT_CATALOG_H_
#define _PLAINTEXT_CATALOG_H_


#include <HashMapCatalog.h>
#include <DataIO.h>
#include <String.h>


class BFile;

namespace BPrivate {


class PlainTextCatalog : public HashMapCatalog {
	public:
		PlainTextCatalog(const char *signature, const char *language,
			uint32 fingerprint);
				// constructor for normal use
		PlainTextCatalog(entry_ref *appOrAddOnRef);
				// constructor for embedded catalog
		PlainTextCatalog(const char *path, const char *signature,
			const char *language);
				// constructor for editor-app

		~PlainTextCatalog();

		// implementation for editor-interface:
		status_t ReadFromFile(const char *path = NULL);
		status_t WriteToFile(const char *path = NULL);

		static BCatalogAddOn *Instantiate(const char *signature,
			const char *language, uint32 fingerprint);

		static const char *kCatMimeType;

	private:
		void UpdateAttributes(BFile& catalogFile);
		void UpdateAttributes(const char* path);

		mutable BString		fPath;
};


} // namespace BPrivate


#endif /* _PLAINTEXT_CATALOG_H_ */
