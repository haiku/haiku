/*
 * Copyright 2007, François Revol, revol@free.fr.
 *
 * Distributed under the terms of the MIT license.
 */


#include <stdio.h>
#include <Application.h>
#include <InterfaceDefs.h>
#include <String.h>

// this isn't public yet ?
namespace BPrivate {
int32 count_decorators(void);
int32 get_decorator(void);
status_t get_decorator_name(const int32 &index, BString &name);
status_t get_decorator_preview(const int32 &index, BBitmap *bitmap);
status_t set_decorator(const int32 &index);
}

using namespace BPrivate;

int main(int argc, char **argv)
{
	status_t err;
	if (argc < 2) {
		printf("usage: %s [-l|-c|decorname]\n", argv[0]);
		printf("\t-l: list available decors\n");
		printf("\t-c: give current decor name\n");
		return 1;
	}
	BApplication app("application/x-vnd.Haiku-setdecor");
	// we want the list
	if (!strcmp(argv[1], "-l")) {
		int32 i, count;
		count = count_decorators();
		if (count < 0) {
			fprintf(stderr, "error counting decorators: %s\n", strerror(count));
			return 1;
		}
		for (i = 0; i < count; i++) {
			BString name;
			err = get_decorator_name(i, name);
			if (err < 0)
				continue;
			printf("%s\n", name.String());
		}
		return 0;
	}
	// we want the current one
	if (!strcmp(argv[1], "-c")) {
		int32 i;
		BString name;
		i = get_decorator();
		if (i < 0) {
			fprintf(stderr, "error getting current decorator: %s\n", strerror(i));
			return 1;
		}
		err = get_decorator_name(i, name);
		if (err < 0) {
			fprintf(stderr, "error getting name of decorator: %s\n", strerror(err));
			return 1;
		}
		printf("%s\n", name.String());
	}
	// we want to change it
	int32 i, count;
	count = count_decorators();
	if (count < 0) {
		fprintf(stderr, "error counting decorators: %s\n", strerror(count));
		return 1;
	}
	for (i = 0; i < count; i++) {
		BString name;
		err = get_decorator_name(i, name);
		if (err < 0)
			continue;
		if (name == argv[1]) {
			err = set_decorator(i);
			if (err < 0) {
				fprintf(stderr, "error setting decorator: %s\n", strerror(err));
				return 1;
			}
			return 0;
		}
	}
	fprintf(stderr, "can't find decorator \"%s\"\n", argv[1]);
	return 1;
}

