/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <SupportDefs.h>

#include <errno.h>

#include <monetary.h>
#include <locale.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>


// ToDo: implementation is not yet working!


enum strfmon_flags {
	USE_GROUPING		= 1,
	USE_SIGN			= 2,
	USE_LOCALE_POSNEG	= 4,
	NEG_IN_PARENTHESES	= 8,
	NO_CURRENCY			= 16,
	LEFT_JUSTIFIED		= 32,
	USE_INT_CURRENCY	= 64
};


static int32
parseNumber(const char **_format)
{
	const char *format = *_format;
	int32 number = 0;
	while (isdigit(*format))
		number = number * 10 + *format++ - '0';

	*_format = format;
	return number;
}


ssize_t
vstrfmon(char *string, size_t maxSize, const char *format, va_list args)
{
	if (format == NULL || string == NULL)
		return B_BAD_VALUE;

	struct lconv *lconv = localeconv();
	int32 length = 0;

	while (*format) {
		if (format[0] != '%' || *++format == '%') {
			if (++length >= (int32)maxSize)
				return E2BIG;

			*string++ = *format++;
			continue;
		}
		if (format[0] == '\0')
			return B_BAD_VALUE;

		char flags = USE_GROUPING | USE_LOCALE_POSNEG;
		char fill = ' ';
		int32 width = 0, leftPrecision = 0, rightPrecision = -1;
		bool isNegative = false;

		// flags
		int32 mode = 0;
		while (*format && mode == 0) {
			switch (*format++) {
				case '=':
					fill = *format++;
					if (fill == '\0')
						return B_BAD_VALUE;
					break;
				case '+':
					if (flags & USE_SIGN)
						return B_BAD_VALUE;
					flags |= USE_SIGN;
					break;
				case '(':
					if (flags & USE_SIGN)
						return B_BAD_VALUE;
					flags |= USE_SIGN | NEG_IN_PARENTHESES;
					break;
				case '^':
					flags &= ~USE_GROUPING;
					break;
				case '!':
					flags |= NO_CURRENCY;
					break;
				case '-':
					flags |= LEFT_JUSTIFIED;
					break;
				default:
					mode = 1;
			}
		}

		// width
		if (isdigit(*format))
			width = parseNumber(&format);

		// left precision
		if (*format == '#') {
			format++;
			if (!isdigit(*format))
				return B_BAD_VALUE;

			leftPrecision = parseNumber(&format);
		}

		// right precision
		if (*format == '.') {
			format++;
			if (!isdigit(*format))
				return B_BAD_VALUE;

			rightPrecision = parseNumber(&format);
		}

		// which currency symbol to use?
		switch (*format++) {
			case 'n':
				// national currency symbol is the default
				break;
			case 'i':
				flags |= USE_INT_CURRENCY;
				break;
			default:
				return B_BAD_VALUE;
		}

		// adjust the default right precision value according the currency symbol
		if (rightPrecision == -1) {
			rightPrecision = flags & USE_INT_CURRENCY ? lconv->int_frac_digits : lconv->frac_digits;
			if (rightPrecision == CHAR_MAX)
				rightPrecision = 2;
		}

		double value = va_arg(args,double);
		if (value < 0) {
			isNegative = true;
			value = -value;
		}

		if (leftPrecision + rightPrecision > 255)
			return B_BAD_VALUE;

		char number[256];
		sprintf(number, "%*.*f", (int)leftPrecision, (int)rightPrecision,
				value);

		if (leftPrecision >= 0) {
			
		}
	}
	return B_OK;
}


ssize_t
strfmon(char *string, size_t maxSize, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	
	ssize_t status = vstrfmon(string, maxSize, format, args);
	
	va_end(args);

	if (status < B_OK) {
		errno = status;
		return -1;
	}
	return status;
}

