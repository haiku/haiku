#include "posix_spawn_pipe_test.h"

#include <stdio.h>
#include <string.h>

 int main() {
  puts(testOut);
  fputs(testErr, stderr);
  return 0;
}
