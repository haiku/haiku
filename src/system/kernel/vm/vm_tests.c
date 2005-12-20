/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm.h>
#include <vm_address_space.h>

#include <KernelExport.h>

#include <string.h>
#include <ctype.h>
#include <stdio.h>


void
vm_test(void)
{
	dprintf("vm_test: entry\n");
#if 1
	dprintf("vm_test 1: creating anonymous region and writing to it\n");
	{
		area_id region;
		addr_t region_addr;

		region = vm_create_anonymous_area(vm_kernel_address_space_id(), "test_region", (void **)&region_addr,
			B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE * 16, B_NO_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (region < 0)
			panic("vm_test 1: failed to create test region\n");
		dprintf("region = 0x%lx, addr = 0x%lx\n", region, region_addr);

		memset((void *)region_addr, 0, B_PAGE_SIZE * 16);

		dprintf("memsetted the region\n");
		if (vm_delete_area(vm_kernel_address_space_id(), region) < 0)
			panic("vm_test 1: error deleting test region\n");
		dprintf("deleted the region\n");
	}
#endif
#if 1
	dprintf("vm_test 2: creating physical region and writing and reading from it\n");
	{
		area_id region;
		area_info info;
		char *ptr;
		int i;

		region = vm_map_physical_memory(vm_kernel_address_space_id(), "test_physical_region", (void **)&ptr,
			B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE * 16, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0xb8000);
		if (region < 0)
			panic("vm_test 2: failed to create test region\n");

		get_area_info(region, &info);
		dprintf("region = 0x%lx, addr = %p, region->base = %p\n", region, ptr, info.address);
		if (ptr != info.address)
			panic("vm_test 2: info returned about region does not match pointer returned\n");

		for(i=0; i<64; i++) {
			ptr[i] = 'a';
		}
		if (vm_delete_area(vm_kernel_address_space_id(), region) < 0)
			panic("vm_test 2: error deleting test region\n");
		dprintf("deleted the region\n");
	}
#endif
#if 1
	dprintf("vm_test 3: testing some functionality of vm_get_page_mapping(), vm_get/put_physical_page()\n");
	{
		addr_t va, pa;
		addr_t va2;

		vm_get_page_mapping(vm_kernel_address_space_id(), 0x80000000, &pa);
		vm_get_physical_page(pa, &va, PHYSICAL_PAGE_CAN_WAIT);
		dprintf("pa 0x%lx va 0x%lx\n", pa, va);
		dprintf("%d\n", memcmp((void *)0x80000000, (void *)va, B_PAGE_SIZE));

		vm_get_page_mapping(vm_kernel_address_space_id(), 0x80001000, &pa);
		vm_get_physical_page(pa, &va2, PHYSICAL_PAGE_CAN_WAIT);
		dprintf("pa 0x%lx va 0x%lx\n", pa, va2);
		dprintf("%d\n", memcmp((void *)0x80001000, (void *)va2, B_PAGE_SIZE));

		vm_put_physical_page(va);
		vm_put_physical_page(va2);

		vm_get_page_mapping(vm_kernel_address_space_id(), 0x80000000, &pa);
		vm_get_physical_page(pa, &va, PHYSICAL_PAGE_CAN_WAIT);
		dprintf("pa 0x%lx va 0x%lx\n", pa, va);
		dprintf("%d\n", memcmp((void *)0x80000000, (void *)va, B_PAGE_SIZE));

		vm_put_physical_page(va);
	}
#endif
#if 1
	dprintf("vm_test 4: cloning vidmem and testing if it compares\n");
	{
		area_id region, region2;
		area_info info;
		void *ptr;
		int rc;

		region = find_area("vid_mem");
		if (region < 0)
			panic("vm_test 4: error finding region 'vid_mem'\n");
		dprintf("vid_mem region = 0x%lx\n", region);

		region2 = vm_clone_area(vm_kernel_address_space_id(), "vid_mem2",
			&ptr, B_ANY_KERNEL_ADDRESS, region, REGION_NO_PRIVATE_MAP, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (region2 < 0)
			panic("vm_test 4: error cloning region 'vid_mem'\n");
		dprintf("region2 = 0x%lx, ptr = %p\n", region2, ptr);

		get_area_info(region, &info);
		rc = memcmp(ptr, (void *)info.address, info.size);
		if (rc != 0)
			panic("vm_test 4: regions are not identical\n");
		else
			dprintf("vm_test 4: comparison ok\n");
		if (vm_delete_area(vm_kernel_address_space_id(), region2) < 0)
			panic("vm_test 4: error deleting cloned region\n");
	}
#endif
#if 1
	dprintf("vm_test 5: cloning vidmem in RO and testing if it compares\n");
	{
		area_id region, region2;
		area_info info;
		void *ptr;
		int rc;

		region = find_area("vid_mem");
		if (region < 0)
			panic("vm_test 5: error finding region 'vid_mem'\n");
		dprintf("vid_mem region = 0x%lx\n", region);

		region2 = vm_clone_area(vm_kernel_address_space_id(), "vid_mem3",
			&ptr, B_ANY_KERNEL_ADDRESS, region, REGION_NO_PRIVATE_MAP, B_KERNEL_READ_AREA);
		if (region2 < 0)
			panic("vm_test 5: error cloning region 'vid_mem'\n");
		dprintf("region2 = 0x%lx, ptr = %p\n", region2, ptr);

		get_area_info(region, &info);
		rc = memcmp(ptr, (void *)info.address, info.size);
		if (rc != 0)
			panic("vm_test 5: regions are not identical\n");
		else
			dprintf("vm_test 5: comparison ok\n");

		if (vm_delete_area(vm_kernel_address_space_id(), region2) < 0)
			panic("vm_test 5: error deleting cloned region\n");
	}
#endif
#if 1
	dprintf("vm_test 6: creating anonymous region, cloning it, and testing if it compares\n");
	{
		area_id region, region2;
		void *region_addr;
		void *ptr;
		int rc;

		region = vm_create_anonymous_area(vm_kernel_address_space_id(), "test_region", &region_addr,
			B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE * 16, B_NO_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (region < 0)
			panic("vm_test 6: error creating test region\n");
		dprintf("region = 0x%lx, addr = %p\n", region, region_addr);

		memset(region_addr, 99, B_PAGE_SIZE * 16);

		dprintf("memsetted the region\n");

		region2 = vm_clone_area(vm_kernel_address_space_id(), "test_region2",
			&ptr, B_ANY_KERNEL_ADDRESS, region, REGION_NO_PRIVATE_MAP, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if(region2 < 0)
			panic("vm_test 6: error cloning test region\n");
		dprintf("region2 = 0x%lx, ptr = %p\n", region2, ptr);

		rc = memcmp(region_addr, ptr, B_PAGE_SIZE * 16);
		if(rc != 0)
			panic("vm_test 6: regions are not identical\n");
		else
			dprintf("vm_test 6: comparison ok\n");

		if(vm_delete_area(vm_kernel_address_space_id(), region) < 0)
			panic("vm_test 6: error deleting test region\n");

		if(vm_delete_area(vm_kernel_address_space_id(), region2) < 0)
			panic("vm_test 6: error deleting cloned region\n");
	}
#endif
#if 0
	dprintf("vm_test 7: mmaping a known file a few times and verifying they see the same data\n");
	{
		void *ptr, *ptr2;
		area_id rid, rid2;
//		char *blah;
		int fd;

		fd = _kern_open("/boot/beos/system/kernel_" OBOS_ARCH, 0);

		rid = vm_map_file(vm_kernel_address_space_id(), "mmap_test", &ptr, B_ANY_KERNEL_ADDRESS,
			B_PAGE_SIZE, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, REGION_NO_PRIVATE_MAP, "/boot/kernel", 0);

		rid2 = vm_map_file(vm_kernel_address_space_id(), "mmap_test2", &ptr2, B_ANY_KERNEL_ADDRESS,
			B_PAGE_SIZE, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, REGION_NO_PRIVATE_MAP, "/boot/kernel", 0);

		dprintf("diff %d\n", memcmp(ptr, ptr2, B_PAGE_SIZE));

		dprintf("removing regions\n");

		vm_delete_area(vm_kernel_address_space_id(), rid);
		vm_delete_area(vm_kernel_address_space_id(), rid2);

		dprintf("regions deleted\n");

		_kern_close(fd);

		dprintf("vm_test 7: passed\n");
	}
#endif
#if 1
	dprintf("vm_test 8: creating anonymous region, cloning it with private mapping\n");
	{
		area_id region, region2;
		void *region_addr;
		void *ptr;
		int rc;

		dprintf("vm_test 8: creating test region...\n");

		region = vm_create_anonymous_area(vm_kernel_address_space_id(), "test_region", &region_addr,
			B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE * 16, B_NO_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if(region < 0)
			panic("vm_test 8: error creating test region\n");
		dprintf("region = 0x%lx, addr = %p\n", region, region_addr);

		dprintf("vm_test 8: memsetting the first 2 pages\n");

		memset(region_addr, 99, B_PAGE_SIZE * 2);

		dprintf("vm_test 8: cloning test region with PRIVATE_MAP\n");

		region2 = vm_clone_area(vm_kernel_address_space_id(), "test_region2",
			&ptr, B_ANY_KERNEL_ADDRESS, region, REGION_PRIVATE_MAP, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if(region2 < 0)
			panic("vm_test 8: error cloning test region\n");
		dprintf("region2 = 0x%lx, ptr = %p\n", region2, ptr);

		dprintf("vm_test 8: comparing first 2 pages, shoudld be identical\n");

		rc = memcmp(region_addr, ptr, B_PAGE_SIZE * 2);
		if(rc != 0)
			panic("vm_test 8: regions are not identical\n");
		else
			dprintf("vm_test 8: comparison ok\n");

		dprintf("vm_test 8: changing one page in the clone and comparing, should now differ\n");

		memset(ptr, 1, 1);

		rc = memcmp(region_addr, ptr, B_PAGE_SIZE);
		if(rc != 0)
			dprintf("vm_test 8: regions are not identical, good\n");
		else
			panic("vm_test 8: comparison shows not private mapping\n");

		dprintf("vm_test 8: comparing untouched pages in source and clone, should be identical\n");

		rc = memcmp((char *)region_addr + 2*B_PAGE_SIZE, (char *)ptr + 2*B_PAGE_SIZE, B_PAGE_SIZE * 1);
		if(rc != 0)
			panic("vm_test 8: regions are not identical\n");
		else
			dprintf("vm_test 8: comparison ok\n");

		dprintf("vm_test 8: modifying source page and comparing, should be the same\n");

		memset((char *)region_addr + 3*B_PAGE_SIZE, 2, 4);

		rc = memcmp((char *)region_addr + 3*B_PAGE_SIZE, (char *)ptr + 3*B_PAGE_SIZE, B_PAGE_SIZE * 1);
		if(rc != 0)
			panic("vm_test 8: regions are not identical\n");
		else
			dprintf("vm_test 8: comparison ok\n");


		dprintf("vm_test 8: modifying new page in clone, should not effect source, which is also untouched\n");

		memset((char *)ptr + 4*B_PAGE_SIZE, 2, 4);

		rc = memcmp((char *)region_addr + 4*B_PAGE_SIZE, (char *)ptr + 4*B_PAGE_SIZE, B_PAGE_SIZE * 1);
		if(rc != 0)
			dprintf("vm_test 8: regions are not identical, good\n");
		else
			panic("vm_test 8: comparison shows not private mapping\n");

		if(vm_delete_area(vm_kernel_address_space_id(), region) < 0)
			panic("vm_test 8: error deleting test region\n");

		if(vm_delete_area(vm_kernel_address_space_id(), region2) < 0)
			panic("vm_test 8: error deleting cloned region\n");
	}
#endif
#if 1
	dprintf("vm_test 9: mmaping a known file a few times, one with private mappings\n");
	{
		void *ptr, *ptr2;
		area_id rid, rid2;
		int err;

		dprintf("vm_test 9: mapping /boot/kernel twice\n");

		rid = vm_map_file(vm_kernel_address_space_id(), "mmap_test", &ptr, B_ANY_KERNEL_ADDRESS,
			B_PAGE_SIZE*4, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, REGION_NO_PRIVATE_MAP, "/boot/kernel", 0);

		rid2 = vm_map_file(vm_kernel_address_space_id(), "mmap_test2", &ptr2, B_ANY_KERNEL_ADDRESS,
			B_PAGE_SIZE*4, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, REGION_PRIVATE_MAP, "/boot/kernel", 0);

		err = memcmp(ptr, ptr2, B_PAGE_SIZE);
		if(err)
			panic("vm_test 9: two mappings are different\n");

		dprintf("vm_test 9: modifying private mapping in section that has been referenced, should be different\n");

		memset(ptr2, 0xaa, 4);

		err = memcmp(ptr, ptr2, B_PAGE_SIZE);
		if(!err)
			panic("vm_test 9: two mappings still identical\n");

		dprintf("vm_test 9: modifying private mapping in section that hasn't been touched, should be different\n");

		memset((char *)ptr2 + B_PAGE_SIZE, 0xaa, 4);

		err = memcmp((char *)ptr + B_PAGE_SIZE, (char *)ptr2 + B_PAGE_SIZE, B_PAGE_SIZE);
		if(!err)
			panic("vm_test 9: two mappings still identical\n");

		dprintf("vm_test 9: modifying non-private mapping in section that hasn't been touched\n");

		memset((char *)ptr + B_PAGE_SIZE*2, 0xaa, 4);

		err = memcmp((char *)ptr + B_PAGE_SIZE*2, (char *)ptr2 + B_PAGE_SIZE*2, B_PAGE_SIZE);
		if(err)
			panic("vm_test 9: two mappings are different\n");

		dprintf("vm_test 9: removing regions\n");

		vm_delete_area(vm_kernel_address_space_id(), rid);
		vm_delete_area(vm_kernel_address_space_id(), rid2);

		dprintf("vm_test 9: regions deleted\n");

		dprintf("vm_test 9: passed\n");
	}
#endif
#if 1
	dprintf("vm_test 10: copy area\n");
	{
		area_id a, b;
		void *address = NULL;
		uint32 *ta, *tb;

		a = create_area("source of copy", &address, B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE * 4,
				B_NO_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		ta = (uint32 *)address;
		ta[0] = 0x1234;
		ta[1024] = 0x5678;
		ta[2048] = 0xabcd;
		ta[3072] = 0xefef;
		
		b = vm_copy_area(vm_kernel_address_space_id(), "copy of source", &address, B_ANY_KERNEL_ADDRESS,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, a);
		tb = (uint32 *)address;
		if (tb[0] != 0x1234 || tb[1024] != 0x5678 || tb[2048] != 0xabcd || tb[3072] != 0xefef)
			panic("vm_test 10: wrong values %lx, %lx, %lx, %lx\n", tb[0], tb[1024], tb[2048], tb[3072]);

		tb[0] = 0xaaaa;
		if (tb[0] != 0xaaaa || ta[0] != 0x1234)
			panic("vm_test 10: %lx (aaaa), %lx (1234)\n", tb[0], ta[0]);

		ta[1024] = 0xbbbb;
		if (ta[1024] != 0xbbbb || tb[1024] != 0x5678)
			panic("vm_test 10: %lx (bbbb), %lx (5678)\n", ta[1024], tb[1024]);

		dprintf("vm_test 10: remove areas\n");

		vm_delete_area(vm_kernel_address_space_id(), a);
		vm_delete_area(vm_kernel_address_space_id(), b);
	}
	dprintf("vm_test 10: passed\n");
#endif
#if 1
	dprintf("vm_test 11: resize area\n");
	{
		void *address = NULL;
		status_t status;
		area_id a;

		a = create_area("resizable", &address, B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE * 4,
				B_NO_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

		status = resize_area(a, B_PAGE_SIZE * 2);
		if (status != B_OK)
			panic("vm_test 11: resize_area() failed trying to shrink area: %s\n", strerror(status));

		status = resize_area(a, B_PAGE_SIZE * 4);
		if (status != B_OK) {
			dprintf("vm_test 11: resize_area() failed trying to grow area: %s\n", strerror(status));
		} else {
			uint32 *values = (uint32 *)address;
			dprintf("vm_test 11: write something in the enlarged section\n");
			values[3072] = 0x1234;
		}

		dprintf("vm_test 11: remove areas\n");

		vm_delete_area(vm_kernel_address_space_id(), a);
	}
	dprintf("vm_test 11: passed\n");
#endif
	dprintf("vm_test: done\n");
}
