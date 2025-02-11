/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <stdio.h>
#include <syscalls.h>


int
reservation_follows_resize_test()
{
	size_t reserveSize = 1 * 1024 * 1024;
	addr_t newAreaBase;
	status_t status = _kern_reserve_address_range(&newAreaBase,
		B_RANDOMIZED_ANY_ADDRESS, reserveSize);
	if (status < B_OK) {
		printf("reserve_address_range 1 unexpectedly failed!\n");
		return status;
	}

	area_id area = create_area("test area", (void**)&newAreaBase,
		B_EXACT_ADDRESS, B_PAGE_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < B_OK) {
		printf("create_area 1 unexpectedly failed!\n");
		return area;
	}

	// Resize the area larger (shriking the reservation)
	status = resize_area(area, B_PAGE_SIZE * 2);
	if (status < B_OK)
		return status;

	// Make sure all is as expected
	addr_t past = (newAreaBase + B_PAGE_SIZE * 2);
	status = _kern_reserve_address_range(&past, B_EXACT_ADDRESS, B_PAGE_SIZE);
	if (status == B_OK) {
		printf("reserve_address_range 2 unexpectedly succeeded!\n");
		return status;
	}

	status = area_for((void*)(newAreaBase + B_PAGE_SIZE * 1));
	if (status != area)
		return status;

	// Now resize the area smaller again
	status = resize_area(area, B_PAGE_SIZE * 1);
	if (status < B_OK)
		return status;

	// And check that the reservation resized with it
	past = (newAreaBase + B_PAGE_SIZE * 1);
	status = _kern_reserve_address_range(&past, B_EXACT_ADDRESS, B_PAGE_SIZE);
	if (status == B_OK) {
		printf("reservation did not resize downwards!\n");
		return status;
	}

	// And check that the reservation still extends to where we expect
	past = (newAreaBase + reserveSize - B_PAGE_SIZE);
	status = _kern_reserve_address_range(&past, B_EXACT_ADDRESS, B_PAGE_SIZE);
	if (status == B_OK) {
		printf("reservation has wrong size!\n");
		return status;
	}

	past = (newAreaBase + reserveSize);
	status = _kern_reserve_address_range(&past, B_EXACT_ADDRESS, B_PAGE_SIZE);
	if (status != B_OK) {
		printf("reservation has wrong size!\n");
		return status;
	}

	delete_area(area);
	return _kern_unreserve_address_range(newAreaBase, reserveSize);
}


int
resize_past_reservation_test()
{
	size_t reserveSize = 4 * B_PAGE_SIZE;
	addr_t newAreaBase;
	status_t status = _kern_reserve_address_range(&newAreaBase,
		B_RANDOMIZED_ANY_ADDRESS, reserveSize * 2);
	if (status < B_OK) {
		printf("reserve_address_range 2 unexpectedly failed!\n");
		return status;
	}

	// Shrink the reservation partially
	status = _kern_unreserve_address_range(newAreaBase + reserveSize, reserveSize);
	if (status < B_OK)
		return status;

	// Make sure it's gone
	status = area_for((void*)(newAreaBase + reserveSize));
	if (status != B_ERROR)
		return -1;

	area_id area = create_area("test area", (void**)&newAreaBase,
		B_EXACT_ADDRESS, B_PAGE_SIZE, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < B_OK) {
		printf("create_area 2 unexpectedly failed!\n");
		return area;
	}

	// Resize the area larger, past the reservation entirely
	status = resize_area(area, reserveSize * 2);
	if (status < B_OK) {
		printf("failed to resize area past first reservation!\n");
		return status;
	}

	// Shrink again, put two new reservations there
	status = resize_area(area, reserveSize);
	if (status < B_OK)
		return status;

	addr_t reserve = newAreaBase + reserveSize + B_PAGE_SIZE;
	status = _kern_reserve_address_range(&reserve, B_EXACT_ADDRESS, B_PAGE_SIZE);
	if (status < B_OK)
		return status;

	reserve += B_PAGE_SIZE;
	status = _kern_reserve_address_range(&reserve, B_EXACT_ADDRESS, B_PAGE_SIZE);
	if (status < B_OK)
		return status;

	// Resize the area larger again, past both reservations
	status = resize_area(area, reserveSize * 2);
	if (status < B_OK) {
		printf("failed to resize area past second and third reservations!\n");
		return status;
	}

	return 0;
}


int
main()
{
	status_t status;

	if ((status = reservation_follows_resize_test()) != 0)
		return status;

	if ((status = resize_past_reservation_test()) != 0)
		return status;

	return 0;
}
