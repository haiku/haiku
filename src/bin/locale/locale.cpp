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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>


extern const char *__progname;
static const char *kProgramName = __progname;


const char*
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
	printf("%s_%s.UTF-8", conventions.LanguageCode(), conventions.CountryCode());
}


void
usage(int status)
{
	printf("Usage: %s [-lcf]\n"
		"  -l, --language\tPrint the currently set preferred language\n"
		"  -c, --ctype\t\tPrint the LC_CTYPE value based on the preferred language\n"
		"  -f, --format\t\tPrint the formatting convention language\n"
		"  -h, --help\t\tDisplay this help and exit\n",
		kProgramName);

	exit(status);
}


int
main(int argc, char **argv)
{
	static struct option const longopts[] = {
		{"language", no_argument, 0, 'l'},
		{"ctype", no_argument, 0, 'c'},
		{"format", no_argument, 0, 'f'},
		{"help", no_argument, 0, 'h'},
		{NULL}
	};

	int c;
	while ((c = getopt_long(argc, argv, "lcfh", longopts, NULL)) != -1) {
		switch (c) {
			case 'l':
				printf("%s", preferred_language());
				break;
			case 'c':
				printf("%s.UTF-8", preferred_language());
				break;
			case 'f':
				print_formatting_conventions();
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

