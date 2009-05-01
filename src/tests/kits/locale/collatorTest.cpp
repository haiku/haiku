/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <Collator.h>
#include <Locale.h>
#include <Message.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


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
	"PECHE",
	"PÊCHE",
	"pecher",
	"eñe",
	"ene",
	"nz",
	"ña",
	"llamar",
	"luz",
};
const uint32 kNumStrings = sizeof(kStrings) / sizeof(kStrings[0]);

BCollator *gCollator;

bool gTestKeyGeneration = true;
bool gShowKeys = false;


int
compareStrings(const void *_a, const void *_b)
{
	const char *a = *(const char **)_a;
	const char *b = *(const char **)_b;

	return gCollator->Compare(a, b);
}


void
printArray(const char *label, const char **strings, size_t size)
{
	puts(label);

	uint32 bucket = 1;
	for (uint32 i = 0; i < size; i++) {
		if (i > 0) {
			int compare = gCollator->Compare(strings[i], strings[i - 1]);
			if (compare > 0)
				printf("\n%2lu)", bucket++);
			else if (compare < 0) {
				printf("\t*** broken sort order!\n");
				exit(-1);
			}

			// Test sort key generation

			if (gTestKeyGeneration) {
				BString a, b;
				gCollator->GetSortKey(strings[i - 1], &a);
				gCollator->GetSortKey(strings[i], &b);

				int keyCompare = strcmp(a.String(), b.String());
				if (keyCompare > 0 || (keyCompare == 0 && compare != 0))
					printf(" (*** \"%s\" wrong keys \"%s\" ***) ", a.String(), b.String());
			}
		} else
			printf("%2lu)", bucket++);

		printf("\t%s", strings[i]);
		
		if (gShowKeys) {
			BString key;
			gCollator->GetSortKey(strings[i], &key);
			printf(" (%s)", key.String());
		}
	}
	putchar('\n');
}


void
usage()
{
	fprintf(stderr,
		"usage: collatorTest [-ick] [<add-on path>]\n"
		"  -i\tignore punctuation (defaults to: punctuation matters)\n"
		"  -c\tcomparison only, no sort key test\n"
		"  -k\tshows the sort keys along the strings (sort keys doesn't have to be visually correct)\n");
	exit(-1);
}


int
main(int argc, char **argv)
{
	// Parse command line arguments

	int strength = B_COLLATE_SECONDARY;
	bool ignorePunctuation = false;
	char *addon = NULL;

	while ((++argv)[0]) {
		if (argv[0][0] == '-' && argv[0][1] != '-') {
			char *arg = argv[0] + 1;
			char c;
			while ((c = *arg++)) {
				if (c == 'i')
					ignorePunctuation = true;
				else if (c == 'c')
					gTestKeyGeneration = false;
				else if (c == 'k')
					gShowKeys = true;
				else if (isdigit(c)) {
					int strength = c - '0';
					if (strength < B_COLLATE_PRIMARY)
						strength = B_COLLATE_PRIMARY;
					else if (strength > B_COLLATE_IDENTICAL)
						strength = B_COLLATE_IDENTICAL;
				}
			}
		} else if (!strcmp(argv[0], "--help")) {
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
			BCollatorAddOn *collator = instantiate();
			if (collator != NULL)
				gCollator = new BCollator(collator, strength, true);
		} else if (image >= B_OK) {
			fprintf(stderr, "could not find instantiate_collator() function in add-on!\n");
			unload_add_on(image);
		}
	}

	if (gCollator == NULL) {
		printf("--------- Use standard collator! -----------\n");
		gCollator = be_locale->Collator();
	}

	// test archiving/unarchiving collator

	BMessage archive;
	if (gCollator->Archive(&archive, true) != B_OK)
		fprintf(stderr, "Archiving failed!\n");
	else {
		BArchivable *unarchived = instantiate_object(&archive);
		gCollator = dynamic_cast<BCollator *>(unarchived);
		if (gCollator == NULL) {
			fprintf(stderr, "Unarchiving failed!\n");

			delete unarchived;
			gCollator = be_locale->Collator();
		}
	}

	// test the BCollator::Compare() and GetSortKey() methods

	const char *strengthLabels[] = {"primary:  ", "secondary:", "tertiary: "};
	uint32 strengths[] = {B_COLLATE_PRIMARY, B_COLLATE_SECONDARY, B_COLLATE_TERTIARY};

	gCollator->SetIgnorePunctuation(ignorePunctuation);

	for (uint32 i = 0; i < sizeof(strengths) / sizeof(strengths[0]); i++) {
		gCollator->SetDefaultStrength(strengths[i]);
		qsort(kStrings, kNumStrings, sizeof(char *), compareStrings);

		printArray(strengthLabels[i], kStrings, kNumStrings);
	}
}

