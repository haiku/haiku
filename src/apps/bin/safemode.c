/* safemode.c - tells if safemode is active
 * (c) 2004, Jérôme DUVAL for Haiku
 * released under the MIT licence.
 *
 * ChangeLog:
 * 08-29-2004 v1.0
 *  Initial.
 * 
 * safemode
 */

#include <stdio.h>
#include <strings.h>
#include <SupportDefs.h>

// i don't know the exact signature but this one works
extern status_t _kget_safemode_option_(char* name, uint8 *p1, uint32 *p2);

int main(int argc, char **argv)
{
	uint8 p1;
	uint32 p2 = 1;
	status_t err;
	
	err = _kget_safemode_option_("safemode", &p1, &p2);
	if (err == B_OK) {
		printf("yes\n");
	} else
		printf("no\n");
	return 0;
}

