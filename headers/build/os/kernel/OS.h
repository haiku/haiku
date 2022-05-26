// Some platforms (Linux) don't define PAGESIZE, set a default value in that
// case.
#include <limits.h>

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#include <../os/kernel/OS.h>
