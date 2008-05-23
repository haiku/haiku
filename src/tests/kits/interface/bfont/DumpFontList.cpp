#include <Application.h>
#include <Font.h>
#include <stdio.h>


#define PRINT_FACE_FLAG(x) \
	if (face & x) { \
		printf(#x); \
		face &= ~x; \
		if (face != 0) \
			printf(", "); \
	}


int main()
{
	BApplication app("application/x-vnd-Test.DumpFontList");

	int32 familyCount = count_font_families();
	for (int32 i = 0; i < familyCount; i++) {
		font_family family;
		uint32 familyFlags = 0;

		if (get_font_family(i, &family, &familyFlags) == B_OK) {
			printf("family \"%s\" (0x%08lx)\n", family, familyFlags);

			int32 styleCount = count_font_styles(family);
			for (int32 j = 0; j < styleCount; j++) {
				font_style style;
				uint32 styleFlags = 0;

				if (get_font_style(family, j, &style, &styleFlags) == B_OK) {
					printf("\tstyle \"%s\" (0x%08lx)\n", style, styleFlags);

					BFont font;
					font.SetFamilyAndStyle(family, style);
					printf("\t\tcode: 0x%08lx\n", font.FamilyAndStyle());
					printf("\t\tsize: %f\n", font.Size());
					printf("\t\tshear: %f\n", font.Shear());
					printf("\t\trotation: %f\n", font.Rotation());
					printf("\t\tspacing: %u\n", font.Spacing());
					printf("\t\tencoding: %u\n", font.Encoding());

					uint16 face = font.Face();
					printf("\t\tface: 0x%04x (", face);
					PRINT_FACE_FLAG(B_ITALIC_FACE);
					PRINT_FACE_FLAG(B_UNDERSCORE_FACE);
					PRINT_FACE_FLAG(B_NEGATIVE_FACE);
					PRINT_FACE_FLAG(B_OUTLINED_FACE);
					PRINT_FACE_FLAG(B_STRIKEOUT_FACE);
					PRINT_FACE_FLAG(B_BOLD_FACE);
					PRINT_FACE_FLAG(B_REGULAR_FACE);
					if (face != 0)
						printf("unknown 0x%04x", face);
					printf(")\n");

					printf("\t\tflags: 0x%08lx\n", font.Flags());

					switch (font.Direction()) {
						case B_FONT_LEFT_TO_RIGHT:
							printf("\t\tdirection: left to right\n");
							break;

						case B_FONT_RIGHT_TO_LEFT:
							printf("\t\tdirection: right to left\n");
							break;

						default:
							printf("\t\tdirection: unknown (%d)\n", font.Direction());
							break;
					}

					printf("\t\tis fixed: %s\n", font.IsFixed() ? "yes" : "no");
					printf("\t\tfull/half fixed: %s\n", font.IsFullAndHalfFixed() ? "yes" : "no");

					switch (font.FileFormat()) {
						case B_TRUETYPE_WINDOWS:
							printf("\t\tfiletype: truetype\n");
							break;

						case B_POSTSCRIPT_TYPE1_WINDOWS:
							printf("\t\tfiletype: postscript type 1\n");
							break;

						default:
							printf("\t\tfiletype: unknown (%d)\n", font.FileFormat());
							break;
					}

					//printf("\t\ttuned count: %ld\n", font.CountTuned());
					printf("\n");
				} else
					printf("\tgetting style %ld failed\n", j);
			}
		} else
			printf("getting family %ld failed\n", i);
	}
}
