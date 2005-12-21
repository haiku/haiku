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

	// test cos2 + sin2 = 1
	fprintf(stdout, "value\tsin2()+cos2()\n");
	for (int i = -63 ; i <= 63 ; i++) {
		double f = (double)i/7.0;
		double x = sin(f);
		double y = cos(f);
		if (fabs(x*x + y*y - 1.0) > 0.000001) {
			fprintf(stdout, "%0.3f\t%0.10f", f, x*x + y*y);
			fprintf(stdout, " **");
			fprintf(stdout, "\n");
			result = B_ERROR;
		}
	}
	fprintf(stdout, "\n");

	// test sin
	fprintf(stdout, "value\tsin(value)\tbe_sin(value)\n");
	for (int i = -63 ; i <= 63 ; i++) {
		double f = (double)i/7.0;
		double x = sin(f);
		double y = be_sin(f);
		if (fabs(x - y) > 0.000001) {
			fprintf(stdout, "%0.3f\t%0.10f\t%0.10f", f, x, y);
			fprintf(stdout, " **");
			fprintf(stdout, "\n");
			result = B_ERROR;
		}
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "value\tcos(value)\tbe_cos(value)\n");
	// test cos
	for (int i = -63 ; i <= 63 ; i++) {
		double f = (double)i/7.0;
		double x = cos(f);
		double y = be_cos(f);
		if (fabs(x - y) > 0.000001) {
			fprintf(stdout, "%0.3f\t%0.10f\t%0.10f", f, x, y);
			fprintf(stdout, " **");
			fprintf(stdout, "\n");
			result = B_ERROR;
		}
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "arg1\targ2\tcopysign()\tbe_copysign()\n");
	// test copysign
	for (int i = -21 ; i <= 21 ; i++) {
		for (int j = -36 ; j <= 36 ; j++) {
			double f = (double)i/3.0;
			double g = (double)j/4.0;
			double x = copysign(f, g);
			double y = be_copysign(f, g);
			if ((x != y) && !(isnan(x) && isnan(y))) {
				fprintf(stdout, "%0.1f\t%0.1f\t%0.6f\t%0.6f", f, g, x, y);
				fprintf(stdout, " **");
				fprintf(stdout, "\n");
				result = B_ERROR;
			}
		}
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "arg1\targ2\tdrem(values)\tbe_drem(values)\n");
	// test drem
	for (int i = -21 ; i <= 21 ; i++) {
		for (int j = -36 ; j <= 36 ; j++) {
			double f = (double)i/3.0;
			double g = (double)j/4.0;
			double x = drem(f, g);
			double y = be_drem(f, g);
			if ((x != y) && !(isnan(x) && isnan(y))) {
				fprintf(stdout, "%0.2f\t%0.2f\t%0.10f\t%0.10f", f, g, x, y);
				fprintf(stdout, " **");
				fprintf(stdout, "\n");
				result = B_ERROR;
			}
		}
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "result = %s\n", (result == B_OK ? "OK" : "ERROR"));
	return result;
}
