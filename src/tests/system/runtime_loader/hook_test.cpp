/*
 * Copyright 2026, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */

#include <runtime_loader.h>

#include <stdio.h>
#include <string.h>


static image_id
get_libroot_id()
{
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		if (strcmp(info.name, "/boot/system/lib/libroot.so") == 0) {
			return info.id;
		}
	}
	return B_BAD_VALUE;
}


int
main(int argc, char** argv)
{
	image_id id = get_libroot_id();
	for (int32 num = 0; ; num++) {
		char symbolName[256];
		int32 nameLength = sizeof(symbolName);
		int32 symbolType;
		void* location;
		status_t status = __gRuntimeLoader->get_nth_image_symbol(id, num,
			symbolName, &nameLength, &symbolType, &location);
		if (status != B_OK)
			break;
		printf("symbol %d %s\n", num, symbolName);
	}

	image_id image;
	char* imagePath;
	char* imageName;
	char* symbolName;
	void* location;
	bool exactMatch;
	status_t status = __gRuntimeLoader->get_nearest_symbol_at_address(__builtin_return_address(0), &image,
		&imagePath, &imageName, &symbolName, NULL, &location, &exactMatch);
	if (status != B_OK)
		return -1;
	printf("get_nearest_symbol_at_address %d %s %s %s %p %d\n",
		image, imagePath, imageName, symbolName, location, exactMatch);
	return 0;
}
