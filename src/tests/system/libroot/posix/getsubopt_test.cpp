/*
 * Copyright 2010, Oliver Tappe <zooey@hirschkaefer.de>.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int
main()
{
	char* const keys[] = {
		"mode", "be-lenient", NULL
	};

	char* option = strdup("mode=readwrite,unknown,be-lenient");
	char* value = NULL;
	int result = getsubopt(&option, keys, &value);
	if (result != 0)
		fprintf(stderr, "failed 1: result=%d, expected %d\n", result, 0);
	if (value == NULL || strcmp(value, "readwrite") != 0)
		fprintf(stderr, "failed 2: value=%s, expected 'readwrite'\n", value);
	result = getsubopt(&option, keys, &value);
	if (result != -1)
		fprintf(stderr, "failed 3: result=%d, expected %d\n", result, -1);
	if (value != NULL)
		fprintf(stderr, "failed 4: value=%p, expected NULL\n", value);
	result = getsubopt(&option, keys, &value);
	if (result != 1)
		fprintf(stderr, "failed 5: result=%d, expected %d\n", result, 1);
	if (value != NULL)
		fprintf(stderr, "failed 6: value=%p, expected NULL\n", value);

	return 0;
}
