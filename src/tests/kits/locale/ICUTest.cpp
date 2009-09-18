/*
** Copyright 2009 Adrien Destugues, pulkomandy@gmail.com.
** Distributed under the terms of the MIT License.
*/


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <typeinfo>
#include <unistd.h>

#include <Application.h>
#include <StopWatch.h>

#include <unicode/locid.h>
#include <unicode/strenum.h>


void TestLocale(const Locale& loc)
{
	printf("-- init\n");
	assert(!loc.isBogus());
	printf("-- basic info\n");
	printf("Default locale:\nLanguage: %s\nScript: %s\nVariant: %s\n"
		"Country: %s\nName: %s\nBaseName: %s\n", loc.getLanguage(), 
		loc.getScript(), loc.getVariant(), 
		loc.getCountry(), loc.getName(), 
		loc.getBaseName());

	UErrorCode err = U_ZERO_ERROR;
	printf("-- keywords\n");
	StringEnumeration* keywords = loc.createKeywords(err);
	if (err != U_ZERO_ERROR)
		printf("FAILED: getting keywords list\n");
	if (keywords == NULL)
		printf("FAILED: getting keywords list returned NULL\n");
	else {
		printf("Keywords: %d available\n",keywords->count(err));
		assert(err == U_ZERO_ERROR);

		char keyvalue[256];
		while (const char* keyname = keywords->next(NULL,err)) {
			loc.getKeywordValue(keyname,keyvalue,256,err);
			printf("%s > %s\n",keyname,keyvalue);	
		}

		delete keywords;
	}
}


int
main(int argc, char **argv)
{
	printf("--------\niDefault Locale\n--------\n");
	Locale defaultLocale;
	TestLocale(defaultLocale);
	printf("--------\nFrench Locale\n--------\n");
	Locale french = Locale::getFrench();
	TestLocale(french);
	printf("--------\nCustom Locale\n--------\n");
	Locale custom("es");
	TestLocale(custom);

	printf("--------\nLocale listing\n--------\n");
	int32_t count;
	const Locale* localeList = Locale::getAvailableLocales(count);
	printf("%d locales found\n",count);

	for (int i=0; i<count; i++) {
		printf("Locale number %d\n",i);
		TestLocale(localeList[i]);
	}

	printf("--------\nLocale country codes\n--------\n");
	const char* const* countryTable = Locale::getISOCountries();

	for (int i=0; countryTable[i] != NULL; i++)
		printf("%s\t",countryTable[i]);

	printf("\n--------\nNumberFormat\n--------\n");
	printf("--------\nDateFormat\n--------\n");
	printf("--------\nCollator\n--------\n");

	return 0;
}
