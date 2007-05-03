#ifndef _FBSD_COMPAT_VM_VM_H_
#define _FBSD_COMPAT_VM_VM_H_

#include <stdint.h>
#include <KernelExport.h>

// for x86

typedef uint32_t	vm_offset_t;
typedef uint32_t	vm_paddr_t;

typedef void *		pmap_t;

#define vmspace_pmap(...)		NULL
#define pmap_extract(...)		NULL


#define pmap_kextract(vaddr)	vtophys(vaddr)

/* Marcus' vtophys */
static inline unsigned long
vtophys(vm_offset_t vaddr)
{
	physical_entry pe;
	status_t status;

	status = get_memory_map((void *)vaddr, 1, &pe, 1);
	if (status < 0)
		panic("fbsd compat: get_memory_map failed for %p, error %08lx\n",
			(void *)vaddr, status);

	return (unsigned long)pe.address;
}

#endif
