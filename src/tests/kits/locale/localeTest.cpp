
#include <Collator.h>
#include <Language.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <String.h>
#include <UnicodeChar.h>

#include <stdio.h>


int
main()
{
	// Tests the BLanguage class

	BLanguage language;
	BLocaleRoster::Default()->GetDefaultLocale()->GetLanguage(&language);
	BString name;
	language.GetName(name);
	printf("Language name = \"%s\", code = \"%s\"\n", name.String(),
		language.Code());
	printf("\tdirection = %s\n",
		language.Direction() == B_LEFT_TO_RIGHT ? "left-to-right" :
		language.Direction() == B_RIGHT_TO_LEFT ? "right-to-left" :
		"top-to-bottom");

	//for (int32 i = 0; i < 200; i++) {
	//	const char *string = language.GetString(i);
	//	if (string != NULL)
	//		printf("%ld: %s\n", i, string);
	//}

	printf("First month = %s (%s)\n",
		language.GetString(B_MON_1),
		language.GetString(B_AB_MON_1));
}

