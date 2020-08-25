/*
 * Copyright 2010-2012, Haiku. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _MUTABLE_LOCALE_ROSTER_H_
#define _MUTABLE_LOCALE_ROSTER_H_


#include <Collator.h>
#include <FormattingConventions.h>
#include <image.h>
#include <Language.h>
#include <List.h>
#include <Locale.h>
#include <Locker.h>
#include <LocaleRoster.h>
#include <Message.h>
#include <Resources.h>
#include <TimeZone.h>


class BLocale;
class BCatalog;
class BCatalogData;

struct entry_ref;


namespace BPrivate {


class MutableLocaleRoster : public BLocaleRoster {
public:
								MutableLocaleRoster();
								~MutableLocaleRoster();

	static	MutableLocaleRoster* Default();

			status_t			SetDefaultFormattingConventions(
									const BFormattingConventions& conventions);
			status_t			SetDefaultTimeZone(const BTimeZone& zone);

			status_t			SetPreferredLanguages(const BMessage* message);
									// the message contains one or more
									// 'language'-string-fields which
									// contain the language-name(s)
			status_t			SetFilesystemTranslationPreferred(
									bool preferred);

			status_t			LoadSystemCatalog(BCatalog* catalog) const;

			BCatalogData*		LoadCatalog(const entry_ref& catalogOwner,
									const char* language = NULL,
									int32 fingerprint = 0) const;
			BCatalogData*		LoadCatalog(const char* signature,
									const char* language = NULL) const;
			status_t			UnloadCatalog(BCatalogData* catalogData);

			BCatalogData*		CreateCatalog(const char* type,
									const char* signature,
									const char* language);
};


}	// namespace BPrivate


#endif	// _MUTABLE_LOCALE_ROSTER_H_
