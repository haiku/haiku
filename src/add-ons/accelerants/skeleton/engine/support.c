/* Some commmon support functions */
/* Mark Watson 2/2000;
 * Rudolf Cornelissen 1/2004 */

#define MODULE_BIT 0x00000800

#include <stdarg.h>
#include "std.h"

/*delays in multiple of microseconds*/
void delay(bigtime_t i)
{
	bigtime_t start=system_time();
	while(system_time()-start<i);
}

/*debug logging*/
void eng_log(char *fmt, ...)
{
	char     buffer[1024];
	char     fname[64];
	FILE    *myhand;
	va_list  args;

	sprintf (fname, "/boot/home/" DRIVER_PREFIX ".accelerant.%d.log", accelerantIsClone);
	myhand=fopen(fname,"a+");

	if (myhand == NULL) return;

	va_start(args,fmt);
	vsprintf (buffer, fmt, args);
	fprintf(myhand, "%s", buffer);
	fclose(myhand);
}
