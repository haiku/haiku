/*
 * Copyright 2005, Andrew Bachmann, andrewbachmann@myrealbox.com
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <image.h>
#include <cassert>
#include <cstdio>

static double (*be_sin)(double);
static double (*be_cos)(double);

static image_id
get_libroot_id()
{
	image_info info;
	int32 cookie = 0;
	while (get_next_image_info(0, &cookie, &info) == B_OK) {
		if (strcmp(info.name, "/boot/beos/system/lib/libroot.so") == 0) {
			return info.id;
		}
	}
	return B_BAD_VALUE;
}


int 
main(int argc, char **argv)
{
	image_id libroot = get_libroot_id();
	assert(get_image_symbol(libroot, "sin", B_SYMBOL_TYPE_TEXT, (void**)&be_sin) == B_OK);
	assert(get_image_symbol(libroot, "cos", B_SYMBOL_TYPE_TEXT, (void**)&be_cos) == B_OK);

	fprintf(stdout, "value\t\tsin(value)\tbe_sin(value)\tcos(value)\tbe_cos(value)\n");
	status_t result = B_OK;
	for (int i = -10 ; i < 10 ; i++) {
		double f = (double)i;
		fprintf(stdout, "%0.10f\t%0.10f\t%0.10f\t%0.10f\t%0.10f\n",
		                 f, sin(f), be_sin(f), cos(f), be_cos(f));
		if ((sin(f) != be_sin(f)) || (cos(f) != be_cos(f))) {
			result = B_ERROR;
		}
	}
	return result;
}
