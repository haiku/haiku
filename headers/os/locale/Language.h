/*
 * Copyright 2003-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_


#include <LocaleStrings.h>
#include <String.h>
#include <SupportDefs.h>


// We must not include the icu headers in there as it could mess up binary
// compatibility.
namespace icu_44 {
	class Locale;
}


enum script_direction {
	B_LEFT_TO_RIGHT = 0,
	B_RIGHT_TO_LEFT,
	B_TOP_TO_BOTTOM,	// seems not to be supported anywhere else?
};


class BLanguage {
public:
								~BLanguage();

			status_t			GetName(BString& name) const;
			status_t			GetTranslatedName(BString& name) const;

			// ISO-639 language code, e.g. "en", "de"
			const char*			Code() const;
			const char*			Country() const;
			const char*			Variant() const;
			const char*			ID() const;

			bool				IsCountrySpecific() const;
			bool				IsVariant() const;

			uint8				Direction() const;

			// see definitions below
			const char*			GetString(uint32 id) const;

private:
			friend class BLocaleRoster;

								BLanguage(const char *language);
			void				Default();

private:
			char*				fStrings[B_NUM_LANGUAGE_STRINGS];
			uint8				fDirection;
			icu_44::Locale*	fICULocale;
};


#endif	// _LANGUAGE_H_
