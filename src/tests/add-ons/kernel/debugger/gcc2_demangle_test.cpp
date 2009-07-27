/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#define kprintf printf

#include "gcc2.cpp"


int
main(int argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		bool isObjectMethod;
		char name[64];
		const char* symbol = demangle_symbol_gcc2(argv[i], name, sizeof(name),
			&isObjectMethod);
		if (symbol == NULL) {
			printf("%s: cannot be parsed\n", argv[i]);
			continue;
		}

		printf("%s (%s method)\n", symbol, isObjectMethod ? "object" : "class");

		uint32 cookie = 0;
		int32 type;
		size_t length;
		while (get_next_argument_gcc2(&cookie, argv[i], name, sizeof(name),
				&type, &length) == B_OK) {
			printf("name \"%s\", type %.4s, length %lu\n", name, (char*)&type,
				length);
		}
	}

	return 0;
}
