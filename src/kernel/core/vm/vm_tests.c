/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <vm.h>
#include <debug.h>
#include <arch/cpu.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>

void vm_test()
{
//	region_id region, region2, region3;
//	addr region_addr;
//	int i;

	dprintf("vm_test: entry\n");
#if 1
	dprintf("vm_test 1: creating anonymous region and writing to it\n");
	{
		region_id region;
		addr region_addr;

		region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "test_region", (void **)&region_addr,
			REGION_ADDR_ANY_ADDRESS, PAGE_SIZE * 16, REGION_WIRING_LAZY, LOCK_RW|LOCK_KERNEL);
		if(region < 0)
			panic("vm_test 1: failed to create test region\n");
		dprintf("region = 0x%x, addr = 0x%lx\n", region, region_addr);

		memset((void *)region_addr, 0, PAGE_SIZE * 16);

		dprintf("memsetted the region\n");
		if(vm_delete_region(vm_get_kernel_aspace_id(), region) < 0)
			panic("vm_test 1: error deleting test region\n");
		dprintf("deleted the region\n");
	}
#endif
#if 1
	dprintf("vm_test 2: creating physical region and writing and reading from it\n");
	{
		region_id region;
		vm_region_info info;
		char *ptr;
		int i;

		region = vm_map_physical_memory(vm_get_kernel_aspace_id(), "test_physical_region", (void **)&ptr,
			REGION_ADDR_ANY_ADDRESS, PAGE_SIZE * 16, LOCK_RW|LOCK_KERNEL, 0xb8000);
		if(region < 0)
			panic("vm_test 2: failed to create test region\n");

		vm_get_region_info(region, &info);
		dprintf("region = 0x%x, addr = %p, region->base = 0x%lx\n", region, ptr, info.base);
		if((addr)ptr != info.base)
			panic("vm_test 2: info returned about region does not match pointer returned\n");

		for(i=0; i<64; i++) {
			ptr[i] = 'a';
		}
		if(vm_delete_region(vm_get_kernel_aspace_id(), region) < 0)
			panic("vm_test 2: error deleting test region\n");
		dprintf("deleted the region\n");
	}
#endif
#if 1
	dprintf("vm_test 3: testing some functionality of vm_get_page_mapping(), vm_get/put_physical_page()\n");
	{
		addr va, pa;
		addr va2;

		vm_get_page_mapping(vm_get_kernel_aspace_id(), 0x80000000, &pa);
		vm_get_physical_page(pa, &va, PHYSICAL_PAGE_CAN_WAIT);
		dprintf("pa 0x%lx va 0x%lx\n", pa, va);
		dprintf("%d\n", memcmp((void *)0x80000000, (void *)va, PAGE_SIZE));

		vm_get_page_mapping(vm_get_kernel_aspace_id(), 0x80001000, &pa);
		vm_get_physical_page(pa, &va2, PHYSICAL_PAGE_CAN_WAIT);
		dprintf("pa 0x%lx va 0x%lx\n", pa, va2);
		dprintf("%d\n", memcmp((void *)0x80001000, (void *)va2, PAGE_SIZE));

		vm_put_physical_page(va);
		vm_put_physical_page(va2);

		vm_get_page_mapping(vm_get_kernel_aspace_id(), 0x80000000, &pa);
		vm_get_physical_page(pa, &va, PHYSICAL_PAGE_CAN_WAIT);
		dprintf("pa 0x%lx va 0x%lx\n", pa, va);
		dprintf("%d\n", memcmp((void *)0x80000000, (void *)va, PAGE_SIZE));

		vm_put_physical_page(va);
	}
#endif
#if 1
	dprintf("vm_test 4: cloning vidmem and testing if it compares\n");
	{
		region_id region, region2;
		vm_region_info info;
		void *ptr;
		int rc;

		region = vm_find_region_by_name(vm_get_kernel_aspace_id(), "vid_mem");
		if(region < 0)
			panic("vm_test 4: error finding region 'vid_mem'\n");
		dprintf("vid_mem region = 0x%x\n", region);

		region2 = vm_clone_region(vm_get_kernel_aspace_id(), "vid_mem2",
			&ptr, REGION_ADDR_ANY_ADDRESS, region, REGION_NO_PRIVATE_MAP, LOCK_RW|LOCK_KERNEL);
		if(region2 < 0)
			panic("vm_test 4: error cloning region 'vid_mem'\n");
		dprintf("region2 = 0x%x, ptr = %p\n", region2, ptr);

		vm_get_region_info(region, &info);
		rc = memcmp(ptr, (void *)info.base, info.size);
		if(rc != 0)
			panic("vm_test 4: regions are not identical\n");
		else
			dprintf("vm_test 4: comparison ok\n");
		if(vm_delete_region(vm_get_kernel_aspace_id(), region2) < 0)
			panic("vm_test 4: error deleting cloned region\n");
	}
#endif
#if 1
	dprintf("vm_test 5: cloning vidmem in RO and testing if it compares\n");
	{
		region_id region, region2;
		vm_region_info info;
		void *ptr;
		int rc;

		region = vm_find_region_by_name(vm_get_kernel_aspace_id(), "vid_mem");
		if(region < 0)
			panic("vm_test 5: error finding region 'vid_mem'\n");
		dprintf("vid_mem region = 0x%x\n", region);

		region2 = vm_clone_region(vm_get_kernel_aspace_id(), "vid_mem3",
			&ptr, REGION_ADDR_ANY_ADDRESS, region, REGION_NO_PRIVATE_MAP, LOCK_RO|LOCK_KERNEL);
		if(region2 < 0)
			panic("vm_test 5: error cloning region 'vid_mem'\n");
		dprintf("region2 = 0x%x, ptr = %p\n", region2, ptr);

		vm_get_region_info(region, &info);
		rc = memcmp(ptr, (void *)info.base, info.size);
		if(rc != 0)
			panic("vm_test 5: regions are not identical\n");
		else
			dprintf("vm_test 5: comparison ok\n");

		if(vm_delete_region(vm_get_kernel_aspace_id(), region2) < 0)
			panic("vm_test 5: error deleting cloned region\n");
	}
#endif
#if 1
	dprintf("vm_test 6: creating anonymous region, cloning it, and testing if it compares\n");
	{
		region_id region, region2;
		void *region_addr;
		void *ptr;
		int rc;

		region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "test_region", &region_addr,
			REGION_ADDR_ANY_ADDRESS, PAGE_SIZE * 16, REGION_WIRING_LAZY, LOCK_RW|LOCK_KERNEL);
		if(region < 0)
			panic("vm_test 6: error creating test region\n");
		dprintf("region = 0x%x, addr = %p\n", region, region_addr);

		memset(region_addr, 99, PAGE_SIZE * 16);

		dprintf("memsetted the region\n");

		region2 = vm_clone_region(vm_get_kernel_aspace_id(), "test_region2",
			&ptr, REGION_ADDR_ANY_ADDRESS, region, REGION_NO_PRIVATE_MAP, LOCK_RW|LOCK_KERNEL);
		if(region2 < 0)
			panic("vm_test 6: error cloning test region\n");
		dprintf("region2 = 0x%x, ptr = %p\n", region2, ptr);

		rc = memcmp(region_addr, ptr, PAGE_SIZE * 16);
		if(rc != 0)
			panic("vm_test 6: regions are not identical\n");
		else
			dprintf("vm_test 6: comparison ok\n");

		if(vm_delete_region(vm_get_kernel_aspace_id(), region) < 0)
			panic("vm_test 6: error deleting test region\n");

		if(vm_delete_region(vm_get_kernel_aspace_id(), region2) < 0)
			panic("vm_test 6: error deleting cloned region\n");
	}
#endif
#if 1
	dprintf("vm_test 7: mmaping a known file a few times and verifying they see the same data\n");
	{
		void *ptr, *ptr2;
		region_id rid, rid2;
//		char *blah;
		int fd;

		fd = sys_open("/boot/kernel", STREAM_TYPE_FILE, 0);

		rid = vm_map_file(vm_get_kernel_aspace_id(), "mmap_test", &ptr, REGION_ADDR_ANY_ADDRESS,
			PAGE_SIZE, LOCK_RW|LOCK_KERNEL, REGION_NO_PRIVATE_MAP, "/boot/kernel", 0);

		rid2 = vm_map_file(vm_get_kernel_aspace_id(), "mmap_test2", &ptr2, REGION_ADDR_ANY_ADDRESS,
			PAGE_SIZE, LOCK_RW|LOCK_KERNEL, REGION_NO_PRIVATE_MAP, "/boot/kernel", 0);

		dprintf("diff %d\n", memcmp(ptr, ptr2, PAGE_SIZE));

		dprintf("removing regions\n");

		vm_delete_region(vm_get_kernel_aspace_id(), rid);
		vm_delete_region(vm_get_kernel_aspace_id(), rid2);

		dprintf("regions deleted\n");

		sys_close(fd);

		dprintf("vm_test 7: passed\n");
	}
#endif
#if 1
	dprintf("vm_test 8: creating anonymous region, cloning it with private mapping\n");
	{
		region_id region, region2;
		void *region_addr;
		void *ptr;
		int rc;

		dprintf("vm_test 8: creating test region...\n");

		region = vm_create_anonymous_region(vm_get_kernel_aspace_id(), "test_region", &region_addr,
			REGION_ADDR_ANY_ADDRESS, PAGE_SIZE * 16, REGION_WIRING_LAZY, LOCK_RW|LOCK_KERNEL);
		if(region < 0)
			panic("vm_test 8: error creating test region\n");
		dprintf("region = 0x%x, addr = %p\n", region, region_addr);

		dprintf("vm_test 8: memsetting the first 2 pages\n");

		memset(region_addr, 99, PAGE_SIZE * 2);

		dprintf("vm_test 8: cloning test region with PRIVATE_MAP\n");

		region2 = vm_clone_region(vm_get_kernel_aspace_id(), "test_region2",
			&ptr, REGION_ADDR_ANY_ADDRESS, region, REGION_PRIVATE_MAP, LOCK_RW|LOCK_KERNEL);
		if(region2 < 0)
			panic("vm_test 8: error cloning test region\n");
		dprintf("region2 = 0x%x, ptr = %p\n", region2, ptr);

		dprintf("vm_test 8: comparing first 2 pages, shoudld be identical\n");

		rc = memcmp(region_addr, ptr, PAGE_SIZE * 2);
		if(rc != 0)
			panic("vm_test 8: regions are not identical\n");
		else
			dprintf("vm_test 8: comparison ok\n");

		dprintf("vm_test 8: changing one page in the clone and comparing, should now differ\n");

		memset(ptr, 1, 1);

		rc = memcmp(region_addr, ptr, PAGE_SIZE);
		if(rc != 0)
			dprintf("vm_test 8: regions are not identical, good\n");
		else
			panic("vm_test 8: comparison shows not private mapping\n");

		dprintf("vm_test 8: comparing untouched pages in source and clone, should be identical\n");

		rc = memcmp((char *)region_addr + 2*PAGE_SIZE, (char *)ptr + 2*PAGE_SIZE, PAGE_SIZE * 1);
		if(rc != 0)
			panic("vm_test 8: regions are not identical\n");
		else
			dprintf("vm_test 8: comparison ok\n");

		dprintf("vm_test 8: modifying source page and comparing, should be the same\n");

		memset((char *)region_addr + 3*PAGE_SIZE, 2, 4);

		rc = memcmp((char *)region_addr + 3*PAGE_SIZE, (char *)ptr + 3*PAGE_SIZE, PAGE_SIZE * 1);
		if(rc != 0)
			panic("vm_test 8: regions are not identical\n");
		else
			dprintf("vm_test 8: comparison ok\n");


		dprintf("vm_test 8: modifying new page in clone, should not effect source, which is also untouched\n");

		memset((char *)ptr + 4*PAGE_SIZE, 2, 4);

		rc = memcmp((char *)region_addr + 4*PAGE_SIZE, (char *)ptr + 4*PAGE_SIZE, PAGE_SIZE * 1);
		if(rc != 0)
			dprintf("vm_test 8: regions are not identical, good\n");
		else
			panic("vm_test 8: comparison shows not private mapping\n");

		if(vm_delete_region(vm_get_kernel_aspace_id(), region) < 0)
			panic("vm_test 8: error deleting test region\n");

		if(vm_delete_region(vm_get_kernel_aspace_id(), region2) < 0)
			panic("vm_test 8: error deleting cloned region\n");
	}
#endif
#if 1
	dprintf("vm_test 9: mmaping a known file a few times, one with private mappings\n");
	{
		void *ptr, *ptr2;
		region_id rid, rid2;
		int err;

		dprintf("vm_test 9: mapping /boot/kernel twice\n");

		rid = vm_map_file(vm_get_kernel_aspace_id(), "mmap_test", &ptr, REGION_ADDR_ANY_ADDRESS,
			PAGE_SIZE*4, LOCK_RW|LOCK_KERNEL, REGION_NO_PRIVATE_MAP, "/boot/kernel", 0);

		rid2 = vm_map_file(vm_get_kernel_aspace_id(), "mmap_test2", &ptr2, REGION_ADDR_ANY_ADDRESS,
			PAGE_SIZE*4, LOCK_RW|LOCK_KERNEL, REGION_PRIVATE_MAP, "/boot/kernel", 0);

		err = memcmp(ptr, ptr2, PAGE_SIZE);
		if(err)
			panic("vm_test 9: two mappings are different\n");

		dprintf("vm_test 9: modifying private mapping in section that has been referenced, should be different\n");

		memset(ptr2, 0xaa, 4);

		err = memcmp(ptr, ptr2, PAGE_SIZE);
		if(!err)
			panic("vm_test 9: two mappings still identical\n");

		dprintf("vm_test 9: modifying private mapping in section that hasn't been touched, should be different\n");

		memset((char *)ptr2 + PAGE_SIZE, 0xaa, 4);

		err = memcmp((char *)ptr + PAGE_SIZE, (char *)ptr2 + PAGE_SIZE, PAGE_SIZE);
		if(!err)
			panic("vm_test 9: two mappings still identical\n");

		dprintf("vm_test 9: modifying non-private mapping in section that hasn't been touched\n");

		memset((char *)ptr + PAGE_SIZE*2, 0xaa, 4);

		err = memcmp((char *)ptr + PAGE_SIZE*2, (char *)ptr2 + PAGE_SIZE*2, PAGE_SIZE);
		if(err)
			panic("vm_test 9: two mappings are different\n");

		dprintf("vm_test 9: removing regions\n");

		vm_delete_region(vm_get_kernel_aspace_id(), rid);
		vm_delete_region(vm_get_kernel_aspace_id(), rid2);

		dprintf("vm_test 9: regions deleted\n");

		dprintf("vm_test 9: passed\n");
	}
#endif
	dprintf("vm_test: done\n");
}
