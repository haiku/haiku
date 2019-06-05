/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2004, François Revol. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <getopt.h>
#include <stdio.h>

#include <Application.h>
#include <Font.h>
#include <String.h>


static struct option const kLongOptions[] = {
	{"styles", no_argument, 0, 's'},
	{"long", no_argument, 0, 'l'},
	{"tuned", no_argument, 0, 't'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};

extern const char *__progname;
static const char *sProgramName = __progname;


void
usage(void)
{
	printf("%s [-s] [-l]\n", sProgramName);
	printf("lists currently installed font families.\n");
	printf("\t-s  --styles  list styles for each family\n");
	printf("\t-l  --long    long listing with more info (spacing, encoding,\n"
		"\t\t\theight (ascent/descent/leading), ...)\n");
	printf("\t-t  --tuned   display tuned fonts\n");
#ifndef __HAIKU__
	printf("\t-u            update font families\n");
#endif
}


int
main(int argc, char **argv)
{
	// parse command line parameters

	bool displayStyles = false;
	bool displayLong = false;
	bool displayTuned = false;
#ifndef __HAIKU__
	bool updateFamilies = false;
#endif

	int c;
	while ((c = getopt_long(argc, argv, "sltuh", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'h':
				usage();
				return 0;
			default:
				usage();
				return 1;

			case 't':
				displayTuned = true;
			case 'l':
				displayLong = true;
			case 's':
				displayStyles = true;
				break;
#ifndef __HAIKU__
			case 'u':
				updateFamilies = true;
				break;
#endif
		}
	}

	BApplication app("application/x-vnd.Haiku-listfont");

#ifndef __HAIKU__
	if (updateFamilies) {
		bool changed = update_font_families(true);
		printf("font families %s.\n", changed ? "changed" : "did not change");
		return 0;
	}
#endif

	int32 familyCount = count_font_families();

	if (displayLong) {
		printf("name/style                            face  spc. enc. "
			"height (a, d, l)  flags\n\n");
	}

	for (int32 f = 0; f < familyCount; f++) {
		font_family family;
		if (get_font_family(f, &family) < B_OK)
			continue;
		if (!displayStyles) {
			printf("%s\n", family);
			continue;
		}

		int32 styleCount = count_font_styles(family);

		for (int32 s = 0; s < styleCount; s++) {
			font_style style;
			uint16 face;
			uint32 flags;
			if (get_font_style(family, s, &style, &face, &flags) < B_OK)
				continue;

			if (!displayLong) {
				printf("%s/%s\n", family, style);
				continue;
			}

			BString fontName;
			fontName << family << "/" << style;
			printf("%-37s", fontName.String());

			BFont font;
			font.SetFamilyAndStyle(family, style);
			printf(" 0x%02x  %-4d %-4d", face, font.Spacing(), font.Encoding());

			font_height fh;
			font.GetHeight(&fh);
			printf(" %5.2f, %4.2f, %4.2f ", fh.ascent, fh.descent, fh.leading);
			if ((flags & B_IS_FIXED) != 0)
				printf("fixed");

			if ((flags & B_HAS_TUNED_FONT) != 0) {
				if (displayTuned)
					printf("\n    ");
				else if ((flags & B_IS_FIXED) != 0)
					printf(", ");

				int32 tunedCount = font.CountTuned();
				printf("%" B_PRId32 " tuned", tunedCount);

				if (displayTuned) {
					printf(":");
					for (int32 i = 0; i < tunedCount; i++) {
						tuned_font_info info;
						font.GetTunedInfo(i, &info);
						printf("\n\t(size %4.1f, "
							"shear %5.3f, "
							"rot. %5.3f, "
							"flags 0x%" B_PRIx32 ", "
							"face 0x%x)",
								info.size,
								info.shear,
								info.rotation,
								info.flags,
								info.face);
					}
				}
			}
			putchar('\n');
		}
	}
	return 0;
}
