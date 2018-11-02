/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Ryan Leavengood, leavengood@gmail.com
 */


#include <FormattingConventions.h>
#include <Locale.h>
#include <LocaleRoster.h>
#include <Message.h>
#include <String.h>

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>


extern const char *__progname;
static const char *kProgramName = __progname;


void
print_available_languages()
{
	BMessage languages;
	BLocaleRoster::Default()->GetAvailableLanguages(&languages);
	BString language;
	for (int i = 0; languages.FindString("language", i, &language) == B_OK;
			i++) {
		printf("%s.UTF-8\n", language.String());
	}
	printf("POSIX\n");
}


BString
preferred_language()
{
	BMessage preferredLanguages;
	BLocaleRoster::Default()->GetPreferredLanguages(&preferredLanguages);
	const char* firstPreferredLanguage;
	if (preferredLanguages.FindString("language", &firstPreferredLanguage)
			!= B_OK) {
		// Default to English
		firstPreferredLanguage = "en";
	}

	return firstPreferredLanguage;
}


void
print_formatting_conventions()
{
	BFormattingConventions conventions;
	BLocale::Default()->GetFormattingConventions(&conventions);
	if (conventions.CountryCode() != NULL) {
		printf("%s_%s.UTF-8\n", conventions.LanguageCode(),
			conventions.CountryCode());
	} else {
		printf("%s.UTF-8\n", conventions.LanguageCode());
	}
}


void
print_time_conventions()
{
	BFormattingConventions conventions;
	BLocale::Default()->GetFormattingConventions(&conventions);
	if (conventions.CountryCode() != NULL) {
		printf("%s_%s.UTF-8%s\n", conventions.LanguageCode(),
			conventions.CountryCode(),
			conventions.UseStringsFromPreferredLanguage()
				? "@strings=messages" : "");
	} else {
		printf("%s.UTF-8%s\n", conventions.LanguageCode(),
			conventions.UseStringsFromPreferredLanguage()
				? "@strings=messages" : "");
	}
}


void
usage(int status)
{
	printf("Usage: %s [-alftcm]\n"
		"  -a, --all\t\tPrint all available languages\n"
		"  -l, --language\tPrint the currently set preferred language\n"
		"  -f, --format\t\tPrint the formatting-related locale\n"
		"  -t, --time\t\tPrint the time-related locale\n"
		"  -c, --message\t\tPrint the message-related locale\n"
		"  -m, --charmap\t\tList available character maps\n"
		"  -h, --help\t\tDisplay this help and exit\n",
		kProgramName);

	exit(status);
}


int
main(int argc, char **argv)
{
	static struct option const longopts[] = {
		{"all", no_argument, 0, 'a'},
		{"language", no_argument, 0, 'l'},
		{"format", no_argument, 0, 'f'},
		{"time", no_argument, 0, 't'},
		{"message", no_argument, 0, 'c'},
		{"charmap", no_argument, 0, 'm'},
		{"help", no_argument, 0, 'h'},
		{NULL}
	};

	int c;
	while ((c = getopt_long(argc, argv, "lcfmath", longopts, NULL)) != -1) {
		switch (c) {
			case 'l':
				printf("%s\n", preferred_language().String());
				break;
			case 'f':
				print_formatting_conventions();
				break;
			case 'c':	// for compatibility, we used to use 'c' for ctype
				printf("%s.UTF-8\n", preferred_language().String());
				break;
			case 't':
				print_time_conventions();
				break;

			// POSIX mandatory options
			case 'm':
				puts("UTF-8");
				break;
			case 'a':
				print_available_languages();
				break;
			// TODO 'c', 'k'

			case 'h':
				usage(0);
				break;
			case 0:
				break;
			default:
				usage(1);
				break;
		}
	}

	return 0;
}
