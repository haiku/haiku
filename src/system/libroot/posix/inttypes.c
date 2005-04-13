/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <inttypes.h>
#include <stdlib.h>


intmax_t
imaxabs(intmax_t number)
{
	return number > 0 ? number : -number;
}


imaxdiv_t
imaxdiv(intmax_t numer, intmax_t denom)
{
	imaxdiv_t result;

	result.quot = numer / denom;
	result.rem = numer % denom;

	if (numer >= 0 && result.rem < 0) {
		result.quot++;
		result.rem -= denom;
	}

	return result;
}


intmax_t
strtoimax(const char *string, char **_end, int base)
{
	// ToDo: implement me properly!
	return (intmax_t)strtol(string, _end, base);
}


uintmax_t
strtoumax(const char *string, char **_end, int base)
{
	// ToDo: implement me properly!
	return (intmax_t)strtoul(string, _end, base);
}

