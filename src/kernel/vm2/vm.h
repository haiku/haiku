#include "list.h"

#ifndef _VM_TYPES
#define _VM_TYPES
const int PAGE_SIZE = 4096;
struct vnode : public node
{
	int fd;
	unsigned long offset;
	bool valid;
	int count;

	vnode (void)
	{
	valid=false;
	count=0;
	}
};
typedef unsigned long owningProcess;
#define B_OS_NAME_LENGTH 32
enum protectType {none=0,readable, writable,copyOnWrite,symCopyOnWrite};
enum pageState {FULL,CONTIGUOUS,LAZY,NO_LOCK,LOMEM};
enum addressSpec {EXACT,BASE,ANY,ANY_KERNEL,CLONE};
#define USER_BASE 0x10000000
#define KERNEL_BASE 0x80000000
#define CACHE_BEGIN 0x90000000
#define CACHE_END 0xe0000000

#endif
