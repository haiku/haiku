/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <stdlib.h>
#include <ctype.h>

static int hexval(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
		
	return 0;
}

int atoi(const char *num)
{
	int value = 0;
	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && isxdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		while (*num && isdigit(*num))
			value = value * 10 + *num++  - '0';
	}

	return value;
}

unsigned int atoui(const char *num)
{
	int value = 0;
	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && isxdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		while (*num && isdigit(*num))
			value = value * 10 + *num++  - '0';
	}

	return value;
}

long atol(const char *num)
{
	int value = 0;
	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && isxdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		while (*num && isdigit(*num))
			value = value * 10 + *num++  - '0';
	}

	return value;
}

unsigned long atoul(const char *num)
{
	int value = 0;
	if (num[0] == '0' && num[1] == 'x') {
		// hex
		num += 2;
		while (*num && isxdigit(*num))
			value = value * 16 + hexval(*num++);
	} else {
		// decimal
		while (*num && isdigit(*num))
			value = value * 10 + *num++  - '0';
	}

	return value;
}
