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


#define TRACE_PARSEDATE 0
#if TRACE_PARSEDATE
#	define TRACE(x) printf x ;
#else
#	define TRACE(x) ;
#endif


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
	TYPE_UNKNOWN	= 0,

	TYPE_DAY,
	TYPE_MONTH,
	TYPE_YEAR,
	TYPE_WEEKDAY,
	TYPE_HOUR,
	TYPE_MINUTE,
	TYPE_SECOND,
	TYPE_TIME_ZONE,
	TYPE_MERIDIAN,

	TYPE_DASH,
	TYPE_DOT,
	TYPE_COMMA,
	TYPE_COLON,

	TYPE_UNIT,
	TYPE_MODIFIER,
	TYPE_END,
};

#define FLAG_NONE				0
#define FLAG_RELATIVE			1
#define FLAG_NOT_MODIFIABLE		2
#define FLAG_NOW				4
#define FLAG_NEXT_LAST_THIS		8
#define FLAG_PLUS_MINUS			16
#define FLAG_HAS_DASH			32

enum units {
	UNIT_NONE,
	UNIT_YEAR,
	UNIT_MONTH,
	UNIT_DAY,
	UNIT_SECOND,
};

enum value_type {
	VALUE_NUMERICAL,
	VALUE_STRING,
	VALUE_CHAR,
};

enum value_modifier {
	MODIFY_MINUS	= -2,
	MODIFY_LAST		= -1,
	MODIFY_NONE		= 0,
	MODIFY_THIS		= MODIFY_NONE,
	MODIFY_NEXT		= 1,
	MODIFY_PLUS		= 2,
};

struct known_identifier {
	const char	*string;
	const char	*alternate_string;
	uint8		type;
	uint8		flags;
	uint8		unit;
	int32		value;
};

static const known_identifier kIdentifiers[] = {
	{"today",		NULL,	TYPE_UNIT, FLAG_RELATIVE | FLAG_NOT_MODIFIABLE, UNIT_DAY, 0},
	{"tomorrow",	NULL,	TYPE_UNIT, FLAG_RELATIVE | FLAG_NOT_MODIFIABLE, UNIT_DAY, 1},
	{"yesterday",	NULL,	TYPE_UNIT, FLAG_RELATIVE | FLAG_NOT_MODIFIABLE, UNIT_DAY, -1},
	{"now",			NULL,	TYPE_UNIT, FLAG_RELATIVE | FLAG_NOT_MODIFIABLE | FLAG_NOW, 0},

	{"this",		NULL,	TYPE_MODIFIER, FLAG_NEXT_LAST_THIS, UNIT_NONE, MODIFY_THIS},
	{"next",		NULL,	TYPE_MODIFIER, FLAG_NEXT_LAST_THIS, UNIT_NONE, MODIFY_NEXT},
	{"last",		NULL,	TYPE_MODIFIER, FLAG_NEXT_LAST_THIS, UNIT_NONE, MODIFY_LAST},

	{"years",		"year",	TYPE_UNIT, FLAG_RELATIVE, UNIT_YEAR, 1},
	{"months",		"month",TYPE_UNIT, FLAG_RELATIVE, UNIT_MONTH, 1},
	{"weeks",		"week",	TYPE_UNIT, FLAG_RELATIVE, UNIT_DAY, 7},
	{"days",		"day",	TYPE_UNIT, FLAG_RELATIVE, UNIT_DAY, 1},
	{"hour",		NULL,	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 1 * 60 * 60},
	{"hours",		"hrs",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 1 * 60 * 60},
	{"second",		"sec",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 1},
	{"seconds",		"secs",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 1},
	{"minute",		"min",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 60},
	{"minutes",		"mins",	TYPE_UNIT, FLAG_RELATIVE, UNIT_SECOND, 60},

	{"am",			NULL,	TYPE_MERIDIAN, FLAG_NOT_MODIFIABLE, UNIT_SECOND, 0},
	{"pm",			NULL,	TYPE_MERIDIAN, FLAG_NOT_MODIFIABLE, UNIT_SECOND, 12 * 60 * 60},

	{"sunday",		"sun",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 0},
	{"monday",		"mon",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 1},
	{"tuesday",		"tue",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 2},
	{"wednesday",	"wed",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 3},
	{"thursday",	"thu",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 4},
	{"friday",		"fri",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 5},
	{"saturday",	"sat",	TYPE_WEEKDAY, FLAG_NONE, UNIT_DAY, 6},

	{"january",		"jan",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 1},
	{"february",	"feb", 	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 2},
	{"march",		"mar",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 3},
	{"april",		"apr",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 4},
	{"may",			"may",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 5},
	{"june",		"jun",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 6},
	{"july",		"jul",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 7},
	{"august",		"aug",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 8},
	{"september",	"sep",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 9},
	{"october",		"oct",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 10},
	{"november",	"nov",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 11},
	{"december",	"dec",	TYPE_MONTH, FLAG_NONE, UNIT_MONTH, 12},

	{"GMT",			NULL,	TYPE_TIME_ZONE,	FLAG_NONE, UNIT_SECOND, 0},
		// ToDo: add more time zones

	{NULL}
};

#define MAX_ELEMENTS	32

class DateMask {
	public:
		DateMask() : fMask(0UL) {}

		void Set(uint8 type) { fMask |= Flag(type); }
		bool IsSet(uint8 type) { return fMask & Flag(type); }

		bool HasTime();
		bool IsComplete();

	private:
		inline uint32 Flag(uint8 type) { return 1UL << type; }

		uint32	fMask;
};


struct parsed_element {
	uint8		base_type;
	uint8		type;
	uint8		flags;
	uint8		unit;
	uint8		value_type;
	int8		modifier;
	bigtime_t	value;
	
	void SetCharType(uint8 fieldType, int8 modify = MODIFY_NONE);

	void Adopt(const known_identifier &identifier);
	void AdoptUnit(const known_identifier &identifier);
	bool IsNextLastThis();
};


void 
parsed_element::SetCharType(uint8 fieldType, int8 modify)
{
	base_type = type = fieldType;
	value_type = VALUE_CHAR;
	modifier = modify;
}


void 
parsed_element::Adopt(const known_identifier &identifier)
{
	base_type = type = identifier.type;
	flags = identifier.flags;
	unit = identifier.unit;
	
	if (identifier.type == TYPE_MODIFIER)
		modifier = identifier.value;

	value_type = VALUE_STRING;
	value = identifier.value;
}


void 
parsed_element::AdoptUnit(const known_identifier &identifier)
{
	base_type = type = TYPE_UNIT;
	flags = identifier.flags;
	unit = identifier.unit;
	value *= identifier.value;
}


inline bool
parsed_element::IsNextLastThis()
{
	return base_type == TYPE_MODIFIER
		&& (modifier == MODIFY_NEXT || modifier == MODIFY_LAST || modifier == MODIFY_THIS);
}


//	#pragma mark -


bool 
DateMask::HasTime()
{
	// this will cause 
	return IsSet(TYPE_HOUR);
}


/** This method checks if the date mask is complete in the
 *	sense that it doesn't need to have a prefilled "struct tm"
 *	when its time value is computed.
 */

bool 
DateMask::IsComplete()
{
	// mask must be absolute, at last
	if (fMask & Flag(TYPE_UNIT))
		return false;

	// minimal set of flags to have a complete set
	return !(~fMask & (Flag(TYPE_DAY) | Flag(TYPE_MONTH)));
}


//	#pragma mark -


status_t
preparseDate(const char *dateString, parsed_element *elements)
{
	int32 index = 0, modify = MODIFY_NONE;
	char c;

	if (dateString == NULL)
		return B_ERROR;

	memset(&elements[0], 0, sizeof(parsed_element));

	for (; (c = dateString[0]) != '\0'; dateString++) {
		// we don't care about spaces
		if (isspace(c)) {
			modify = MODIFY_NONE;
			continue;
		}

		// if we're reached our maximum number of elements, bail out
		if (index >= MAX_ELEMENTS)
			return B_ERROR;

		if (c == ',') {
			elements[index].SetCharType(TYPE_COMMA);
		} else if (c == '.') {
			elements[index].SetCharType(TYPE_DOT);
		} else if (c == '/') {
			// "-" is handled differently (as a modifier)
			elements[index].SetCharType(TYPE_DASH);
		} else if (c == ':') {
			elements[index].SetCharType(TYPE_COLON);
		} else if (c == '+') {
			modify = MODIFY_PLUS;

			// this counts for the next element
			continue;
		} else if (c == '-') {
			modify = MODIFY_MINUS;
			elements[index].flags = FLAG_HAS_DASH;

			// this counts for the next element
			continue;
		} else if (isdigit(c)) {
			// fetch whole number

			elements[index].type = TYPE_UNKNOWN;
			elements[index].value_type = VALUE_NUMERICAL;
			elements[index].value = atoll(dateString);
			elements[index].modifier = modify;

			// skip number
			while (isdigit(dateString[1]))
				dateString++;
			
			// check for "1st", "2nd, "3rd", "4th", ...
			
			const char *suffixes[] = {"th", "st", "nd", "rd"};
			const char *validSuffix = elements[index].value > 3 ? "th" : suffixes[elements[index].value];
			if (!strncasecmp(dateString + 1, validSuffix, 2)
				&& !isalpha(dateString[3])) {
				// for now, just ignore the suffix - but we might be able
				// to deduce some meaning out of it, since it's not really
				// possible to put it in anywhere
				dateString += 2;
			}
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

			if (index > 0 && identifier->type == TYPE_UNIT) {
				// this is just a unit, so it will give the last value a meaning

				if (elements[--index].value_type != VALUE_NUMERICAL
					&& !elements[index].IsNextLastThis())
					return B_ERROR;

				elements[index].AdoptUnit(*identifier);
			} else if (index > 0 && elements[index - 1].IsNextLastThis()) {
				if (identifier->type == TYPE_MONTH
					|| identifier->type == TYPE_WEEKDAY) {
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
					elements[index].Adopt(*identifier);
					elements[index].type = TYPE_UNIT;
				} else
					return B_ERROR;
			} else {
				elements[index].Adopt(*identifier);
			}
		}

		// see if we can join any preceding modifiers

		if (index > 0
			&& elements[index - 1].type == TYPE_MODIFIER
			&& (elements[index].flags & FLAG_NOT_MODIFIABLE) == 0) {
			// copy the current one to the last and go on
			elements[index].modifier = elements[index - 1].modifier;
			elements[index].value *= elements[index - 1].value;
			elements[index].flags |= elements[index - 1].flags;
			elements[index - 1] = elements[index];
		} else {
			// we filled out one parsed_element
			index++;
		}

		memset(&elements[index], 0, sizeof(parsed_element));
	}

	// were there any elements?
	if (index == 0)
		return B_ERROR;

	elements[index].type = TYPE_END;

	return B_OK;
}


static void
computeRelativeUnit(parsed_element &element, struct tm &tm, int *_flags)
{
	// set the relative start depending on unit

	switch (element.unit) {
		case UNIT_YEAR:
			tm.tm_mon = 0;	// supposed to fall through
		case UNIT_MONTH:
			tm.tm_mday = 1;	// supposed to fall through
		case UNIT_DAY:
			tm.tm_hour = 0;
			tm.tm_min = 0;
			tm.tm_sec = 0;
			break;
	}

	// adjust value

	if (element.flags & FLAG_RELATIVE) {
		if (element.unit == UNIT_MONTH)
			tm.tm_mon += element.value;
		else if (element.unit == UNIT_DAY)
			tm.tm_mday += element.value;
		else if (element.unit == UNIT_SECOND) {
			if (element.modifier == MODIFY_MINUS)
				tm.tm_sec -= element.value;
			else
				tm.tm_sec += element.value;

			*_flags |= PARSEDATE_MINUTE_RELATIVE_TIME;
		} else if (element.unit == UNIT_YEAR)
			tm.tm_year += element.value;
	} else if (element.base_type == TYPE_WEEKDAY) {
		tm.tm_mday += element.value - tm.tm_wday;

		if (element.modifier == MODIFY_NEXT)
			tm.tm_mday += 7;
		else if (element.modifier == MODIFY_LAST)
			tm.tm_mday -= 7;
	} else if (element.base_type == TYPE_MONTH) {
		tm.tm_mon = element.value - 1;

		if (element.modifier == MODIFY_NEXT)
			tm.tm_year++;
		else if (element.modifier == MODIFY_LAST)
			tm.tm_year--;
	}
}


/**	Uses the format assignment (through "format", and "optional") for the parsed elements
 *	and calculates the time value with respect to "now".
 *	Will also set the day/minute relative flags in "_flags".
 */

static time_t
computeDate(const char *format, bool *optional, parsed_element *elements, time_t now, DateMask dateMask, int *_flags)
{
	TRACE(("matches: %s\n", format));

	parsed_element *element = elements;
	uint32 position = 0;
	struct tm tm;

	if (now == -1)
		now = time(NULL);

	if (dateMask.IsComplete())
		memset(&tm, 0, sizeof(tm));
	else {
		localtime_r(&now, &tm);

		if (dateMask.HasTime()) {
			tm.tm_min = 0;
			tm.tm_sec = 0;
		}

		*_flags = PARSEDATE_RELATIVE_TIME;
	}

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
						break;
					case 'm':
						tm.tm_mon = element->value - 1;
						break;
					case 'H':
					case 'I':
						tm.tm_hour = element->value;
						break;
					case 'M':
						tm.tm_min = element->value;
						break;
					case 'S':
						tm.tm_sec = element->value;
						break;
					case 'y':
					case 'Y':
						tm.tm_year = element->value;
						if (tm.tm_year > 1900)
							tm.tm_year -= 1900;
						break;
					case 'T':
						computeRelativeUnit(*element, tm, _flags);
						break;
					case '-':
						// there is no TYPE_DASH element for this (just a flag)
						format++;
						continue;
				}
				break;

			case VALUE_STRING:
				switch (format[0]) {
					case 'a':	// weekday
					case 'A':
						// we'll apply this element later, if still necessary
						if (!dateMask.IsComplete())
							computeRelativeUnit(*element, tm, _flags);
						break;
					case 'b':	// month
					case 'B':
						tm.tm_mon = element->value - 1;
						break;
					case 'p':	// meridian
						tm.tm_sec += element->value;
						break;
					case 'z':	// time zone
					case 'Z':
						tm.tm_sec += element->value - timezone;
						break;
					case 'T':	// time unit
						if (element->flags & FLAG_NOW) {
							*_flags = PARSEDATE_MINUTE_RELATIVE_TIME | PARSEDATE_RELATIVE_TIME;
							break;
						}

						computeRelativeUnit(*element, tm, _flags);
						break;
				}
				break;
		}

		// format matched at this point, check next element
		format++;
		if (format[0] == ']')
			format++;

		position++;
		element++;
	}

	return mktime(&tm);
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

#if TRACE_PARSEDATE
	for (int32 index = 0; elements[index].type != TYPE_END; index++) {
		parsed_element e = elements[index];

		printf("  %ld: type = %ld, base_type = %ld, unit = %ld, flags = %ld, value = %Ld (%s)\n",
			index, e.type, e.base_type, e.unit, e.flags, e.value,
			e.value_type == VALUE_NUMERICAL ? "numerical" :
			(e.value_type == VALUE_STRING ? "string" : "char"));
	}
#endif

	bool optional[MAX_ELEMENTS];

	for (int32 index = 0; sFormatsTable[index]; index++) {
		// test if this format matches our date string
		
		const char *format = sFormatsTable[index];
		uint32 position = 0;
		DateMask dateMask;

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

					switch (element->type) {
						case TYPE_DOT:
							if (format[0] != '.')
								goto next_format;
							break;
						case TYPE_DASH:
							if (format[0] != '-')
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
					// make sure that unit types are respected
					if (element->type == TYPE_UNIT && format[0] != 'T')
						goto next_format;

					switch (format[0]) {
						case 'd':
							if (element->value > 31)
								goto next_format;
							
							dateMask.Set(TYPE_DAY);
							break;
						case 'm':
							if (element->value > 12)
								goto next_format;

							dateMask.Set(TYPE_MONTH);
							break;
						case 'H':
						case 'I':
							if (element->value > 24)
								goto next_format;

							dateMask.Set(TYPE_HOUR);
							break;
						case 'M':
							dateMask.Set(TYPE_MINUTE);
						case 'S':
							if (element->value > 59)
								goto next_format;

							break;
						case 'y':
						case 'Y':
							// accept all values
							break;
						case 'T':
							dateMask.Set(TYPE_UNIT);
							break;
						case '-':
							if (element->flags & FLAG_HAS_DASH) {
								element--;	// consider this element again
								break;
							}
							// supposed to fall through
						default:
							goto next_format;
					}
					break;
				
				case VALUE_STRING:
					switch (format[0]) {
						case 'a':	// weekday
						case 'A':
							if (element->type != TYPE_WEEKDAY)
								goto next_format;
							break;
						case 'b':	// month
						case 'B':
							if (element->type != TYPE_MONTH)
								goto next_format;

							dateMask.Set(TYPE_MONTH);
							break;
						case 'p':	// meridian
							if (element->type != TYPE_MERIDIAN)
								goto next_format;
							break;
						case 'z':	// time zone
						case 'Z':
							if (element->type != TYPE_TIME_ZONE)
								goto next_format;
							break;
						case 'T':	// time unit
							if (element->type != TYPE_UNIT)
								goto next_format;

							dateMask.Set(TYPE_UNIT);
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

		return computeDate(sFormatsTable[index], optional, elements, now, dateMask, _flags);

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

