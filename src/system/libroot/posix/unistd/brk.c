/*
 * Copyright 2015 Simon South, ssouth@simonsouth.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


/*!
 * \file brk.c
 *
 * This module provides \c brk and \c sbrk, functions that adjust the program
 * break of their calling process.
 *
 * A process' program break is the first memory location past the end of its
 * data segment. Moving the program break has the effect of increasing or
 * decreasing the amount of memory allocated to the process for its data and
 * can be used to achieve a simple form of memory management.
 *
 * An ELF executable may contain multiple writable segments that make up a
 * program's data segment, each of which may be placed by Haiku's \c
 * runtime_loader in either one or two areas in memory. Additionally, nothing
 * stops a program from adding onto its data segment directly using \c
 * create_area.  For these reasons this implementation avoids making
 * assumptions about the process' data segment except that it is contiguous.
 * \c brk and \c sbrk will function correctly regardless of the number of
 * (adjacent) areas used to hold the process' data.
 *
 * Note this implementation never creates new areas; its operation is limited
 * to expanding, contracting or deleting areas as necessary.
 */


#include <errno.h>
#include <stdint.h>
#include <sys/resource.h>
#include <unistd.h>

#include <image.h>
#include <OS.h>

#include <errno_private.h>


/*!
 * Locates and returns information about the base (lowest in memory) area of
 * the contiguous set of areas that hold the calling process' data segment.
 *
 * \param base_area_info A pointer to an \c area_info structure to receive
 *                       information about the area.
 * \return \c B_OK on success; \c B_BAD_VALUE otherwise.
 */
static status_t
get_data_segment_base_area_info(area_info *base_area_info)
{
	status_t result = B_ERROR;
	bool app_image_found = false;

	image_info i_info;
	int32 image_cookie = 0;

	/* Locate our app image and output information for the area that holds the
	 * start of its data segment */
	while (!app_image_found
		&& get_next_image_info(0, &image_cookie, &i_info) == B_OK) {
		if (i_info.type == B_APP_IMAGE) {
			app_image_found = true;

			result = get_area_info(area_for(i_info.data), base_area_info);
		}
	}

	return result;
}


/*!
 * Checks whether a proposed new program break would be valid given the
 * resource limits imposed on the calling process.
 *
 * \param program_break The proposed new program break.
 * \param base_address The base (start) address of the process' data segment.
 * \return \c true if the program break would be valid given this process'
 *         resource limits; \c false otherwise.
 */
static bool
program_break_within_resource_limits(void *program_break, void *base_address)
{
	bool result = false;

	struct rlimit rlim;

	if (getrlimit(RLIMIT_AS, &rlim) == 0
		&& (rlim.rlim_cur == RLIM_INFINITY
			|| program_break < (void *)rlim.rlim_cur)
		&& getrlimit(RLIMIT_DATA, &rlim) == 0
		&& (rlim.rlim_cur == RLIM_INFINITY
			|| ((rlim_t)((uint8_t *)program_break - (uint8_t *)(base_address))
				< rlim.rlim_cur)))
		result = true;

	return result;
}


/*!
 * Resizes an area, up or down as necessary, so it ends at the specified
 * address (rounded up to the nearest page boundary).
 *
 * \param a_info An \c area_info structure corresponding to the area on which
 *               to operate.
 * \param address The new address at which the area should end. This address
 *                will be rounded up to the nearest page boundary.
 * \return \c B_OK on success; \c B_BAD_VALUE, \c B_NO_MEMORY or \c B_ERROR
 *         otherwise (refer to the documentation for \c resize_area).
 */
static status_t
resize_area_to_address(area_info *a_info, void *address)
{
	size_t new_size = (uint8_t *)address - (uint8_t *)(a_info->address);
	size_t new_size_aligned = (new_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	return resize_area(a_info->area, new_size_aligned);
}


/*!
 * An internal method that sets the program break of the calling process to the
 * specified address.
 *
 * \param addr The requested new program break. This address will be rounded up
 *             as necessary to align the program break on a page boundary.
 * \param base_area_info An \c area_info structure corresponding to the base
 *                       (lowest in memory) area of the contiguous set of areas
 *                       that hold the calling process' data segment.
 * \return 0 on success; -1 otherwise.
 */
static int
brk_internal(void *addr, area_info *base_area_info)
{
	int result = 0;

	void *next_area_address;
	area_id next_area;
	area_info next_area_info;

	/* First, recursively process the next (higher) adjacent area, if any */
	next_area_address = (uint8_t *)base_area_info->address
		+ base_area_info->size;
	next_area = area_for(next_area_address);
	if (next_area != B_ERROR) {
		result = get_area_info(next_area, &next_area_info) == B_OK ?
			brk_internal(addr, &next_area_info) : -1;
	} else {
		/* This is the highest ("last") area in the data segment, so any
		 * increase in size needs to occur on this area */
		if (addr > next_area_address) {
			result = resize_area_to_address(base_area_info, addr) == B_OK ? 0
				: -1;
		}
	}

	if (result == 0) {
		if (addr <= base_area_info->address) {
			/* This area starts at or above the program break the caller has
			 * requested, so the entire area can be deleted */
			result = delete_area(base_area_info->area) == B_OK ? 0 : -1;
		} else if (addr < next_area_address) {
			/* The requested program break lies within this area, so this area
			 * must be contracted */
			result = resize_area_to_address(base_area_info, addr) == B_OK ? 0
				: -1;
		}
	}

	return result;
}


/*!
 * Sets the calling process' program break to the specified address, if valid
 * and within the limits of available resources, expanding or contracting the
 * process' data segment as necessary.
 *
 * \param addr The requested new program break.
 * \return 0 on success; -1 on error (in which case \c errno will be set to
 *         \c ENOMEM).
 */
int
brk(void *addr)
{
	int result = -1;

	area_info base_area_info;

	if (get_data_segment_base_area_info(&base_area_info) == B_OK
		&& addr > base_area_info.address
		&& program_break_within_resource_limits(addr, base_area_info.address))
		result = brk_internal(addr, &base_area_info);

	if (result == -1)
		__set_errno(ENOMEM);

	return result;
}


/*!
 * Adjusts the calling process' program break up or down by the requested
 * number of bytes, if valid and within the limits of available resources,
 * expanding or contracting the process' data segment as necessary.
 *
 * \param increment The amount, positive or negative, in bytes by which to
 *                  adjust the program break. This value will be rounded up as
 *                  necessary to align the program break on a page boundary.
 * \return The previous program break on success; (void *)-1 on error (in which
 *         case \c errno will be set to \c ENOMEM).
 */
void *
sbrk(intptr_t increment)
{
	void *result = NULL;

	area_info base_area_info;

	area_id next_area;
	area_info next_area_info;

	uint8_t *program_break;
	uint8_t *new_program_break;

	if (get_data_segment_base_area_info(&base_area_info) == B_OK) {
		/* Find the current program break, which will be the memory address
		 * just past the end of the highest ("last") of the areas that hold the
		 * data segment */
		next_area = base_area_info.area;
		do {
			if (get_area_info(next_area, &next_area_info) == B_OK) {
				next_area = area_for((uint8_t *)(next_area_info.address)
					+ next_area_info.size);
			} else {
				result = (void *)-1;
			}
		} while (next_area != B_ERROR && result == NULL);

		if (result == NULL) {
			program_break = (uint8_t *)(next_area_info.address)
				+ next_area_info.size;
			new_program_break = program_break + increment;

			/* If the requested increment is zero, just return the address of
			 * the current program break; otherwise, set a new program break
			 * and return the address of the old one */
			if (increment == 0
				|| (program_break_within_resource_limits(new_program_break,
						base_area_info.address)
					&& brk_internal(new_program_break, &base_area_info) == 0))
				result = program_break;
			else
				result = (void *)-1;
		}
	} else {
		result = (void *)-1;
	}

	if (result == (void *)-1)
		__set_errno(ENOMEM);

	return result;
}
