/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <parsedate.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


/* The date format is as follows:
 *
 *	a/A		weekday
 *	d		day of month
 *	b/B		month name
 *	m		month
 *	y/Y		year
 *	H/I		hours
 *	M		minute
 *	S		seconds
 *	p		meridian (i.e. am/pm)
 *	T		time unit: last hour, next tuesday, today, ...
 *	z/Z		time zone
 *	-		dash or slash
 *
 *	Any of ",.:" is allowed and will be expected in the input string as is.
 *	You can enclose a single field with "[]" to mark it as being optional.
 *	A space stands for white space.
 *	No other character is allowed.
 */

static const char * const kFormatsTable[] = {
	"[A][,] B d[,] H:M:S [p] Y[,] [Z]",
	"[A][,] B d[,] [Y][,] H:M:S [p] [Z]",
	"[A][,] B d[,] [Y][,] H:M [p][,] [Z]",
	"[A][,] B d[,] [Y][,] H [p][,] [Z]",
	"[A][,] B d[,] H:M [p][,] [Y] [Z]",
	"[A][,] d B[,] [Y][,] H:M [p][,] [Z]",
	"[A][,] d B[,] [Y][,] H:M:S [p][,] [Z]",
	"[A][,] d B[,] H:M:S [Y][,] [p][,] [Z]",
	"[A][,] d B[,] H:M [Y][,] [p][,] [Z]",
	"d.m.y H:M:S [p] [Z]",
	"d.m.y H:M [p] [Z]",
	"d.m.y",
	"[A][,] m-d-y[,] [H][:][M] [p]",
	"[A][,] m-d[,] H:M [p]",
	"[A][,] m-d[,] H[p]",
	"[A][,] m-d",
	"[A][,] B d[,] Y",
	"[A][,] H:M [p]",
	"[A][,] H [p]",
	"H:M [p][,] [A]",
	"H [p][,] [A]",
	"[A][,] B d[,] H:M:S [p] [Z] [Y]",
	"[A][,] B d[,] H:M [p] [Z] [Y]",
	"[A][,] d B [,] H:M:S [p] [Z] [Y]",
	"[A][,] d B [,] H:M [p] [Z] [Y]",
	"[A][,] d-B-Y H:M:S [p] [Z]",
	"[A][,] d-B-Y H:M [p] [Z]",
	"d B Y H:M:S [Z]",
	"d B Y H:M [Z]",
	"y-m-d",
	"y-m-d H:M:S [p] [Z]",
	"m-d-y H[p]",
	"m-d-y H:M[p]",
	"m-d-y H:M:S[p]",
	"H[p] m-d-y",
	"H:M[p] m-d-y",
	"H:M:S[p] m-d-y",
	"A[,] H:M:S [p] [Z]",
	"A[,] H:M [p] [Z]",
	"H:M:S [p] [Z]",
	"H:M [p] [Z]",
	"A[,] [B] [d] [Y]",
	"A[,] [d] [B] [Y]",
	"B d[,][Y] H[p][,] [Z]",
	"B d[,] H[p]",
	"B d [,] H:M [p]",
	"d B [,][Y] H [p] [Z]",
	"d B [,] H:M [p]",
	"B d [,][Y]",
	"B d [,] H:M [p][,] [Y]",
	"B d [,] H [p][,] [Y]",
	"d B [,][Y]",
	"H[p] [,] B d",
	"H:M[p] [,] B d",
	"T [T][T][T][T][T]",
	"T H:M:S [p]",
	"T H:M [p]",
	"T H [p]",
	"H:M [p] T",
	"H [p] T",
	"H [p]",
	NULL
};
static const char * const *sFormatsTable = kFormatsTable;


enum field_type {
	TYPE_DAY_UNIT		= 0x0100,
	TYPE_SECOND_UNIT	= 0x0200,
	TYPE_MONTH_UNIT		= 0x0400,
	TYPE_YEAR_UNIT		= 0x0800,

	TYPE_RELATIVE		= 0x1000,
	TYPE_NOT_MODIFIABLE = 0x2000,
	TYPE_MODIFIER		= 0x4000,
	TYPE_UNIT			= 0x8000,

	TYPE_UNIT_MASK		= 0x0f00,
	TYPE_MASK			= 0x00ff,

	TYPE_UNKNOWN		= 0,

	TYPE_DAY			= 1 | TYPE_DAY_UNIT,
	TYPE_MONTH			= 2 | TYPE_MONTH_UNIT,
	TYPE_YEAR			= 3 | TYPE_MONTH_UNIT,
	TYPE_WEEKDAY		= 4 | TYPE_DAY_UNIT,
	TYPE_NOW			= 5,
	TYPE_HOUR			= 6 | TYPE_SECOND_UNIT,
	TYPE_MINUTE			= 7 | TYPE_SECOND_UNIT,
	TYPE_SECOND			= 8 | TYPE_SECOND_UNIT,
	TYPE_TIME_ZONE		= 9 | TYPE_SECOND_UNIT,
	TYPE_MERIDIAN		= 10 | TYPE_SECOND_UNIT,

	TYPE_DASH			= 12,
	TYPE_DOT			= 13,
	TYPE_COMMA			= 14,
	TYPE_COLON			= 15,
	
	TYPE_PLUS_MINUS		= 16 | TYPE_MODIFIER | TYPE_RELATIVE,
	TYPE_NEXT_LAST_THIS	= 17 | TYPE_MODIFIER | TYPE_RELATIVE,

	TYPE_END,
};

enum value_type {
	VALUE_NUMERICAL,
	VALUE_STRING,
	VALUE_CHAR,
};

enum value_modifier {
	MODIFY_PLUS,
	MODIFY_MINUS,
	MODIFY_NEXT,
	MODIFY_THIS,
	MODIFY_LAST,
};

struct known_identifier {
	const char	*string;
	const char	*alternate_string;
	int32		type;
	int32		value;
};

static const known_identifier kIdentifiers[] = {
	{"today",		NULL,	TYPE_RELATIVE | TYPE_NOT_MODIFIABLE | TYPE_DAY_UNIT, 0},
	{"tomorrow",	NULL,	TYPE_RELATIVE | TYPE_NOT_MODIFIABLE | TYPE_DAY_UNIT, 1},
	{"yesterday",	NULL,	TYPE_RELATIVE | TYPE_NOT_MODIFIABLE | TYPE_DAY_UNIT, -1},
	{"now",			NULL,	TYPE_RELATIVE | TYPE_NOT_MODIFIABLE | TYPE_NOW, 0},

	{"this",		NULL,	TYPE_NEXT_LAST_THIS, 0},
	{"next",		NULL,	TYPE_NEXT_LAST_THIS, 1},
	{"last",		NULL,	TYPE_NEXT_LAST_THIS, -1},

	{"years",		"year",	TYPE_UNIT | TYPE_RELATIVE | TYPE_YEAR_UNIT, 1},
	{"months",		"month",TYPE_UNIT | TYPE_RELATIVE | TYPE_MONTH_UNIT, 1},
	{"days",		"day",	TYPE_UNIT | TYPE_RELATIVE | TYPE_DAY_UNIT, 1},
	{"hour",		NULL,	TYPE_UNIT | TYPE_RELATIVE | TYPE_SECOND_UNIT, 1 * 60 * 60},
	{"hours",		"hrs",	TYPE_UNIT | TYPE_RELATIVE | TYPE_SECOND_UNIT, 1 * 60 * 60},
	{"second",		"sec",	TYPE_UNIT | TYPE_RELATIVE | TYPE_SECOND_UNIT, 1},
	{"seconds",		"secs",	TYPE_UNIT | TYPE_RELATIVE | TYPE_SECOND_UNIT, 1},
	{"minute",		"min",	TYPE_UNIT | TYPE_RELATIVE | TYPE_SECOND_UNIT, 60},
	{"minutes",		"mins",	TYPE_UNIT | TYPE_RELATIVE | TYPE_SECOND_UNIT, 60},

	{"am",			NULL,	TYPE_MERIDIAN | TYPE_NOT_MODIFIABLE, 0},
	{"pm",			NULL,	TYPE_MERIDIAN | TYPE_NOT_MODIFIABLE, 12 * 60 * 60},	// 12 hours

	{"sunday",		"sun",	TYPE_WEEKDAY, 0},
	{"monday",		"mon",	TYPE_WEEKDAY, 1},
	{"tuesday",		"tue",	TYPE_WEEKDAY, 2},
	{"wednesday",	"wed",	TYPE_WEEKDAY, 3},
	{"thursday",	"thu",	TYPE_WEEKDAY, 4},
	{"friday",		"fri",	TYPE_WEEKDAY, 5},
	{"saturday",	"sat",	TYPE_WEEKDAY, 6},

	{"january",		"jan",	TYPE_MONTH, 1},
	{"february",	"feb", 	TYPE_MONTH, 2},
	{"march",		"mar",	TYPE_MONTH, 3},
	{"april",		"apr",	TYPE_MONTH, 4},
	{"may",			"may",	TYPE_MONTH, 5},
	{"june",		"jun",	TYPE_MONTH, 6},
	{"july",		"jul",	TYPE_MONTH, 7},
	{"august",		"aug",	TYPE_MONTH, 8},
	{"september",	"sep",	TYPE_MONTH, 9},
	{"october",		"oct",	TYPE_MONTH, 10},
	{"november",	"nov",	TYPE_MONTH, 11},
	{"december",	"dec",	TYPE_MONTH, 12},

	{"GMT",			NULL,	TYPE_TIME_ZONE,	0},
		// ToDo: add more time zones

	{NULL}
};

#define MAX_ELEMENTS	32

struct parsed_element {
	int32		type;
	int32		value_type;
	int32		modifier;
	bigtime_t	value;
};


bool
isType(int32 typeA, int32 typeB)
{
	return (typeA & TYPE_MASK) == (typeB & TYPE_MASK);
}


status_t
preparseDate(const char *dateString, parsed_element *elements)
{
	int32 index = 0;
	char c;

	if (dateString == NULL)
		return B_ERROR;

	for (; (c = dateString[0]) != '\0'; dateString++) {
		// we don't care about spaces
		if (isspace(c))
			continue;

		// if we're reached our maximum number of elements, bail out
		if (index >= MAX_ELEMENTS)
			return B_ERROR;

		memset(&elements[index], 0, sizeof(parsed_element));

		if (c == ',') {
			elements[index].type = TYPE_COMMA;
			elements[index].value_type = VALUE_CHAR;
		} else if (c == '.') {
			elements[index].type = TYPE_DOT;
			elements[index].value_type = VALUE_CHAR;
		} else if (c == '/' || c == '-') {
			// ToDo: "-" can also be the minus modifier... - how to distinguish?
			elements[index].type = TYPE_DASH;
			elements[index].value_type = VALUE_CHAR;
		} else if (c == ':') {
			elements[index].type = TYPE_COLON;
			elements[index].value_type = VALUE_CHAR;
		} else if (c == '+') {
			elements[index].type = TYPE_RELATIVE;
			elements[index].value_type = VALUE_CHAR;
			elements[index].modifier = MODIFY_PLUS;

			// this counts for the next element
			continue;
		} else if (c == '-') {
			// ToDo: this never happens, due to TYPE_DASH
			elements[index].type = TYPE_MODIFIER;
			elements[index].value_type = VALUE_CHAR;
			elements[index].modifier = MODIFY_MINUS;

			// this counts for the next element
			continue;
		} else if (isdigit(c)) {
			// fetch whole number

			elements[index].type = TYPE_UNKNOWN;
			elements[index].value_type = VALUE_NUMERICAL;
			elements[index].value = atoll(dateString);

			// skip number
			while (isdigit(dateString[1]))
				dateString++;
		} else if (isalpha(c)) {
			// fetch whole string

			const char *string = dateString;
			while (isalpha(dateString[1]))
				dateString++;
			int32 length = dateString + 1 - string;

			// compare with known strings
			// ToDo: should understand other languages as well...

			const known_identifier *identifier = kIdentifiers;
			for (; identifier->string; identifier++) {
				if (!strncasecmp(identifier->string, string, length)
					&& !identifier->string[length])
					break;

				if (identifier->alternate_string != NULL
					&& !strncasecmp(identifier->alternate_string, string, length)
					&& !identifier->alternate_string[length])
					break;
			}
			if (identifier->string == NULL) {
				// unknown string, we don't have to parse any further
				return B_ERROR;
			}

			if (index > 0 && (identifier->type & TYPE_UNIT) != 0) {
				// this is just a unit, so it will give the last value a meaning

				if (elements[--index].value_type != VALUE_NUMERICAL
					&& elements[index].type != TYPE_NEXT_LAST_THIS)
					return B_ERROR;

				elements[index].value *= identifier->value;
			} else if (index > 0 && elements[index - 1].type == TYPE_NEXT_LAST_THIS) {
				if (isType(identifier->type, TYPE_MONTH)
					|| isType(identifier->type, TYPE_WEEKDAY)) {
					index--;

					switch (elements[index].value) {
						case -1:
							elements[index].modifier = MODIFY_LAST;
							break;
						case 0:
							elements[index].modifier = MODIFY_THIS;
							break;
						case 1:
							elements[index].modifier = MODIFY_NEXT;
							break;
					}
					elements[index].value = identifier->value;
				} else
					return B_ERROR;
			} else {
				elements[index].value_type = VALUE_STRING;
				elements[index].value = identifier->value;
			}

			elements[index].type = identifier->type;
		}

		// see if we can join any preceding modifiers

		if (index > 0
			&& (elements[index - 1].type & TYPE_MODIFIER) != 0
			&& (elements[index].type & TYPE_NOT_MODIFIABLE) == 0) {
			// copy the current one to the last and go on
			elements[index].modifier = elements[index - 1].modifier;
			elements[index].value *= elements[index - 1].value;
			elements[index - 1] = elements[index];
		} else {
			// we filled out one parsed_element
			index++;
		}
	}

	// were there any elements?
	if (index == 0)
		return B_ERROR;

	elements[index].type = TYPE_END;

	return B_OK;
}


static void
computeRelativeUnit(parsed_element &element, struct tm &tm, time_t &now, bool &makeTime, bool isString)
{
	uint32 type = element.type;

	// ToDo: should change flags as well!

	makeTime = true;
	localtime_r(&now, &tm);

	// set the relative start depending on unit

	if (type & (TYPE_DAY_UNIT | TYPE_MONTH_UNIT | TYPE_YEAR_UNIT)) {
		tm.tm_hour = 0;
		tm.tm_min = 0;
		tm.tm_sec = 0;
	}
	if (type & (TYPE_MONTH_UNIT | TYPE_YEAR_UNIT))
		tm.tm_mday = 1;
	if (type & TYPE_YEAR_UNIT)
		tm.tm_mon = 0;

	// adjust value

	if (type & TYPE_RELATIVE) {
		if (type & TYPE_MONTH_UNIT)
			tm.tm_mon += element.value;
		else if (type & TYPE_DAY_UNIT)
			tm.tm_mday += element.value;
		else if (type & TYPE_SECOND_UNIT)
			tm.tm_sec += element.value;
		else if (!isType(type, TYPE_NOW))
			;
			//puts("what's going on here?");
	} else if (isType(type, TYPE_WEEKDAY)) {
		tm.tm_mday += abs(element.value) - tm.tm_wday;

		if (element.modifier == MODIFY_NEXT)
			tm.tm_mday += 7;
		else if (element.modifier == MODIFY_LAST)
			tm.tm_mday -= 7;
	}
}


static time_t
computeDate(const char *format, bool *optional, parsed_element *elements, time_t now, int *_flags)
{
	//printf("matches: %s\n", format);

	parsed_element *element = elements;
	uint32 position = 0;
	struct tm tm;
	time_t date;
	bool relative = false, makeTime = false;

	// ToDo: only call time() if necessary
	if (now == -1)
		now = time(NULL);

	memset(&tm, 0, sizeof(tm));
//	localtime_r(&now, &tm);
//	printf("now = %ld, -> %ld\n", now, mktime(&tm));

	while (element->type != TYPE_END) {
		// skip whitespace
		while (isspace(format[0]))
			format++;

		if (format[0] == '[' && format[2] == ']') {
			// does this optional parameter not match our date string?
			if (!optional[position]) {
				format += 3;
				position++;
				continue;
			}

			format++;
		}

		switch (element->value_type) {
			case VALUE_CHAR:
				// skip the single character
				break;

			case VALUE_NUMERICAL:
				switch (format[0]) {
					case 'd':
						tm.tm_mday = element->value;
						makeTime = true;
						break;
					case 'm':
						tm.tm_mon = element->value - 1;
						makeTime = true;
						break;
					case 'H':
					case 'I':
						tm.tm_hour = element->value;
						makeTime = true;
						break;
					case 'M':
						tm.tm_min = element->value;
						makeTime = true;
						break;
					case 'S':
						tm.tm_sec = element->value;
						makeTime = true;
						break;
					case 'y':
					case 'Y':
						tm.tm_year = element->value;
						if (tm.tm_year > 1900)
							tm.tm_year -= 1900;
						makeTime = true;
						break;
					case 'T':
						// ToDo:!
						computeRelativeUnit(*element, tm, now, makeTime, false);
						break;
				}
				break;

			case VALUE_STRING:
				switch (format[0]) {
					case 'a':	// weekday
					case 'A':
						break;
					case 'b':
					case 'B':
						break;
					case 'p':
						break;
					case 'z':	// time zone
					case 'Z':
						break;
					case 'T':
						computeRelativeUnit(*element, tm, now, makeTime, true);
						break;
				}
				break;
		}

		// format matched at this point, check next element
		format++;
		position++;
		element++;
	}

	if (makeTime)
		return mktime(&tm);

	return now;
}


time_t
parsedate_etc(const char *dateString, time_t now, int *_flags)
{
	// preparse date string so that it can be easily compared to our formats
	
	parsed_element elements[MAX_ELEMENTS];

	if (preparseDate(dateString, elements) < B_OK) {
		*_flags = PARSEDATE_INVALID_DATE;
		return B_ERROR;
	}

	for (int32 index = 0; elements[index].type != TYPE_END; index++) {
		parsed_element e = elements[index];
		char type[32];
		sprintf(type, "0x%lx", e.type);
		if (e.type & TYPE_UNIT)
			strcat(type, " unit");
		if (e.type & TYPE_RELATIVE)
			strcat(type, " relative");

		printf("  %ld: type = %ld (%s), value = %Ld (%s)\n",
			index, e.type & TYPE_MASK, type, e.value,
			e.value_type == VALUE_NUMERICAL ? "numerical" :
			(e.value_type == VALUE_STRING ? "string" : "char"));
	}

	bool optional[MAX_ELEMENTS];

	for (int32 index = 0; sFormatsTable[index]; index++) {
		// test if this format matches our date string
		
		const char *format = sFormatsTable[index];
		uint32 position = 0;

		parsed_element *element = elements;
		while (element->type != TYPE_END) {
			// skip whitespace
			while (isspace(format[0]))
				format++;

			if (format[0] == '[' && format[2] == ']') {
				optional[position] = true;
				format++;
			} else
				optional[position] = false;

			switch (element->value_type) {
				case VALUE_CHAR:
					// check the allowed single characters

					switch (element->type & TYPE_MASK) {
						case TYPE_DOT:
							if (format[0] != '.')
								goto next_format;
							break;
						case TYPE_DASH:
							if (format[0] != '/' && format[0] != '-')
								goto next_format;
							break;
						case TYPE_COMMA:
							if (format[0] != ',')
								goto next_format;
							break;
						case TYPE_COLON:
							if (format[0] != ':')
								goto next_format;
							break;
						default:
							goto next_format;
					}
					break;

				case VALUE_NUMERICAL:
					switch (format[0]) {
						case 'd':
							if (element->value > 31)
								goto next_format;
							break;
						case 'm':
							if (element->value > 12)
								goto next_format;
							break;
						case 'H':
						case 'I':
							if (element->value > 24)
								goto next_format;
							break;
						case 'M':
						case 'S':
							if (element->value > 59)
								goto next_format;
							break;
						case 'y':
						case 'Y':
						case 'T':
							break;
						default:
							goto next_format;
					}
					break;
				
				case VALUE_STRING:
					switch (format[0]) {
						case 'a':	// weekday
						case 'A':
							if (!isType(element->type, TYPE_WEEKDAY))
								goto next_format;
							break;
						case 'b':	// month
						case 'B':
							if (!isType(element->type, TYPE_MONTH))
								goto next_format;
							break;
						case 'p':	// meridian
							if (!isType(element->type, TYPE_MERIDIAN))
								goto next_format;
							break;
						case 'z':	// time zone
						case 'Z':
							if (!isType(element->type, TYPE_TIME_ZONE))
								goto next_format;
							break;
						case 'T':
							// last/next/this
							break;
						default:
							goto next_format;
					}
					break;
			}

			// format matched at this point, check next element
			if (optional[position])
				format++;
			format++;
			position++;
			element++;
			continue;

		next_format:
			// format didn't match element - let's see if the current
			// one is only optional (in which case we can continue)
			if (!optional[position])
				goto skip_format;
				
			optional[position] = false;
			format += 2;
			position++;
				// skip the closing ']'
		}

		// check if the format is already empty (since we reached our last element)
		while (format[0]) {
			if (format[0] == '[')
				format += 3;
			else if (isspace(format[0]))
				format++;
			else
				break;
		}
		if (format[0])
			goto skip_format;

		// made it here? then we seem to have found our guy

		return computeDate(sFormatsTable[index], optional, elements, now, _flags);

	skip_format:
		// check if the next format has the same beginning as the skipped one,
		// and if so, skip that one, too.

		int32 length = format + 1 - sFormatsTable[index];

		while (sFormatsTable[index + 1]
			&& !strncmp(sFormatsTable[index], sFormatsTable[index + 1], length))
			index++;
	}

	// didn't find any matching formats
	return B_ERROR;
}


time_t
parsedate(const char *dateString, time_t now)
{
	int flags = 0;
	
	return parsedate_etc(dateString, now, &flags);
}


void
set_dateformats(const char **table)
{
	sFormatsTable = table ? table : kFormatsTable;
}


const char **
get_dateformats(void)
{
	return const_cast<const char **>(sFormatsTable);
}

