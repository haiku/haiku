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
		PlainTextCatalog(const entry_ref& owner, const char *language,
			uint32 fingerprint);
				// constructor for normal use
		PlainTextCatalog(const char *path, const char *signature,
			const char *language);
				// constructor for editor-app

		~PlainTextCatalog();

		void SetSignature(const entry_ref &catalogOwner);

		// implementation for editor-interface:
		status_t ReadFromFile(const char *path = NULL);
		status_t WriteToFile(const char *path = NULL);

		static BCatalogData *Instantiate(const entry_ref &signature,
			const char *language, uint32 fingerprint);

		static const char *kCatMimeType;

	private:
		void UpdateAttributes(BFile& catalogFile);
		void UpdateAttributes(const char* path);

		mutable BString		fPath;
};


} // namespace BPrivate


#endif /* _PLAINTEXT_CATALOG_H_ */
