/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include <MediaFormats.h>
#include <stdio.h>


int 
main(int argc, char **argv)
{
	media_format_description a;
	a.family = B_AVI_FORMAT_FAMILY;
	a.u.avi.codec = 'DIVX';

	media_format_description b;
	printf("avi/divx == empty? %s\n", a == b ? "yes" : "no");

	b.family = a.family;
	printf("avi/divx == same family, no codec? %s\n", a == b ? "yes" : "no");

	a.family = B_QUICKTIME_FORMAT_FAMILY;
	a.u.quicktime.vendor = 5;
	a.u.quicktime.codec = 5;
	
	b.family = B_QUICKTIME_FORMAT_FAMILY;
	b.u.quicktime.vendor = 6;
	b.u.quicktime.codec = 5;
	printf("qt(5,5) < qt(6, 5)? %s\n", a < b ? "yes" : "no");

	b.u.quicktime.vendor = 4;
	printf("qt(5,5) < qt(4, 5)? %s\n", a < b ? "yes" : "no");

	b.u.quicktime.vendor = 5;
	b.u.quicktime.codec = 6;
	printf("qt(5,5) < qt(5, 6)? %s\n", a < b ? "yes" : "no");

	b.u.quicktime.codec = 4;
	printf("qt(5,5) < qt(5, 4)? %s\n", a < b ? "yes" : "no");

	a.family = B_MISC_FORMAT_FAMILY;
	a.u.misc.file_format = 5;
	a.u.misc.codec = 5;
	
	b.family = B_MISC_FORMAT_FAMILY;
	b.u.misc.file_format = 6;
	b.u.misc.codec = 5;
	printf("misc(5,5) < misc(6, 5)? %s\n", a < b ? "yes" : "no");

	b.u.misc.file_format = 4;
	printf("misc(5,5) < qt(4, 5)? %s\n", a < b ? "yes" : "no");

	b.u.misc.file_format = 5;
	b.u.misc.codec = 6;
	printf("misc(5,5) < misc(5, 6)? %s\n", a < b ? "yes" : "no");

	b.u.misc.codec = 4;
	printf("misc(5,5) < misc(5, 4)? %s\n", a < b ? "yes" : "no");

	return 0;
}
