// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//  Copyright (c) 2001-2003, OpenBeOS
//
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//
//  File:        printf.c
//  Author:      Daniel Reinhold (danielre@users.sf.net)
//  Description: command line version of printf
//
//  Comments:
//  [6 Oct 2002]
//  This utility was written using the following document for reference:
//
//  http://www.opengroup.org/onlinepubs/007908799/xcu/printf.html
//
//  (which is an online Unix man page) and by testing the output against
//  the BeOS R5 program '/bin/printf'. There are a few deviations in the
//  output between the two, but they are so minor and technical that I
//  don't think any normal human being would care ;-).
//
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


void do_printf (int, char **);
int  esc_string_len(char *, bool *);
char escaped (char, char **);
int  get_decimal (char **);
char get_next_specifier(char *, int);
void get_spec_fields(char *, bool *, int *, int *);
bool parse_hex (char **, int, int, char *);
bool parse_octal (char **, int, int, char *);
void print_escaped_string (char *, char *);
void print_next_arg (void);


typedef union
{
	int    n;
	double d;
} number;

number        string_to_number(char *, bool);
inline int    string_to_decimal(char *);
inline double string_to_real(char *);



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// globals

// the variables below don't really have to be global,
// but it sure simplifies the code this way (trust me!)

static char  *Format;	// format string
static char **Argv;		// the list of args to be printed
static int    Argc;		// arg count


// a kludgy (but useful) value to encode a "\c" escape
static const char SLASHC = 1;




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// top-level


int
main(int argc, char *argv[])
{
	if (argc < 2)
		printf("Usage: printf format [arguments]\n");
	else
		do_printf(argc, argv);
	
	return 0;
}


void
do_printf(int argc, char *argv[])
{
	char c;
	char e;
	
	// the actual print arguments start at argv[2]
	Argv = argv + 2;
	Argc = argc - 2;

	for (;;) {
		Format = argv[1];
		
		// inner loop: interpret the format string
		while ((c = *Format++) != 0) {
			if (c == '%') {
				if (*Format == '%')
					++Format, putchar('%');
				else
					print_next_arg();
			} else {
				e = escaped(c, &Format);
				putchar(e);
			}
		}

		if (Argc == 0)
			return;
		
		// for any args remaining, just cycle thru again
		// re-using the previous format string...
	}
}


void
print_next_arg()
{
	// prints the next argument using the current specifier.
	// an empty string ("") is used for the output arg
	// when there are no more arguments remaining
	
	char  fmt[64];   // format specifier for a single arg
	char  fc;        // format character
	char *arg = "";  // next arg to be printed

	if (Argc > 0)
		arg = *Argv++, Argc--;
	
	fc = get_next_specifier(fmt, sizeof fmt);
	switch (fc) {		
		case 'c':
			printf(fmt, arg[0]);
			break;
		
		case 's':
			printf(fmt, arg);
			break;
		
		case 'b':
			// fmt+1 ==> skips over the unneeded '%' char
			print_escaped_string(fmt+1, arg);
			break;

		case 'd': case 'i': case 'o': case 'u':
		case 'x': case 'X': case 'p':
			printf(fmt, string_to_decimal(arg));
			break;
		
		case 'f': case 'g': case 'G': case 'e': case 'E':
			printf(fmt, string_to_real(arg));
			break;

		default:
			fprintf(stderr, "printf: `%c': illegal format character\n", escaped(fc, &Format));
			exit(1);
	}
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// parsing, character processing...


char
get_next_specifier(char *spec_buffer, int buflen)
{
	// called whenever a '%' is found in the format string.
	// extracts the next format specifier from the global format string
	// (which is advanced) and copies it into the given buffer.
	// the final format character is returned

	char *p = spec_buffer;
	char  c;
	char  fc  = 0;
	int   len = 1;	// will copy in the '%' regardless...

	*p++ = '%';
	while ((c = *Format++) != 0) {
		*p++ = c;
		if (++len > buflen)
			break;
		
		if (isalpha(c) || c == '%' || c == '\\') {
			fc = c;
			break;
		}
	}
	*p = 0;
	
	if (fc == 0) {
		fprintf(stderr, "printf: `%%': missing format character\n");
		exit(1);
	}
	
	return fc;
}


int
esc_string_len(char *s, bool *halt)
{
	// returns the string length that the string would have
	// if all (of any) embedded escape sequences were converted
	// e.g. "x\ty\tz\n" ==> 6
	//
	// note: the special '\c' escape char is not counted,
	// but instead terminates the string and sets the halt flag
	
	char c;
	char e;
	int  len = 0;
	
	while ((c = *s++) != 0) {
		e = escaped(c, &s);
		if (e == SLASHC)
			break;
		
		++len;
	}
	
	*halt = (e == SLASHC);
	return len;
}


void
print_escaped_string(char *fmt, char *s)
{
	// called whenever the "%b" format specifier is found.
	// prints a string possibly containing escaped chars
	// (precision field holds the number of chars requested)
	//
	// this is the only place where the "\c" escape sequence
	// is to be interpreted (i.e. the program halted)
	
	int  pad, lpad, rpad;  // count of padding chars
	bool leftAdjust;       // left/right adjust?
	int  fwidth;           // specified field width
	int  wanted;           // requested number of chars
	int  len;              // number of chars to actually print
	bool halt = false;
	
	get_spec_fields(fmt, &leftAdjust, &fwidth, &wanted);
	
	// compute the string length and padding
	len = esc_string_len(s, &halt);	
	if (wanted > 0 && wanted < len)
		len = wanted;
	
	pad = fwidth - len;
	if (leftAdjust)
		lpad = 0,   rpad = pad;
	else
		lpad = pad, rpad = 0;
	
	// print the translated chars with padding
	{
		char c;
		char e;
	
		while (lpad-- > 0) putchar(' ');	// pad on the left
		while (len--  > 0) {
			c = *s++;
			e = escaped(c, &s);
			putchar(e);
		}
		while (rpad-- > 0) putchar(' ');	// pad on the right
	}
	
	if (halt)
		exit(0);
}


void
get_spec_fields(char *specifier, bool *leftAdjust, int *fieldwidth, int *precision)
{
	// parses the given specifier to determine:
	//   * left/right adjustment
	//   * field width
	//   * precision
	// these values are then returned thru the parameters
	// note: this routine is only called for the "%b" conversion
	
	const char *flags = " 0-+#";
	
	char *s = specifier;
	
	*leftAdjust = false;
	*fieldwidth = 0;
	*precision  = 0;
	
	while (strchr(flags, *s)) {
		if (*s == '-')
			// only flag we care about here
			*leftAdjust = true;
		++s;
	}
	
	if (isdigit (*s)) {
		*fieldwidth = get_decimal(&s);
		if (*s == '.')
			++s, *precision = get_decimal(&s);
	}
	
	if (!(isalpha(*s))) {
		fprintf(stderr, "printf: `%c': illegal format character\n", escaped(*s, &s));
		exit(1);
	}		
}


char
escaped(char c, char **src)
{
	// returns the character value for an escape sequence
	// if no escape is found, the original char is returned
	// the source pointer is advanced for any extra chars read
	
	char *p = *src;      // local pointer to scan thru the string
	char  esc;           // the converted character
	bool  found = true;  // assume that an escape will be found
	
	if (c != '\\')
		return c;

	switch (*p++) {
		case 'a':  esc = '\a'; break;
		case 'b':  esc = '\b'; break;
		case 'f':  esc = '\f'; break;
		case 'n':  esc = '\n'; break;
		case 'r':  esc = '\r'; break;
		case 't':  esc = '\t'; break;
		case 'v':  esc = '\v'; break;
		case '\\': esc = '\\'; break;
		case '"':  esc = '"';  break;
		case '\'': esc = '\''; break;
		
		case 'c':  esc = SLASHC; break;
		
		case '0':
			// format: "\0ooo" (0 to 3 octal digits)
			found = parse_octal(&p, 0, 3, &esc);
			break;
		
		case 'x':
			// format: "\xhhh" (1 to 3 hex digits)
			found = parse_hex(&p, 1, 3, &esc);
			break;
		
		default:
			// nope, not an escape sequence
			found = false;
	}

	if (found) {
		*src = p;	// update the source pointer
		return esc;
	}
	
	return c;
}




// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// numeric routines


int
get_decimal(char **src)
{
	// grabs a decimal integer value from the source string
	// and returns the full computed value.
	// if any valid digits were found, the source pointer is advanced.
	// otherwise, a default value of 0 is returned
	
	char *s = *src;
	char  c;
	int   n = 0;
	
	while (isdigit(c = *s)) {
		++s;
		n = n * 10 + (c - '0');
	}
	
	*src = s;
	return n;
}


#define isodigit(c) (isdigit(c) && !(c == '8' || c == '9'))

bool
parse_octal(char **src, int min_digits, int max_digits, char *octval)
{
	// searches for an octal value in the source string.
	// returns true if valid octal digits were found
	//
	// at least 'min_digits' and no more than 'max_digits'
	// of valid octal characters must be present
	//
	// on success, the source pointer is advanced and the
	// full octal value is returned in the last parameter.
	// otherwise, the pointer is left unchanged and the
	// value of the last parameter is undefined.

	char *s = *src;
	char  c;
	int   n = 0;
	int   i;
	
	for (i = 0; i < max_digits; ++i) {
		c = *s;
		if (isodigit(c)) {
			++s;
			n = n * 8 + (c - '0');
		}
		else
			break;
	}
	
	if (i >= min_digits) {
		*src = s;
		*octval = (char) n;
		return true;
	}
	
	return false;
}


bool
parse_hex(char **src, int min_digits, int max_digits, char *hexval)
{
	// searches for a hex value in the source string.
	// returns true if valid hex digits were found
	//
	// at least 'min_digits' and no more than 'max_digits'
	// of valid hex characters must be present
	//
	// on success, the source pointer is advanced and the
	// full hex value is returned in the last parameter.
	// otherwise, the pointer is left unchanged and the
	// value of the last parameter is undefined.
	
	char *s = *src;
	char  c;
	int   n = 0;
	int   i;
	
	for (i = 0; i < max_digits; ++i) {
		c = tolower(*s);
		if (isxdigit(c)) {
			++s;
			n *= 16;
			if (c > '9')
				n += (c - 'a' + 10);
			else
				n += (c - '0');
		}
		else
			break;
	}
	
	if (i >= min_digits) {
		*src = s;
		*hexval = (char) n;
		return true;
	}
	
	return false;
}


inline
int
string_to_decimal(char *numstr)
{
	return string_to_number(numstr, false).n;
}


inline
double
string_to_real(char *numstr)
{
	return string_to_number(numstr, true).d;
}


number
string_to_number(char *numstr, bool isreal)
{
	// converts a numeric string to a number value
	// (either a decimal integer or a floating point)
	//
	// since the code for the two types of conversion is very
	// nearly the same, this master function takes care of both,
	// albeit at the price of having to create a union type
	// to hold the return values
	//
	// invalid numeric formats should be caught here and cause
	// the program to abort with an error message
	
	char   *end;
	number  num;
	
	// check for a character integer format (e.g. 'A ==> 65)
	if (numstr[0] == '\"' || numstr[0] == '\'') {
		if (isreal)
			num.d = (double) numstr[1];
		else
			num.n = (int) numstr[1];
		return num;
	}
	
	// attempt to convert with a library function
	errno = 0;
	if (isreal)
		num.d = strtod(numstr, &end);
	else
		num.n = (int) strtol(numstr, &end, 10);
	
	if (errno) {
		fprintf(stderr, "printf: %s: %s\n", numstr, strerror(errno));
		exit(1);
	}
	
	if (*end) {
		fprintf(stderr, "printf: %s: illegal number\n", numstr);
		exit(1);
	}
	
	return num;
}

