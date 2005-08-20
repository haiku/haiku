#include <OS.h>
#include "syscalls.h"


thread_id
find_thread(const char *name)
{
	return _kern_find_thread(name);
}
