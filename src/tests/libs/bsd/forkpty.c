/* from http://jwz.livejournal.com/817438.html */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pty.h>

static int
do_fork(void)
{
  int fd = -1;
  pid_t pid;

  if ((pid = forkpty(&fd, NULL, NULL, NULL)) < 0)
    perror("forkpty");
  else if (!pid)
    {
      printf ("0123456789\n");

      /* #### Uncommenting this makes it work! */
      /* sleep(20); */

      exit (0);
    }

  return fd;
}


int
main (int argc, char **argv)
{
  char s[1024];
  int n;
  int fd = do_fork();

  /* On 10.4, this prints the whole 10 character line, 1 char per second.
     On 10.5, it prints 1 character and stops.
   */
  do {
    n = read (fd, s, 1);
    if (n > 0) fprintf (stderr, "%c", *s);
    sleep (1);
  } while (n > 0);

  return 0;
}

