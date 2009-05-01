/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Language.h>

#include <Path.h>
#include <FindDirectory.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


static char *gBuiltInStrings[] = {
	"Yesterday",
	"Today",
	"Tomorrow",
	"Future",
	
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday",
	
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat",
	
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December",
	
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec",
	
	"^[yY]",
	"^[nN]",
	"yes",
	"no"
};


static char *
TrimCopy(char *string)
{
	while (isspace(*string))
		string++;

	int32 length = strlen(string);
	while (length-- > 0 && isspace(string[length]))
		string[length] = '\0';

	if (length < 0)
		return NULL;

	return strdup(string);
}


//	#pragma mark -


BLanguage::BLanguage(const char *language)
	:
	fName(NULL),
	fCode(NULL),
	fFamily(NULL),
	fDirection(B_LEFT_TO_RIGHT)
{
	for (int32 i = B_NUM_LANGUAGE_STRINGS;i-- > 0;)
		fStrings[i] = NULL;

	if (language == NULL) {
		Default();
		return;
	}

	char name[B_FILE_NAME_LENGTH];
	sprintf(name, "locale/languages/%s.language", language);

	BPath path;
	if (find_directory(B_BEOS_ETC_DIRECTORY, &path) < B_OK) {
		Default();
		return;
	}

	path.Append(name);

	FILE *file = fopen(path.Path(), "r");
	if (file == NULL) {
		Default();
		return;
	}

	int32 count = -2;
	while (fgets(name, B_FILE_NAME_LENGTH, file) != NULL) {
		if (count == -2) {
			char *family = strchr(name, ',');
			if (family == NULL)
				break;
			*family++ = '\0';

			// set the direction of writing			
			char *direction = strchr(family, ',');
			if (direction != NULL) {
				*direction++ = '\0';
				direction = TrimCopy(direction);

				if (!strcasecmp(direction, "ltr"))
					fDirection = B_LEFT_TO_RIGHT;
				else if (!strcasecmp(direction, "rtl"))
					fDirection = B_RIGHT_TO_LEFT;
				else if (!strcasecmp(direction, "ttb"))
					fDirection = B_TOP_TO_BOTTOM;

				free(direction);
			}

			fName = strdup(language);
			fCode = TrimCopy(name);
			fFamily = TrimCopy(family);
			count++;
		} else if (count == -1) {
			if (!strcmp(name,"--\n"))
				count++;
		} else if (count < B_NUM_LANGUAGE_STRINGS) {
			char *string = TrimCopy(name);
			if (string == NULL)
				continue;

			fStrings[count++] = string; 
		}
	}

	if (count < 0)
		Default();

	fclose(file);
}


BLanguage::~BLanguage()
{
	if (fName != NULL) 
		free(fName);
	if (fCode != NULL)
		free(fCode);
	if (fFamily != NULL)
		free(fFamily);

	for (int32 i = B_NUM_LANGUAGE_STRINGS;i-- > 0;)
		free(fStrings[i]);
}


void
BLanguage::Default()
{
	fName = strdup("english");
	fCode = strdup("en");
	fFamily = strdup("germanic");
	fDirection = B_LEFT_TO_RIGHT;

	for (int32 i = B_NUM_LANGUAGE_STRINGS;i-- > 0;) {
		free(fStrings[i]);
		fStrings[i] = strdup(gBuiltInStrings[i]);
	}
}


uint8 
BLanguage::Direction() const
{
	return fDirection;
}


const char *
BLanguage::GetString(uint32 id) const
{
	if (id < B_LANGUAGE_STRINGS_BASE || id > B_LANGUAGE_STRINGS_BASE + B_NUM_LANGUAGE_STRINGS)
		return NULL;

	return fStrings[id - B_LANGUAGE_STRINGS_BASE];
}

