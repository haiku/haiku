
#include <Collator.h>
#include <Language.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <String.h>
#include <UnicodeChar.h>

#include <stdio.h>


void
unicode_char_to_string(uint32 c, char *text)
{
	BUnicodeChar::ToUTF8(c, &text);
	text[0] = '\0';
}


int
main()
{
	// Test BUnicodeChar class

	char text[16];

	for (int32 i = 30; i < 70; i++) {
		unicode_char_to_string(i, text);
		printf("%s: alpha == %d, alNum == %d, lower == %d, upper == %d, "
			"defined == %d, charType == %d\n", text,
			BUnicodeChar::IsAlpha(i), BUnicodeChar::IsAlNum(i),
			BUnicodeChar::IsLower(i), BUnicodeChar::IsUpper(i),
			BUnicodeChar::IsDefined(i), BUnicodeChar::Type(i));
	}

	uint32 chars[] = {(uint8)'\xe4', (uint8)'\xd6', (uint8)'\xdf',
		(uint8)'\xe8', (uint8)'\xe1', (uint8)'\xe9', 0};
	for (int32 j = 0, i; (i = chars[j]) != 0; j++) {
		unicode_char_to_string(i, text);
		printf("%s: alpha == %d, alNum == %d, lower == %d, upper == %d, "
			"defined == %d, charType == %d\n", text,
			BUnicodeChar::IsAlpha(i), BUnicodeChar::IsAlNum(i),
			BUnicodeChar::IsLower(i), BUnicodeChar::IsUpper(i),
			BUnicodeChar::IsDefined(i), BUnicodeChar::Type(i));

		unicode_char_to_string(BUnicodeChar::ToUpper(i), text);
		printf("toUpper == %s, ", text);
		unicode_char_to_string(BUnicodeChar::ToLower(i), text);
		printf("toLower == %s\n", text);
	}

	const char *utf8chars[] = {"à", "ß", "ñ", "é", "ç", "ä", NULL};
	for (int32 j = 0; utf8chars[j] != 0; j++) {
		int32 i = BUnicodeChar::FromUTF8(utf8chars[j]);
		unicode_char_to_string(i, text);
		printf("%s: alpha == %d, alNum == %d, lower == %d, upper == %d, "
			"defined == %d, charType == %d\n", text,
			BUnicodeChar::IsAlpha(i), BUnicodeChar::IsAlNum(i),
			BUnicodeChar::IsLower(i), BUnicodeChar::IsUpper(i),
			BUnicodeChar::IsDefined(i), BUnicodeChar::Type(i));

		unicode_char_to_string(BUnicodeChar::ToUpper(i), text);
		printf("toUpper == %s, ", text);
		unicode_char_to_string(BUnicodeChar::ToLower(i), text);
		printf("toLower == %s\n", text);
	}
	printf("%c: digitValue == %ld\n", '8', BUnicodeChar::DigitValue('8'));

	// Test BCollator class

	BCollator collator;
	BLocaleRoster::Default()->GetDefaultLocale()->GetCollator(&collator);
	const char *strings[] = {"gehen", "géhen", "aus", "äUß", "auss", "äUß",
		"WO", "wÖ", "SO", "so", "açñ", "acn", NULL};
	const char *strengths[] = {"primary:  ", "secondary:", "tertiary: "};
	for (int32 i = 0; strings[i]; i += 2) {
		for (int32 strength = B_COLLATE_PRIMARY; strength < 4; strength++) {
			BString a, b;
			collator.GetSortKey(strings[i], &a, strength);
			collator.GetSortKey(strings[i + 1], &b, strength);

			printf("%s sort keys: \"%s\" -> \"%s\", \"%s\" -> \"%s\"\n",
				strengths[strength-1], strings[i], a.String(), strings[i+1],
				b.String());
			printf("\tcmp = %d (key compare = %d)\n",
				collator.Compare(strings[i], strings[i + 1], strength),
				strcmp(a.String(), b.String()));
		}
		putchar('\n');
	}

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

