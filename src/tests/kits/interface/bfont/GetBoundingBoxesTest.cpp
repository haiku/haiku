#include <Application.h>
#include <Font.h>
#include <Rect.h>
#include <stdio.h>

void
GetBoundingBoxesAsGlyphsTest(BFont *font)
{
	const char string[] = "test string";
	int32 numChars = 11;
	BRect rectArray[numChars];
	font->GetBoundingBoxesAsGlyphs(string, numChars, B_SCREEN_METRIC, rectArray);

	for (int32 i=0; i<numChars; i++) {
		rectArray[i].PrintToStream();
	}
}

void
GetBoundingBoxesAsStringTest(BFont *font)
{
	const char string[] = "test string";
	int32 numChars = 11;
	BRect rectArray[numChars];
	escapement_delta delta = { 0.0, 0.0 };
	font->GetBoundingBoxesAsString(string, numChars, B_SCREEN_METRIC, &delta, rectArray);

	for (int32 i=0; i<numChars; i++) {
		rectArray[i].PrintToStream();
	}
}

void
GetBoundingBoxesForStringsTest(BFont *font)
{
	char string[] = "test string";
	int32 numStrings = 1;
	const char* stringArray[numStrings];
	stringArray[0] = string;
	BRect rectArray[numStrings];
	escapement_delta delta = { 0.0, 0.0 };
	font->GetBoundingBoxesForStrings(stringArray, numStrings, B_SCREEN_METRIC, &delta, rectArray);

	for (int32 i=0; i<numStrings; i++) {
		rectArray[i].PrintToStream();
	}
}

int main()
{
	BApplication app("application/x-vnd-Test.GetBoundingBoxesTest");
	BFont font(be_plain_font);
	//font.SetRotation(45);
	//font.SetShear(45);
	font.SetSpacing(B_CHAR_SPACING);
	printf("\nGetBoundingBoxesAsGlyphsTest\n");
	GetBoundingBoxesAsGlyphsTest(&font);
	printf("\nGetBoundingBoxesAsStringTest\n");
	GetBoundingBoxesAsStringTest(&font);
	printf("\nGetBoundingBoxesForStringsTest\n");
	GetBoundingBoxesForStringsTest(&font);
}
