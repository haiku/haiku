/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <SupportDefs.h>
#include <kernel.h>

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>


#if 0
static unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((*cp == 'x') && isxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}

static long simple_strtol(const char *cp,char **endp,unsigned int base)
{
	if(*cp=='-')
		return -simple_strtoul(cp+1,endp,base);
	return simple_strtoul(cp,endp,base);
}
#endif


static int
skip_atoi(const char **s)
{
	int i = 0;

	while (isdigit(**s))
		i = i*10 + *((*s)++) - '0';

	return i;
}


#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define LARGE	64		/* use 'ABCDEF' instead of 'abcdef' */


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


static char *
number(char *str, long long num, unsigned base, int size, int precision, int type)
{
	const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	char c, sign, tmp[66];
	int i;

	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;

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
		tmp[i++] = digits[do_div(&num, base)];

	if (i > precision)
		precision = i;
	size -= precision;

	if (!(type&(ZEROPAD + LEFT))) {
		while (size-- > 0)
			*str++ = ' ';
	}
	if (sign)
		*str++ = sign;

	if (type & SPECIAL) {
		if (base == 8)
			*str++ = '0';
		else if (base == 16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}

	if (!(type & LEFT))
		while (size-- > 0)
			*str++ = c;
	while (i < precision--)
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';

	return str;
}


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

	if (buffer == NULL || bufferSize == 0)
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
		if (format[0] == 'h' || format[0] == 'l' || format[0] == 'L')
			qualifier = *format++;

		/* default base */
		base = 10;

		switch (format[0]) {
			case 'c':
				if (!(flags & LEFT) && !put_padding(&string, &bytesLeft, fieldWidth))
					goto out;

				put_character(&string, &bytesLeft, (char)va_arg(args, int));

				if ((flags & LEFT) != 0 && !put_padding(&string, &bytesLeft, fieldWidth))
					goto out;
				continue;

			case 's':
			{
				const char *argument = va_arg(args, char *);
				int32 length;

				if (argument == NULL)
					argument = "<NULL>";

				length = strnlen(argument, precision);

				if (!(flags & LEFT) && !put_padding(&string, &bytesLeft, fieldWidth))
					goto out;

				if (!put_string(&string, &bytesLeft, argument, length))
					goto out;

				if ((flags & LEFT) != 0 && !put_padding(&string, &bytesLeft, fieldWidth))
					goto out;
				continue;
			}

			case 'p':
				if (fieldWidth == -1) {
					fieldWidth = 2*sizeof(void *);
					flags |= ZEROPAD;
				}

				// ToDo: fix me!
				string[0] = '0';
				string[1] = 'x';
				string = number(string + 2, (uint32)va_arg(args, void *), 16,
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
			num = va_arg(args, unsigned long long);
		else if (qualifier == 'l') {
			num = va_arg(args, unsigned long);
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

		// ToDo: fix me!
		string = number(string, num, base, fieldWidth, precision, flags);
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

