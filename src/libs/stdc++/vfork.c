/* Emulate vfork using just plain fork, for systems without a real vfork.
   This function is in the public domain. */

#include <unistd.h>

pid_t
vfork (void)
{
  return (fork ());
}
