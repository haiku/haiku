#include <OS.h>
#include <stdio.h>
#include <stdarg.h>

static sem_id errorPrinting=0;

void error(char *fmt, ...)
{
		if (errorPrinting==0)
			errorPrinting=create_sem(1,"error_printing");
		acquire_sem(errorPrinting);
        va_list argp;
		char tmp[2000];
        sprintf(tmp, "[%lld] error: %s",real_time_clock_usecs(),fmt);
        va_start(argp, tmp);
        vfprintf(stderr,tmp,argp);
        va_end(argp);
		release_sem(errorPrinting);
}
