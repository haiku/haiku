#ifndef _LANGUAGE_H_
#define _LANGUAGE_H_


#include <SupportDefs.h>
#include <LocaleBuild.h>
#include <LocaleStrings.h>


enum script_direction {
	B_LEFT_TO_RIGHT = 0,
	B_RIGHT_TO_LEFT,
	B_TOP_TO_BOTTOM,	// seems not to be supported anywhere else?
};


class _IMPEXP_LOCALE BLanguage {
	public:
		~BLanguage();

		// language name, e.g. "english", "deutsch"
		const char *Name() const { return fName; }
		// ISO-639 language code, e.g. "en", "de"
		const char *Code() const { return fCode; }
		// ISO-639 language family, e.g. "germanic"
		const char *Family() const { return fFamily; }

		uint8 Direction() const;

		// see definitions below
		const char *GetString(uint32 id) const;

	private:
		friend class BLocaleRoster;

		BLanguage(const char *language);
		void Default();

		char	*fName, *fCode, *fFamily, *fStrings[B_NUM_LANGUAGE_STRINGS];
		uint8	fDirection;
};

#endif	/* _LANGUAGE_H_ */
