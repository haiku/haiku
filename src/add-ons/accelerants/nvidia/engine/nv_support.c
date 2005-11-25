/* Some commmon support functions */
/* Mark Watson 2/2000;
 * Rudolf Cornelissen 1/2004-11/2005 */

#define MODULE_BIT 0x00000800

#include <stdarg.h>
#include "nv_std.h"

/*delays in multiple of microseconds*/
void delay(bigtime_t i)
{
	bigtime_t start=system_time();
	while(system_time()-start<i);
}

/*debug logging*/
void nv_log(char *fmt, ...)
{
	char     buffer[1024];
	char     fname[64];
	FILE    *myhand;
	va_list  args;

	/* determine the logfile name:
	 * we need split-up logging per card and instance of the accelerant */
	sprintf (fname, "/boot/home/" DRIVER_PREFIX "." DEVICE_FORMAT ".%d.log",
		si->vendor_id, si->device_id, si->bus, si->device, si->function,
		accelerantIsClone);
	myhand=fopen(fname,"a+");

	if (myhand == NULL) return;

	va_start(args,fmt);
	vsprintf (buffer, fmt, args);
	fprintf(myhand, "%s", buffer);
	fclose(myhand);
}
