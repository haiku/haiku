/*
 * Copyright 2003-2006, Axel Dörfler. All rights reserved.
 * Distributed under the terms of the MIT license.
 * 
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <SupportDefs.h>
#include <kernel.h>

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>


#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define LARGE	64		/* use 'ABCDEF' instead of 'abcdef' */

#define FLOATING_SUPPORT


static int
skip_atoi(const char **s)
{
	int i = 0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';

	return i;
}


static uint64
do_div(uint64 *_number, uint32 base) 
{ 
	uint64 result = *_number % (uint64)base;
	*_number = *_number / (uint64)base;

	return result;
}


static bool
put_padding(char **_buffer, int32 *_bytesLeft, int32 count)
{
	int32 left = *_bytesLeft;
	char *string = *_buffer;
	int32 written;
	bool allWritten;

	if (count <= 0)
		return true;
	if (left == 0)
		return false;

	if (left < count) {
		count = left;
		allWritten = false;
	} else
		allWritten = true;

	written = count;

	while (count-- > 0)
		*string++ = ' ';

	*_buffer = string;
	*_bytesLeft = left - written;

	return allWritten;
}


static inline bool
put_string(char **_buffer, int32 *_bytesLeft, const char *source, int32 length)
{
	int32 left = *_bytesLeft;
	char *target = *_buffer;
	bool allWritten;

	if (length == 0)
		return true;
	if (left == 0)
		return false;

	if (left < length) {
		length = left;
		allWritten = false;
	} else
		allWritten = true;

#if 0
	// take care of string arguments in user space
	if (CHECK_USER_ADDRESS(source)) {
		if (user_memcpy(target, source, length) < B_OK)
			return put_string(_buffer, _bytesLeft, "<FAULT>", 7);
	} else
#endif
		memcpy(target, source, length);

	*_bytesLeft = left - length;
	*_buffer = target + length;

	return allWritten;
}


static inline bool
put_character(char **_buffer, int32 *_bytesLeft, char c)
{
	int32 left = *_bytesLeft;
	char *string = *_buffer;

	if (left > 0) {
		*string++ = c;
		*_buffer = string;
		*_bytesLeft = left - 1;
		return true;
	}

	return false;
}


static void
number(char **_string, int32 *_bytesLeft, int64 num, uint32 base, int size,
	int precision, int type)
{
	const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	char c, sign, tmp[66];
	int i;

	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return;

	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		} else if (type & PLUS) {
			sign = '+';
			size--;
		} else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++] = '0';
	else while (num != 0)
		tmp[i++] = digits[do_div((uint64 *)&num, base)];

	if (i > precision)
		precision = i;
	size -= precision;

	if (!(type & (ZEROPAD + LEFT))) {
		put_padding(_string, _bytesLeft, size);
		size = 0;
	}
	if (sign)
		put_character(_string, _bytesLeft, sign);

	if (type & SPECIAL) {
		if (base == 8)
			put_character(_string, _bytesLeft, '0');
		else if (base == 16) {
			put_character(_string, _bytesLeft, '0');
			put_character(_string, _bytesLeft, digits[33]);
		}
	}

	if (!(type & LEFT)) {
		while (size-- > 0)
			put_character(_string, _bytesLeft, c);
	}
	while (i < precision--)
		put_character(_string, _bytesLeft, '0');
	while (i-- > 0)
		put_character(_string, _bytesLeft, tmp[i]);

	put_padding(_string, _bytesLeft, size);
}


#ifdef FLOATING_SUPPORT
/*!
	This is a very basic floating point to string conversion routine.
	It prints up to 3 fraction digits, and doesn't support any precision arguments.
	It's just here for your convenience so that you can use it for debug output.
*/
static bool
floating(char **_string, int32 *_bytesLeft, double value, int fieldWidth, int flags)
{
	char buffer[66];
	bool sign = value < 0.0;
	uint64 fraction;
	uint64 integer;
	int32 length = 0;

	if (sign)
		value = -value;

	fraction = (uint64)(value * 1000) % 1000;
	integer = (uint64)value;

	// put fraction part, if any

	if (fraction != 0) {
		bool first = true;
		while (fraction != 0) {
			int digit = do_div(&fraction, 10);
			if (!first || digit > 0) {
				buffer[length++] = '0' + digit;
				first = false;
			}
		}

		buffer[length++] = '.';
	}

	// put integer part

	if (integer == 0)
		buffer[length++] = '0';
	else while (integer != 0) {
		buffer[length++] = '0' + do_div(&integer, 10);
	}

	// write back to string

	if (!(flags & LEFT) && !put_padding(_string, _bytesLeft, fieldWidth))
		return false;

	while (length-- > 0) {
		if (!put_character(_string, _bytesLeft, buffer[length]))
			return false;
	}

	if ((flags & LEFT) != 0 && !put_padding(_string, _bytesLeft, fieldWidth))
		return false;

	return true;
}
#endif	// FLOATING_SUPPORT


int
vsnprintf(char *buffer, size_t bufferSize, const char *format, va_list args)
{
	int32 bytesLeft;
	uint64 num;
	int base;
	char *string;
	int flags;			/* flags to number() */
	int fieldWidth;	/* width of output field */
	int precision;		
		/* min. # of digits for integers; max number of chars for from string */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	if (bufferSize == 0)
		return 0;

	bytesLeft = ((int32)bufferSize - 1) & 0x7fffffff;
		// make space for the terminating '\0' byte, and we
		// only allow 2G characters :)

	for (string = buffer; format[0] && bytesLeft > 0; format++) {
		if (format[0] != '%') {
			if (!put_character(&string, &bytesLeft, format[0]))
				break;

			continue;
		}

		/* process flags */

		flags = 0;

	repeat:
		++format;		/* this also skips first '%' */
		switch (format[0]) {
			case '-': flags |= LEFT; goto repeat;
			case '+': flags |= PLUS; goto repeat;
			case ' ': flags |= SPACE; goto repeat;
			case '#': flags |= SPECIAL; goto repeat;
			case '0': flags |= ZEROPAD; goto repeat;
		}

		/* get field width */

		fieldWidth = -1;
		if (isdigit(*format))
			fieldWidth = skip_atoi(&format);
		else if (format[0] == '*') {
			++format;
			/* it's the next argument */
			fieldWidth = va_arg(args, int);
			if (fieldWidth < 0) {
				fieldWidth = -fieldWidth;
				flags |= LEFT;
			}
		}

		/* get the precision */

		precision = -1;
		if (format[0] == '.') {
			++format;	
			if (isdigit(*format))
				precision = skip_atoi(&format);
			else if (format[0] == '*') {
				++format;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */

		qualifier = -1;
		if (format[0] == 'h' || format[0] == 'L') {
			qualifier = *format++;
		} else if (format[0] == 'l')  {
			format++;
			if (format[0] == 'l') {
				qualifier = 'L';
				format++;
			} else
				qualifier = 'l';
		}

		/* default base */
		base = 10;

		switch (format[0]) {
			case 'c':
				if (!(flags & LEFT) && !put_padding(&string, &bytesLeft, fieldWidth - 1))
					goto out;

				put_character(&string, &bytesLeft, (char)va_arg(args, int));

				if ((flags & LEFT) != 0 && !put_padding(&string, &bytesLeft, fieldWidth - 1))
					goto out;
				continue;

			case 's':
			{
				const char *argument = va_arg(args, char *);
				int32 length;

				if (argument == NULL)
					argument = "<NULL>";

				length = strnlen(argument, precision);
				fieldWidth -= length;

				if (!(flags & LEFT) && !put_padding(&string, &bytesLeft, fieldWidth))
					goto out;

				if (!put_string(&string, &bytesLeft, argument, length))
					goto out;

				if ((flags & LEFT) != 0 && !put_padding(&string, &bytesLeft, fieldWidth))
					goto out;
				continue;
			}

#ifdef FLOATING_SUPPORT
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			{
				double value = va_arg(args, double);
				if (!floating(&string, &bytesLeft, value, flags, fieldWidth))
					goto out;
				continue;
			}
#endif	// FLOATING_SUPPORT

			case 'p':
				if (fieldWidth == -1) {
					fieldWidth = 2*sizeof(void *);
					flags |= ZEROPAD;
				}

				if (!put_string(&string, &bytesLeft, "0x", 2))
					goto out;
				number(&string, &bytesLeft, (uint32)va_arg(args, void *), 16,
						fieldWidth, precision, flags);
				continue;

			case 'n':
				if (qualifier == 'l') {
					long *ip = va_arg(args, long *);
					*ip = (string - buffer);
				} else {
					int *ip = va_arg(args, int *);
					*ip = (string - buffer);
				}
				continue;

			/* integer number formats - set up the flags and "break" */
			case 'o':
				base = 8;
				break;

			case 'X':
				flags |= LARGE;
			case 'x':
				base = 16;
				break;

			case 'd':
			case 'i':
				flags |= SIGN;
			case 'u':
				break;

			default:
				if (format[0] != '%')
					put_character(&string, &bytesLeft, '%');

				if (!format[0])
					goto out;

				put_character(&string, &bytesLeft, format[0]);
				continue;
		}

		if (qualifier == 'L')
			num = va_arg(args, uint64);
		else if (qualifier == 'l') {
			num = va_arg(args, uint32);
			if (flags & SIGN)
				num = (long)num;
		} else if (qualifier == 'h') {
			num = (unsigned short)va_arg(args, int);
			if (flags & SIGN)
				num = (short)num;
		} else if (flags & SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);

		number(&string, &bytesLeft, num, base, fieldWidth, precision, flags);
	}

out:
	*string = '\0';
	return string - buffer;
}


int
vsprintf(char *buffer, const char *format, va_list args)
{
	return vsnprintf(buffer, ~0UL, format, args);
}


int
snprintf(char *buffer, size_t bufferSize, const char *format, ...)
{
	va_list args;
	int i;

	va_start(args, format);
	i = vsnprintf(buffer, bufferSize, format, args);
	va_end(args);

	return i;
}


int 
sprintf(char *buffer, const char *format, ...)
{
	va_list args;
	int i;

	va_start(args, format);
	i = vsnprintf(buffer, ~0UL, format, args);
	va_end(args);

	return i;
}

