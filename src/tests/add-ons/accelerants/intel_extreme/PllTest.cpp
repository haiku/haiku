/*
 * Copyright 2006-2018, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */

#include "accelerant_protos.h"
#include "accelerant.h"
#include "intel_extreme.h"
#include "pll.h"

#include <stdio.h>
#include <stdlib.h>

struct accelerant_info* gInfo;

const struct  test_device {
	uint32 type;
	const char* name;
} kTestDevices[] = {
	{INTEL_MODEL_915, "915"},
	{INTEL_MODEL_945, "945"},
	{INTEL_MODEL_965, "965"},
	{INTEL_MODEL_G33, "G33"},
	{INTEL_MODEL_G45, "G45"},
	{INTEL_MODEL_PINE, "PineView"},
	{INTEL_MODEL_ILKG, "IronLake"},
	{INTEL_MODEL_SNBG, "SandyBridge"},
	{INTEL_MODEL_IVBG, "IvyBridge"}
};


// This is a function from app_server which the accelerant calls, we need an
// implementation so pll.cpp can link.
extern "C" void spin(long long int) {
}


static void
simulate_mode(display_mode* mode, int j)
{
	mode->timing.flags = 0;
	if (j == 0)
		mode->timing.pixel_clock = uint32(75.2 * 1000);
	else
		mode->timing.pixel_clock = uint32(146.25 * 1000);
	mode->timing.h_display = 1366;
	mode->timing.h_sync_start = 1414;
	mode->timing.h_sync_end = 1478;
	mode->timing.h_total = 1582;

	mode->timing.v_display = 768;
	mode->timing.v_sync_start = 772;
	mode->timing.v_sync_end = 779;
	mode->timing.v_total = 792;

	mode->virtual_width = 1366;
	mode->virtual_height = 768;
}


int
main(void)
{
	// First we simulate our global card info structs
	gInfo = (accelerant_info*)malloc(sizeof(accelerant_info));
	if (gInfo == NULL) {
		fprintf(stderr, "Unable to malloc artificial gInfo!\n");
		return 1;
	}

	gInfo->shared_info = (intel_shared_info*)malloc(sizeof(intel_shared_info));
	if (gInfo->shared_info == NULL) {
		fprintf(stderr, "Unable to malloc shared_info!\n");
		free(gInfo);
		return 2;
	}

	for (uint32 index = 0; index < (sizeof(kTestDevices) / sizeof(test_device));
		index++) {
		gInfo->shared_info->device_type = kTestDevices[index].type;
		fprintf(stderr, "=== %s (Generation %d)\n",  kTestDevices[index].name,
			gInfo->shared_info->device_type.Generation());

		if (gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_SER5)) {
			printf("Is series 5\n");
			gInfo->shared_info->pll_info.reference_frequency = 120000;
			gInfo->shared_info->pll_info.max_frequency = 350000;
			gInfo->shared_info->pll_info.min_frequency = 20000;
			gInfo->shared_info->pch_info = INTEL_PCH_CPT;
		} else if (gInfo->shared_info->device_type.InFamily(INTEL_FAMILY_9xx)) {
			printf("Is 9xx\n");
			gInfo->shared_info->pll_info.reference_frequency = 96000;
			gInfo->shared_info->pll_info.max_frequency = 400000;
			gInfo->shared_info->pll_info.min_frequency = 20000;
		} else {
			printf("Is other\n");
			gInfo->shared_info->pll_info.reference_frequency = 96000;
			gInfo->shared_info->pll_info.max_frequency = 400000;
			gInfo->shared_info->pll_info.min_frequency = 20000;
		}

		pll_divisors output;
		for (uint32 j = 0; j < 2; j++) {
			display_mode fakeMode;
			simulate_mode(&fakeMode, j);
			compute_pll_divisors(&fakeMode, &output, false);

			printf("%d -> %d\n", fakeMode.timing.pixel_clock,
				gInfo->shared_info->pll_info.reference_frequency * output.m
					/ (output.n * output.p));
		}
	}

	free(gInfo->shared_info);
	free(gInfo);
	return 0;
}
