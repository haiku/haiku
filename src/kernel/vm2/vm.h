#include "list.h"

#ifndef _VM_TYPES
#define _VM_TYPES
const int PAGE_SIZE = 4096;
const int BITS_IN_PAGE_SIZE = 12;
const int AREA_HASH_TABLE_SIZE = 40;
struct vnode : public node
{
	int fd;
	unsigned long offset;
	bool valid;
	long  count;

	vnode (void)
	{
	valid=false;
	count=0;
	}
};
#define B_OS_NAME_LENGTH 32
enum protectType {none=0,readable, writable,copyOnWrite,symCopyOnWrite};
//B_EXACT_ADDRESS 		You want the value of *addr to be taken literally and strictly. 
//						If the area can't be allocated at that location, the function fails. 
//B_BASE_ADDRESS 		The area can start at a location equal to or greater than *addr. 
//B_ANY_ADDRESS 		The starting address is determined by the system. 
//						In this case, the value that's pointed to by addr is ignored (going into the function).
//B_ANY_KERNEL_ADDRESS	The starting address is determined by the system, and the new area will belong to the kernel's team; 
//						it won't be deleted when the application quits. In this case, the value that's pointed to by addr is 
//						ignored (going into the function)
//B_CLONE_ADDRESS 		This is only meaningful to the clone_area() function.
enum addressSpec {EXACT,BASE,ANY,ANY_KERNEL,CLONE};

//B_FULL_LOCK 	The area's memory is locked into RAM when the area is created, and won't be swapped out.
//B_CONTIGUOUS	Not only is the area's memory locked into RAM, it's also guaranteed to be contiguous. This is particularly - 
//				and perhaps exclusively - useful to designers of certain types of device drivers.
//B_LAZY_LOCK 	Allows individual pages of memory to be brought into RAM through the natural order of things and then locks them.
//B_NO_LOCK 	Pages are never locked, they're swapped in and out as needed.
//B_LOMEM 		This is a special constant that's used for for areas that need to be locked, contiguous, and that fit within the 
//				first 16MB of physical memory. The folks that need this constant know who they are.

enum pageState {FULL,CONTIGUOUS,LAZY,NO_LOCK,LOMEM};
#define USER_BASE 0x10000000
#define KERNEL_BASE 0x80000000
#define CACHE_BEGIN 0x90000000
#define CACHE_END 0xe0000000

#endif
