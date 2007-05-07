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

unsigned long vtophys(vm_offset_t vaddr);

#endif
