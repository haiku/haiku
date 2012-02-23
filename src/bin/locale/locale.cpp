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
	printf("%s_%s.UTF-8\n", conventions.LanguageCode(),
		conventions.CountryCode());
}


void
print_time_conventions()
{
	BFormattingConventions conventions;
	BLocale::Default()->GetFormattingConventions(&conventions);
	if (conventions.UseStringsFromPreferredLanguage()) {
		printf("%s_%s.UTF-8@strings=messages\n", conventions.LanguageCode(),
			conventions.CountryCode());
	} else {
		printf("%s_%s.UTF-8\n", conventions.LanguageCode(),
			conventions.CountryCode());
	}
}


void
usage(int status)
{
	printf("Usage: %s [-lfmt]\n"
		"  -l, --language\tPrint the currently set preferred language\n"
		"  -f, --format\t\tPrint the formatting-related locale\n"
		"  -m, --message\t\tPrint the message-related locale\n"
		"  -t, --time\t\tPrint the time-related locale\n"
		"  -h, --help\t\tDisplay this help and exit\n",
		kProgramName);

	exit(status);
}


int
main(int argc, char **argv)
{
	static struct option const longopts[] = {
		{"language", no_argument, 0, 'l'},
		{"format", no_argument, 0, 'f'},
		{"message", no_argument, 0, 'm'},
		{"time", no_argument, 0, 't'},
		{"help", no_argument, 0, 'h'},
		{NULL}
	};

	int c;
	while ((c = getopt_long(argc, argv, "lcfmth", longopts, NULL)) != -1) {
		switch (c) {
			case 'l':
				printf("%s\n", preferred_language().String());
				break;
			case 'f':
				print_formatting_conventions();
				break;
			case 'c':	// for compatibility, we used to use 'c' for ctype
			case 'm':
				printf("%s.UTF-8\n", preferred_language().String());
				break;
			case 't':
				print_time_conventions();
				break;
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
