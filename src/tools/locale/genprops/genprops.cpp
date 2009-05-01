/*
*******************************************************************************
*
*	Copyright (C) 1999-2001, International Business Machines
*	Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
*	originally created by: Markus W. Scherer
*
*	This program reads the Unicode character database text file,
*	parses it, and extracts most of the properties for each character.
*	It then writes a binary file containing the properties
*	that is designed to be used directly for random-access to
*	the properties of each Unicode character.
*
*	adapted for use under BeOS by Axel Dörfler, axeld@pinc-software.de.
*/


#include "genprops.h"
#include "utf.h"

#include <UnicodeChar.h>
#include <Path.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


bool gBeVerbose = false;


/* prototypes --------------------------------------------------------------- */

static status_t parseMirror(const char *filename);
static status_t parseSpecialCasing(const char *filename);
static status_t parseCaseFolding(const char *filename);
static status_t parseDB(const char *filename);

/* -------------------------------------------------------------------------- */

typedef status_t
UParseLineFn(void *context,char *fields[][2],int32 fieldCount);


status_t
parseDelimitedFile(const char *filename, char delimiter,
                     char *fields[][2], int32 fieldCount,
                     UParseLineFn *lineFunction, void *context)
{
	FILE *file = fopen(filename,"r");
	if (file == NULL) {
		fprintf(stderr, "*** Unable to open input file %s\n",filename);
		return B_IO_ERROR;
	}

	status_t status = B_OK;
	char line[300];
	while (fgets(line,sizeof(line),file) != NULL) {
		// remove trailing newline characters
		int32 length = strlen(line);
		while (length > 0 && (line[length-1] == '\r' || line[length-1] == '\n'))
            line[--length] = '\0';

        // skip this line if it is empty or a comment
        if (line[0] == '\0' || line[0] == '#')
            continue;

		// remove in-line comments
		char *limit = strchr(line, '#');
		if (limit != NULL) {
			/* get white space before the pound sign */
			while (limit > line && (*(limit-1) == ' ' || *(limit-1) == '\t'))
				--limit;

            /* truncate the line */
            *limit = '\0';
        }

		// for each field, call the corresponding field function
		char *start = line;
		for (int32 i = 0;i < fieldCount;i++) {
			// set the limit pointer of this field
			limit = start;
			while(*limit != delimiter && *limit != 0)
				++limit;

			// set the field start and limit in the fields array
			fields[i][0] = start;
			fields[i][1] = limit;

			// set start to the beginning of the next field, if any
			start = limit;
			if (*start != 0) {
				++start;
			} else if (i+1 < fieldCount) {
				fprintf(stderr, "*** too few fields in line %s\n", line);
                status = B_ERROR;
                goto bailout;
            }
        }

        // call the field function
        status = lineFunction(context, fields, fieldCount);
        if (status < B_OK)
            break;
	}

bailout:
	fclose(file);
	return status;
}


static const char *
skipWhitespace(const char *s)
{
	while(*s == ' ' || *s == '\t')
		++s;

	return s;
}


/*
 * parse a list of code points
 * store them as a string in dest[destSize] with the string length in dest[0]
 * set the first code point in *pFirst
 * return the number of code points
 */

static int32
parseCodePoints(const char *s,UChar *dest, int32 destSize,uint32 *pFirst,status_t *pErrorCode)
{
	int32 i,count = 0;
	*pErrorCode = B_OK;

	if (pFirst != NULL)
		*pFirst = 0xffff;

	// leave dest[0] for the length value
	for (i = 1;;) {
		s = skipWhitespace(s);
		if (*s == ';' || *s == 0) {
			dest[0] = (UChar)(i-1);
			return count;
		}

		/* read one code point */
		char *end;
		uint32 value = strtoul(s, &end, 16);
		if (end <= s || (*end != ' ' && *end != '\t' && *end != ';') || value >= 0x110000) {
			fprintf(stderr, "genprops: syntax error parsing code point at %s\n", s);
			*pErrorCode = B_ERROR;
			return -1;
		}

		// store the first code point
		if (++count == 1 && pFirst != NULL)
			*pFirst = value;

		// append it to the destination array
		UTF_APPEND_CHAR(dest, i, destSize, value);

		// overflow?
		if (i >= destSize) {
			fprintf(stderr, "genprops: code point sequence too long at at %s\n", s);
			*pErrorCode = B_BAD_VALUE;
			return -1;
		}

		// go to the following characters
		s = end;
	}
}


/* parser for Mirror.txt ---------------------------------------------------- */

#define MAX_MIRROR_COUNT 2000

static uint32 mirrorMappings[MAX_MIRROR_COUNT][2];
static int32 mirrorCount = 0;


static status_t
mirrorLineFn(void *context,char *fields[][2],int32 fieldCount)
{
	char *end;

	mirrorMappings[mirrorCount][0] = strtoul(fields[0][0], &end, 16);
	if (end <= fields[0][0] || end != fields[0][1]) {
		fprintf(stderr, "genprops: syntax error in Mirror.txt field 0 at %s\n", fields[0][0]);
		return B_ERROR;
	}

	mirrorMappings[mirrorCount][1] = strtoul(fields[1][0], &end, 16);
	if (end <= fields[1][0] || end != fields[1][1]) {
		fprintf(stderr, "genprops: syntax error in Mirror.txt field 1 at %s\n", fields[1][0]);
		return B_ERROR;
	}

	if (++mirrorCount == MAX_MIRROR_COUNT) {
		fprintf(stderr, "genprops: too many mirror mappings\n");
		return B_BAD_VALUE;
	}
	return B_OK;
}


static status_t
parseMirror(const char *filename)
{
	char *fields[2][2];
	return parseDelimitedFile(filename, ';', fields, 2, mirrorLineFn, NULL);
}


/* parser for SpecialCasing.txt --------------------------------------------- */

#define MAX_SPECIAL_CASING_COUNT 500

static SpecialCasing specialCasings[MAX_SPECIAL_CASING_COUNT];
static int32 specialCasingCount = 0;


static status_t
specialCasingLineFn(void *context,char *fields[][2],int32 fieldCount)
{
	char *end;

	// get code point
	specialCasings[specialCasingCount].code = strtoul(skipWhitespace(fields[0][0]), &end, 16);
	end = (char *)skipWhitespace(end);
	if (end <= fields[0][0] || end != fields[0][1]) {
		fprintf(stderr, "genprops: syntax error in SpecialCasing.txt field 0 at %s\n", fields[0][0]);
		return B_ERROR;
	}

	// is this a complex mapping?
	if (*skipWhitespace(fields[4][0]) != 0) {
		// there is some condition text in the fifth field
		specialCasings[specialCasingCount].isComplex = true;
		
		// do not store any actual mappings for this
		specialCasings[specialCasingCount].lowerCase[0] = 0;
		specialCasings[specialCasingCount].upperCase[0] = 0;
		specialCasings[specialCasingCount].titleCase[0] = 0;
	} else {
		// just set the "complex" flag and get the case mappings
		specialCasings[specialCasingCount].isComplex = false;
		status_t errorCode = B_OK;
		parseCodePoints(fields[1][0], specialCasings[specialCasingCount].lowerCase, 32, NULL, &errorCode);
		parseCodePoints(fields[3][0], specialCasings[specialCasingCount].upperCase, 32, NULL, &errorCode);
		parseCodePoints(fields[2][0], specialCasings[specialCasingCount].titleCase, 32, NULL, &errorCode);
		if (errorCode < B_OK) {
			fprintf(stderr, "genprops: error parsing special casing at %s\n", fields[0][0]);
			return errorCode;
		}
	}

	if (++specialCasingCount == MAX_SPECIAL_CASING_COUNT) {
		fprintf(stderr, "genprops: too many special casing mappings\n");
		return B_BAD_VALUE;
	}
	return B_OK;
}


static int
compareSpecialCasings(const void *left, const void *right)
{
	return ((const SpecialCasing *)left)->code - ((const SpecialCasing *)right)->code;
}


static status_t
parseSpecialCasing(const char *filename)
{
	char *fields[5][2];
	status_t status = parseDelimitedFile(filename, ';', fields, 5, specialCasingLineFn, NULL);
	if (status < B_OK)
		return status;

	// sort the special casing entries by code point
	if (specialCasingCount>0)
		qsort(specialCasings, specialCasingCount, sizeof(SpecialCasing), compareSpecialCasings);

	// replace multiple entries for any code point by one "complex" one
	int32 j = 0;
	for (int32 i = 1;i < specialCasingCount;++i) {
		if (specialCasings[i-1].code == specialCasings[i].code) {
			// there is a duplicate code point
			specialCasings[i-1].code = 0x7fffffff;	// remove this entry in the following qsort
			specialCasings[i].isComplex = true;		// make the following one complex
			specialCasings[i].lowerCase[0] = 0;
			specialCasings[i].upperCase[0] = 0;
			specialCasings[i].titleCase[0] = 0;
			j++;
		}
	}

    /* if some entries just were removed, then re-sort */
    if (j > 0) {
        qsort(specialCasings, specialCasingCount, sizeof(SpecialCasing), compareSpecialCasings);
        specialCasingCount -= j;
    }
    return B_OK;
}


/* parser for CaseFolding.txt ----------------------------------------------- */

#define MAX_CASE_FOLDING_COUNT 500

static CaseFolding caseFoldings[MAX_CASE_FOLDING_COUNT];
static int32 caseFoldingCount = 0;


static status_t
caseFoldingLineFn(void *context,char *fields[][2],int32 fieldCount)
{
	char *end;
	int32 count;
	char status;

	// get code point
	caseFoldings[caseFoldingCount].code = strtoul(skipWhitespace(fields[0][0]), &end, 16);
	end = (char *)skipWhitespace(end);
	if (end <= fields[0][0] || end != fields[0][1]) {
		fprintf(stderr, "genprops: syntax error in CaseFolding.txt field 0 at %s\n", fields[0][0]);
		return B_ERROR;
	}

	// get the status of this mapping
	caseFoldings[caseFoldingCount].status = status = *skipWhitespace(fields[1][0]);
	if (status != 'L' && status != 'E' && status != 'C'
		&& status != 'S' && status != 'F' && status != 'I') {
		fprintf(stderr, "genprops: unrecognized status field in CaseFolding.txt at %s\n", fields[0][0]);
		return B_ERROR;
	}

	// ignore all case folding mappings that are the same as the UnicodeData.txt lowercase mappings
	if (status == 'L')
        return B_OK;

	// get the mapping
	status_t errorCode;
	count = parseCodePoints(fields[2][0], caseFoldings[caseFoldingCount].full, 32, &caseFoldings[caseFoldingCount].simple, &errorCode);
    if (errorCode < B_OK) {
		fprintf(stderr, "genprops: error parsing CaseFolding.txt mapping at %s\n", fields[0][0]);
		return errorCode;
	}

	// there is a simple mapping only if there is exactly one code point
	if (count != 1)
		caseFoldings[caseFoldingCount].simple = 0;

	// check the status
    if (status == 'S') {
		// check if there was a full mapping for this code point before
		if (caseFoldingCount > 0
			&& caseFoldings[caseFoldingCount-1].code == caseFoldings[caseFoldingCount].code
			&& caseFoldings[caseFoldingCount-1].status == 'F') {
			// merge the two entries
			caseFoldings[caseFoldingCount-1].simple=caseFoldings[caseFoldingCount].simple;
			return B_OK;
		}
	} else if (status == 'F') {
		// check if there was a simple mapping for this code point before */
		if (caseFoldingCount > 0
			&& caseFoldings[caseFoldingCount-1].code == caseFoldings[caseFoldingCount].code
			&& caseFoldings[caseFoldingCount-1].status == 'S') {
			// merge the two entries
			memcpy(caseFoldings[caseFoldingCount-1].full, caseFoldings[caseFoldingCount].full, 32 * U_SIZEOF_UCHAR);
			return B_OK;
		}
	} else if (status == 'I') {
		// store only a marker for special handling for cases like dotless i
		caseFoldings[caseFoldingCount].simple = 0;
		caseFoldings[caseFoldingCount].full[0] = 0;
	}

	if (++caseFoldingCount == MAX_CASE_FOLDING_COUNT) {
        fprintf(stderr, "genprops: too many case folding mappings\n");
        return B_BAD_VALUE;
	}
	return B_OK;
}


static status_t
parseCaseFolding(const char *filename)
{
	char *fields[3][2];
	return parseDelimitedFile(filename, ';', fields, 3, caseFoldingLineFn, NULL);
}


/* parser for UnicodeData.txt ----------------------------------------------- */

// general categories
const char *const
genCategoryNames[B_UNICODE_CATEGORY_COUNT] = {
	NULL,
	"Lu", "Ll", "Lt", "Lm", "Lo", "Mn", "Me",
	"Mc", "Nd", "Nl", "No",
	"Zs", "Zl", "Zp",
	"Cc", "Cf", "Co", "Cs",
	"Pd", "Ps", "Pe", "Pc", "Po",
	"Sm", "Sc", "Sk", "So",
	"Pi", "Pf",
	"Cn"
};

const char *const
bidiNames[B_UNICODE_DIRECTION_COUNT] = {
	"L", "R", "EN", "ES", "ET", "AN", "CS", "B", "S",
	"WS", "ON", "LRE", "LRO", "AL", "RLE", "RLO", "PDF", "NSM", "BN"
};

// control code properties
static const struct {
	uint32	code;
	uint8	generalCategory;
} controlProps[] = {
	/* TAB */	{ 0x9, B_UNICODE_SPACE_SEPARATOR },
	/* VT */	{ 0xb, B_UNICODE_SPACE_SEPARATOR },
	/* LF */	{ 0xa, B_UNICODE_PARAGRAPH_SEPARATOR },
	/* FF */	{ 0xc, B_UNICODE_LINE_SEPARATOR },
	/* CR */	{ 0xd, B_UNICODE_PARAGRAPH_SEPARATOR },
	/* FS */	{ 0x1c, B_UNICODE_PARAGRAPH_SEPARATOR },
	/* GS */	{ 0x1d, B_UNICODE_PARAGRAPH_SEPARATOR },
	/* RS */	{ 0x1e, B_UNICODE_PARAGRAPH_SEPARATOR },
	/* US */	{ 0x1f, B_UNICODE_SPACE_SEPARATOR },
	/* NL */	{ 0x85, B_UNICODE_PARAGRAPH_SEPARATOR }
};

static struct {
	uint32	first, last, props;
	char	name[80];
} unicodeAreas[32];

static int32 unicodeAreaIndex = 0;


static status_t
unicodeDataLineFn(void *context,char *fields[][2],int32 fieldCount)
{
	static int32 mirrorIndex = 0, specialCasingIndex = 0, caseFoldingIndex = 0;
	Props props;
	char *end;
	uint32 value;

	// reset the properties
	memset(&props, 0, sizeof(Props));
	props.decimalDigitValue = props.digitValue = -1;
	props.numericValue = 0x80000000;

	// get the character code, field 0
	props.code = strtoul(fields[0][0], &end, 16);
	if (end <= fields[0][0] || end != fields[0][1]) {
		fprintf(stderr, "genprops: syntax error in field 0 at %s\n", fields[0][0]);
		return B_ERROR;
	}

	// get general category, field 2
	*fields[2][1] = 0;
	for (int i = 1;;) {
		if (!strcmp(fields[2][0], genCategoryNames[i])) {
			props.generalCategory = (uint8)i;
			break;
		}
		if (++i == B_UNICODE_CATEGORY_COUNT) {
			fprintf(stderr, "genprops: unknown general category \"%s\" at code 0x%lx\n", fields[2][0], props.code);
			return B_ERROR;
		}
	}

	// get canonical combining class, field 3
	props.canonicalCombining = (uint8)strtoul(fields[3][0], &end, 10);
	if (end <= fields[3][0] || end != fields[3][1]) {
		fprintf(stderr, "genprops: syntax error in field 3 at code 0x%lx\n", props.code);
		return B_ERROR;
	}

	// get BiDi category, field 4
	*fields[4][1] = 0;
	for (int i = 0;;) {
		if (!strcmp(fields[4][0], bidiNames[i])) {
			props.bidi = (uint8)i;
			break;
		}
		if (++i == B_UNICODE_DIRECTION_COUNT) {
			fprintf(stderr, "genprops: unknown BiDi category \"%s\" at code 0x%lx\n", fields[4][0], props.code);
			return B_ERROR;
		}
	}

	// decimal digit value, field 6
	if (fields[6][0] < fields[6][1]) {
		value = strtoul(fields[6][0], &end, 10);
		if (end != fields[6][1] || value > 0x7fff) {
			fprintf(stderr, "genprops: syntax error in field 6 at code 0x%lx\n", props.code);
			return B_ERROR;
		}
		props.decimalDigitValue = (int16)value;
	}

	// digit value, field 7
	if (fields[7][0] < fields[7][1]) {
		value = strtoul(fields[7][0], &end, 10);
		if (end != fields[7][1] || value > 0x7fff) {
			fprintf(stderr, "genprops: syntax error in field 7 at code 0x%lx\n", props.code);
			return B_ERROR;
		}
		props.digitValue = (int16)value;
	}

	// numeric value, field 8
	if (fields[8][0] < fields[8][1]) {
		char *s = fields[8][0];
		bool isNegative;

		// get a possible minus sign
		if (*s == '-') {
			isNegative = true;
			++s;
		} else
			isNegative = false;

		value = strtoul(s, &end, 10);
		if (value > 0 && *end == '/') {
			// field 8 may contain a fractional value, get the denominator
			props.denominator = strtoul(end+1, &end, 10);
			if (props.denominator == 0) {
				fprintf(stderr, "genprops: denominator is 0 in field 8 at code 0x%lx\n", props.code);
				return B_ERROR;
			}
		}
		if (end != fields[8][1] || value > 0x7fffffff) {
			fprintf(stderr, "genprops: syntax error in field 8 at code 0x%lx\n", props.code);
			return B_ERROR;
		}

		if (isNegative)
			props.numericValue = -(int32)value;
		else
			props.numericValue = (int32)value;

		props.hasNumericValue = true;
	}

	// get Mirrored flag, field 9
	if (*fields[9][0] == 'Y') {
		props.isMirrored = 1;
	} else if (fields[9][1] - fields[9][0] != 1 || *fields[9][0] != 'N') {
		fprintf(stderr, "genprops: syntax error in field 9 at code 0x%lx\n", props.code);
		return B_ERROR;
	}

	// get uppercase mapping, field 12
	value = strtoul(fields[12][0], &end, 16);
	if (end != fields[12][1]) {
		fprintf(stderr, "genprops: syntax error in field 12 at code 0x%lx\n", props.code);
		return B_ERROR;
	}
	props.upperCase = value;

	// get lowercase value, field 13
	value = strtoul(fields[13][0], &end, 16);
	if (end != fields[13][1]) {
		fprintf(stderr, "genprops: syntax error in field 13 at code 0x%lx\n", props.code);
		return B_ERROR;
	}
	props.lowerCase = value;

	// get titlecase value, field 14
	value = strtoul(fields[14][0], &end, 16);
	if (end != fields[14][1]) {
		fprintf(stderr, "genprops: syntax error in field 14 at code 0x%lx\n", props.code);
		return B_ERROR;
	}
	props.titleCase = value;

	// override properties for some common control characters
	if (props.generalCategory == B_UNICODE_CONTROL_CHAR) {
		for (uint32 i = 0; i < sizeof(controlProps) / sizeof(controlProps[0]); i++) {
			if (controlProps[i].code == props.code)
				props.generalCategory = controlProps[i].generalCategory;
		}
	}

	// set additional properties from previously parsed files
	if (mirrorIndex < mirrorCount && props.code == mirrorMappings[mirrorIndex][0])
		props.mirrorMapping = mirrorMappings[mirrorIndex++][1];

	if (specialCasingIndex < specialCasingCount && props.code == specialCasings[specialCasingIndex].code)
		props.specialCasing = specialCasings + specialCasingIndex++;
	else
		props.specialCasing = NULL;

	if (caseFoldingIndex < caseFoldingCount && props.code == caseFoldings[caseFoldingIndex].code) {
		props.caseFolding = caseFoldings + caseFoldingIndex++;

		// ignore "Common" mappings (simple==full) that map to the same code
		// point as the regular lowercase mapping
		if (props.caseFolding->status == 'C' && props.caseFolding->simple == props.lowerCase)
			props.caseFolding = NULL;
	} else
		props.caseFolding = NULL;

	value = makeProps(&props);

	if (*fields[1][0] == '<') {
		// first or last entry of a Unicode area
		size_t length = fields[1][1] - fields[1][0];

		if (length < 9) {
            /* name too short for an area name */
		} else if (!memcmp(", First>", fields[1][1]-8, 8)) {
			// set the current area
			if (unicodeAreas[unicodeAreaIndex].first == 0xffffffff) {
				length -= 9;
				unicodeAreas[unicodeAreaIndex].first = props.code;
				unicodeAreas[unicodeAreaIndex].props = value;
				memcpy(unicodeAreas[unicodeAreaIndex].name, fields[1][0]+1, length);
				unicodeAreas[unicodeAreaIndex].name[length] = 0;
			} else {
				// error: a previous area is incomplete
				fprintf(stderr, "genprops: error - area \"%s\" is incomplete\n", unicodeAreas[unicodeAreaIndex].name);
				return B_ERROR;
			}
			return B_OK;
		} else if (!memcmp(", Last>", fields[1][1]-7, 7)) {
            // check that the current area matches, and complete it with the last code point
            length -= 8;
			if (unicodeAreas[unicodeAreaIndex].props == value
				&& !memcmp(unicodeAreas[unicodeAreaIndex].name, fields[1][0]+1, length)
				&& unicodeAreas[unicodeAreaIndex].name[length] == 0
				&& unicodeAreas[unicodeAreaIndex].first < props.code) {

				unicodeAreas[unicodeAreaIndex].last = props.code;
				if (gBeVerbose) {
					printf("Unicode area U+%04lx..U+%04lx \"%s\"\n",
							unicodeAreas[unicodeAreaIndex].first,
							unicodeAreas[unicodeAreaIndex].last,
							unicodeAreas[unicodeAreaIndex].name);
				}
					unicodeAreas[++unicodeAreaIndex].first = 0xffffffff;
			} else {
				// error: different properties between first & last, different area name, first >= last
				fprintf(stderr, "genprops: error - Last of area \"%s\" is incorrect\n", unicodeAreas[unicodeAreaIndex].name);
				return B_ERROR;
			}
			return B_OK;
		} else {
			/* not an area name */
		}
	}

	// properties for a single code point
	// ### TODO: check that the code points (props.code) are in ascending order
	addProps(props.code, value);
	return B_OK;
}


/* set repeated properties for the areas */

static void
repeatAreaProps()
{
	uint32 puaProps;
	int32 i;
	bool hasPlane15PUA, hasPlane16PUA;

	/*
	 * UnicodeData.txt before 3.0.1 did not contain the PUAs on
	 * planes 15 and 16.
	 * If that is the case, then we add them here, using the properties
	 * from the BMP PUA.
	 */
	puaProps = 0;
	hasPlane15PUA = hasPlane16PUA = false;

	for (i = 0;i < unicodeAreaIndex;i++) {
		repeatProps(unicodeAreas[i].first,unicodeAreas[i].last,unicodeAreas[i].props);

		if (unicodeAreas[i].first == 0xe000)
			puaProps = unicodeAreas[i].props;
		else if (unicodeAreas[i].first == 0xf0000)
			hasPlane15PUA = false;
		else if (unicodeAreas[i].first == 0x100000)
			hasPlane16PUA = true;
	}

	if (puaProps != 0) {
		if (!hasPlane15PUA)
			repeatProps(0xf0000, 0xffffd, puaProps);

		if (!hasPlane16PUA)
			repeatProps(0x100000, 0x10fffd, puaProps);
	}
}


static status_t
parseDB(const char *filename)
{
	// while unicodeAreas[unicodeAreaIndex] is unused, set its first to a bogus value
	unicodeAreas[0].first = 0xffffffff;

	char *fields[15][2];
	status_t status = parseDelimitedFile(filename, ';', fields, 15, unicodeDataLineFn, NULL);
	if (status < B_OK)
		return status;

	if (unicodeAreas[unicodeAreaIndex].first != 0xffffffff) {
		fprintf(stderr, "genprops: error - the last area \"%s\" from U+%04lx is incomplete\n",
					unicodeAreas[unicodeAreaIndex].name,
					unicodeAreas[unicodeAreaIndex].first);
		return B_ERROR;
	}

	repeatAreaProps();
	return B_OK;
}


int
main(int argc,char **argv)
{
    const char *srcDir = "data", *destDir = ".";

//	gBeVerbose = true;
	if (argc >= 2 && argv[1])
		srcDir = argv[1];

	// prepare the filename beginning with the source dir
	
	initStore();

	BPath path(srcDir,"Mirror.txt");
	status_t status = parseMirror(path.Path());
	if (status < B_OK)
		return -1;

	path.SetTo(srcDir,"SpecialCasing.txt");
	status = parseSpecialCasing(path.Path());
	if (status < B_OK)
		return -1;

	path.SetTo(srcDir,"CaseFolding.txt");
	status = parseCaseFolding(path.Path());
	if (status < B_OK)
		return -1;

	path.SetTo(srcDir,"UnicodeData.txt");
	status = parseDB(path.Path());
	if (status < B_OK)
		return -1;

	// process parsed data
	compactProps();
	compactStage3();
	compactStage2();

	// write the properties data file
	return generateData(destDir);
}
