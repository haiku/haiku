/*
 * Copyright 2005, Andrew Bachmann, andrewbachmann@myrealbox.com
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <image.h>
#include <cmath>
#include <cassert>
#include <cstdio>

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


static double (*be_sin)(double);
static double (*be_cos)(double);
static double (*be_copysign)(double, double);
static double (*be_drem)(double, double);


int 
main(int argc, char **argv)
{
	image_id libroot = get_libroot_id();
	assert(get_image_symbol(libroot, "sin", B_SYMBOL_TYPE_TEXT, (void**)&be_sin) == B_OK);
	assert(get_image_symbol(libroot, "cos", B_SYMBOL_TYPE_TEXT, (void**)&be_cos) == B_OK);
	assert(get_image_symbol(libroot, "copysign", B_SYMBOL_TYPE_TEXT, (void**)&be_copysign) == B_OK);
	assert(get_image_symbol(libroot, "drem", B_SYMBOL_TYPE_TEXT, (void**)&be_drem) == B_OK);

	status_t result = B_OK;

	// test sin
	fprintf(stdout, "value\tsin(value)\tbe_sin(value)\n");
	for (int i = -9 ; i <= 9 ; i++) {
		double f = (double)i;
		fprintf(stdout, "%0.1f\t%0.10f\t%0.10f\n", f, sin(f), be_sin(f));
		if (sin(f) != be_sin(f)) {
			result = B_ERROR;
		}
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "value\tcos(value)\tbe_cos(value)\n");
	// test cos
	for (int i = -9 ; i <= 9 ; i++) {
		double f = (double)i;
		fprintf(stdout, "%0.1f\t%0.10f\t%0.10f\n", f, cos(f), be_cos(f));
		if (cos(f) != be_cos(f)) {
			result = B_ERROR;
		}
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "arg1\targ2\tcopysign()\tbe_copysign()\n");
	// test copysign
	for (int i = -2 ; i <= 2 ; i++) {
		for (int j = -2 ; j <= 2 ; j++) {
			double f = (double)i;
			double g = (double)j;
			fprintf(stdout, "%0.1f\t%0.1f\t%0.10f\t%0.10f\n",
			                 f, g, copysign(f, g), be_copysign(f, g));
			if (copysign(f, g) != be_copysign(f, g)) {
				result = B_ERROR;
			}
		}
	}
	fprintf(stdout, "\n");

	return result;
}
