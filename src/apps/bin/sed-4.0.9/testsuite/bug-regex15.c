/* Test for memory/CPU leak in regcomp.  */

#include <error.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_DATA_LIMIT (32 << 20)

int
main ()
{
#ifdef RLIMIT_DATA
  regex_t re;
  int reerr;

  /* Try to avoid eating all memory if a test leaks.  */
  struct rlimit data_limit;
  if (getrlimit (RLIMIT_DATA, &data_limit) == 0)
    {
      if ((rlim_t) TEST_DATA_LIMIT > data_limit.rlim_max)
	data_limit.rlim_cur = data_limit.rlim_max;
      else if (data_limit.rlim_cur > (rlim_t) TEST_DATA_LIMIT)
	data_limit.rlim_cur = (rlim_t) TEST_DATA_LIMIT;
      if (setrlimit (RLIMIT_DATA, &data_limit) < 0)
	perror ("setrlimit: RLIMIT_DATA");
    }
  else
    perror ("getrlimit: RLIMIT_DATA");

  reerr = regcomp (&re, "^6?3?[25]?5?[14]*[25]*[69]*+[58]*87?4?$",
		   REG_EXTENDED | REG_NOSUB);
  if (reerr != 0)
    {
      char buf[100];
      regerror (reerr, &re, buf, sizeof buf);
      printf ("regerror %s\n", buf);
      return 1;
    }

  return 0;
#else
  return 77;
#endif
}
