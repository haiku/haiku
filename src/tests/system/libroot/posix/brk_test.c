/*
 * Copyright 2015 Simon South, ssouth@simonsouth.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <image.h>
#include <OS.h>


struct sbrk_test {
	char *name;
	char *sbrk_arg_text;
	intptr_t sbrk_arg;
};


static void
output_area_info(void *base_address)
{
	uint8_t *next_area_address;
	area_id next_area;
	area_info next_area_info;

	next_area = area_for(base_address);
	while (next_area != B_ERROR) {
		if (get_area_info(next_area, &next_area_info) == B_OK) {
			next_area_address = (uint8_t *)(next_area_info.address)
				+ next_area_info.size;

			printf("Area ID 0x%x: Addr: %p - %p  Size: 0x%04zx  %s\n",
				next_area_info.area, next_area_info.address,
				next_area_address - 1, next_area_info.size,
				next_area_info.name);

			next_area = area_for(next_area_address);
		} else {
			fprintf(stderr, "PROBLEM: Couldn't get area info");
		}
	}
}


static void
output_data_segment_info(void *address)
{
	puts("Current data segment layout:");
	output_area_info(address);
	puts("");
}


static void
output_sbrk_result(intptr_t sbrk_arg, char *sbrk_arg_text)
{
	printf("\tsbrk(%s) returns %p\n", sbrk_arg_text, sbrk(sbrk_arg));
	if (errno != 0) {
		printf("\tError: %s\n", strerror(errno));
		errno = 0;
	}
}


int
main(int argc, char **argv)
{
	static const uint NUM_TESTS = 7;
	static struct sbrk_test sbrk_tests[] = {
		{ "Get current program break",                     "0",      0 },

		{ "Expand data segment (less than one page)",  "0x7ff",  0x7ff },
		{ "Expand data segment (more than one page)", "0x57ff", 0x57ff },
		{
			"Expand data segment (unreasonable value)",
			"INTPTR_MAX",
			 INTPTR_MAX
		},

		{ "Shrink data segment (less than one page)",  "-0x7ff",  -0x7ff },
		{ "Shrink data segment (more than one page)", "-0x27ff", -0x27ff },
		{
			"Shrink data segment (unreasonable value)",
			"INTPTR_MIN",
			 INTPTR_MIN
		}
	};

	int result = -1;

	bool app_image_found = false;
	image_info i_info;
	int32 image_cookie = 0;

	void *data_segment_address = NULL;

	uint test_index;
	struct sbrk_test *next_sbrk_test;

	/* Find the address of our data segment */
	while (!app_image_found
		&& get_next_image_info(0, &image_cookie, &i_info) == B_OK) {
		if (i_info.type == B_APP_IMAGE) {
			app_image_found = true;

			data_segment_address = i_info.data;
		}
	}

	if (data_segment_address != NULL) {
		/* Run our tests */
		test_index = 0;
		while (test_index < NUM_TESTS) {
			next_sbrk_test = &sbrk_tests[test_index];

			output_data_segment_info(data_segment_address);
			printf("%s:\n", next_sbrk_test->name);
			output_sbrk_result(next_sbrk_test->sbrk_arg,
				next_sbrk_test->sbrk_arg_text);
			printf("\n");

			test_index += 1;
		}

		output_data_segment_info(data_segment_address);

		result = 0;
	} else {
		fprintf(stderr, "PROBLEM: Couldn't locate data segment\n");
	}

	return result;
}
