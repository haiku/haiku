//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.

#ifndef _SGB_NEW_STRINGS_H_
#define _SGB_NEW_STRINGS_H_

#include <SupportDefs.h>

//Fills the StringsWidths array with the width of each individual string in the array using 
//BFont::GetStringWidths(), then finds the longest string width in the array and returns that width.
//If a string_widths array is provided, it fills it in with the length of each string.
float GetStringsMaxWidth(const char** strings, int32 num_strings, const BFont* font,
	float* string_widths = NULL);

//Gets the bounding box for one or more strings.  If full_height is true, it increases the top and bottom to
//the font's ascent and descent.  If round_up is true, it rounds each edge up to the next pixel to get the
//box that will actually be occupied onscreen.
BRect StringBoundingBox(const char* string, const BFont* font, bool full_height = false,
	bool round_up = true);
BRect GetMaxStringBoundingBoxes(const char** strings, int32 num_strings, const BFont* font,
	BRect* rects = NULL, bool full_height = false, bool round_up = true);

//Each of these functions either duplicates or concatenates the strings into a new char array allocated
//with new.  The resulting char array must be delete[]'d when finished with it.
char *Strdup_new(const char *source);
char *Strcat_new(const char *string_1, const char *string_2);
char *Strcat_new(const char *string_1, const char *string_2, const char *string_3);
char *Strcat_new(const char *string_1, const char *string_2, const char *string_3, const char *string_4);

//Added because due to some error, the Be libraries on x86 don't export strtcopy.  Len includes the null
//terminator.
char *Strtcpy(char *dst, const char *src, int len);

//Just some handy functions....
void StrToUpper(char* string);
void StrToLower(char* string);

#endif