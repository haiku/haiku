/*
 * Copyright 2010, Oliver Tappe <zooey@hirschkaefer.de>.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <string.h>


int
getsubopt(char** optionPtr, char* const* keys, char** valuePtr)
{
	if (optionPtr == NULL || valuePtr == NULL)
		return -1;

	char* option = *optionPtr;
	if (*option == '\0')
		return -1;

	char* startOfNextOption = strchr(option, ',');
	if (startOfNextOption != NULL)
		*startOfNextOption++ = '\0';
	else
		startOfNextOption = option + strlen(option);
	*optionPtr = startOfNextOption;

	char* startOfValue = strchr(option, '=');
	if (startOfValue != NULL)
		*startOfValue++ = '\0';
	*valuePtr = startOfValue;

	for (int keyIndex = 0; keys[keyIndex] != NULL; ++keyIndex) {
		if (strcmp(option, keys[keyIndex]) == 0)
			return keyIndex;
	}

	return -1;
}
