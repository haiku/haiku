/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <Collator.h>
#include <Locale.h>
#include <StopWatch.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


const char *kStrings[] = {
	"a-b-c",
	"a b c",
	"A.b.c",
	"ä,b,c",
	"abc",
	"gehen",
	"géhen",
	"aus",
	"äUß",
	"auss",
	"WO",
	"wÖ",
	"SO",
	"so",
	"açñ",
	"acn",
	"pêche",
	"pêché",
	"peché",
	"peche",
	"pecher",
	"eñe",
	"ene",
	"nz",
	"ña",
	"llamar",
	"luz",
};
const uint32 kNumStrings = sizeof(kStrings) / sizeof(kStrings[0]);
const uint32 kIterations = 50000;


void
test(BCollator *collator, const char *name, int8 strength)
{
	collator->SetDefaultStrength(strength);

	BStopWatch watch(name, true);

	for (uint32 j = 0; j < kIterations; j++) {
		for (uint32 i = 0; i < kNumStrings; i++) {
			BString key;
			collator->GetSortKey(kStrings[i], &key);
		}
	}

	watch.Suspend();
	double secs = watch.ElapsedTime() / 1000000.0;
	printf("\t%s%9Ld usecs, %6.3g secs,%9lu keys/s\n",
		name, watch.ElapsedTime(), secs, uint32(kIterations * kNumStrings / secs));
}


void
usage()
{
	fprintf(stderr,
		"usage: collatorSpeed [-i] [<add-on path>]\n"
		"  -i\tignore punctuation (defaults to: punctuation matters)\n");
	exit(-1);
}


int
main(int argc, char **argv)
{
	// Parse command line arguments

	bool ignorePunctuation = false;
	char *addon = NULL;
	BCollator *collator = NULL;

	while ((++argv)[0]) {
		if (argv[0][0] == '-') {
			if (!strcmp(argv[0], "-i"))
				ignorePunctuation = true;
			else if (!strcmp(argv[0], "--help"))
				usage();
		} else {
			// this will be the add-on to be loaded
			addon = argv[0];
		}
	}

	// load the collator add-on if necessary

	if (addon != NULL) {
		image_id image = load_add_on(addon);
		if (image < B_OK)
			fprintf(stderr, "could not load add-on at \"%s\": %s.\n", addon, strerror(image));

		BCollatorAddOn *(*instantiate)(void);
		if (get_image_symbol(image, "instantiate_collator",
				B_SYMBOL_TYPE_TEXT, (void **)&instantiate) == B_OK) {
			BCollatorAddOn *collatorAddOn = instantiate();
			if (collatorAddOn != NULL)
				collator = new BCollator(collatorAddOn, B_COLLATE_PRIMARY, true);
		} else if (image >= B_OK) {
			fprintf(stderr, "could not find instantiate_collator() function in add-on!\n");
			unload_add_on(image);
		}
	}

	if (collator == NULL) {
		collator = be_locale->Collator();
		addon = (char*)"default";
	}

	// test key creation speed

	collator->SetIgnorePunctuation(ignorePunctuation);

	printf("%s:\n", addon);
	test(collator, "primary:   ", B_COLLATE_PRIMARY);
	test(collator, "secondary: ", B_COLLATE_SECONDARY);
	test(collator, "tertiary:  ", B_COLLATE_TERTIARY);
	test(collator, "quaternary:", B_COLLATE_QUATERNARY);
	test(collator, "identical: ", B_COLLATE_IDENTICAL);

	return 0;
}

