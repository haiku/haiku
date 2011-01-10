/*
 * Copyright 2004-2011, Haiku, Inc. All rights reserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	PIO data transmission

	This file is more difficult then you might expect as the SCSI system
	uses physical addresses everywhere which have to be mapped into
	virtual address space during transmission. Additionally, during ATAPI
	commands we may have to transmit more data then exist because the
	data len specified by the command doesn't need to be the same as
	of the data buffer provided.

	The handling of S/G entries of odd size may look superfluous as the
	SCSI bus manager can take care of that. In general, this would be possible
	as most controllers need even alignment for DMA as well, but some can
	handle _any_ S/G list and it wouldn't be sensitive to enforce stricter
	alignement just for some rare PIO transmissions.

	Little hint for the meaning of "transferred": this is the number of bytes
	sent over the bus. For read-transmissions, this may be one more then copied
	into the buffer (the extra byte read is stored in device->odd_byte), for
	write-transmissions, this may be one less (the waiting byte is pending in
	device->odd_byte).

	In terms of error handling: we don't bother checking transmission of every
	single byte via read/write_pio(). At least at the end of the request, when
	the status bits are verified, we will see that something has gone wrong.

	TBD: S/G entries may have odd start address. For non-Intel architecture
	we either have to copy data to an aligned buffer or have to modify
	PIO-handling in controller drivers.
*/

#include "ide_internal.h"
#include "ide_sim.h"

#include <thread.h>
#include <vm/vm.h>

#include <string.h>


// internal error code if scatter gather table is too short
#define ERR_TOO_BIG	(B_ERRORS_END + 1)


/*! Prepare PIO transfer */
void
prep_PIO_transfer(ide_device_info *device, ide_qrequest *qrequest)
{
	SHOW_FLOW0(4, "");

	device->left_sg_elem = qrequest->request->sg_count;
	device->cur_sg_elem = qrequest->request->sg_list;
	device->cur_sg_ofs = 0;
	device->has_odd_byte = false;
	qrequest->request->data_resid = qrequest->request->data_length;
}


/*! Transfer virtually continuous data */
static inline status_t
transfer_PIO_virtcont(ide_device_info *device, uint8 *virtualAddress,
	int length, bool write, int *transferred)
{
	ide_bus_info *bus = device->bus;
	ide_controller_interface *controller = bus->controller;
	void * channel_cookie = bus->channel_cookie;

	if (write) {
		// if there is a byte left from last chunk, transmit it together
		// with the first byte of the current chunk (IDE requires 16 bits
		// to be transmitted at once)
		if (device->has_odd_byte) {
			uint8 buffer[2];

			buffer[0] = device->odd_byte;
			buffer[1] = *virtualAddress++;

			controller->write_pio(channel_cookie, (uint16 *)buffer, 1, false);

			--length;
			*transferred += 2;
		}

		controller->write_pio(channel_cookie, (uint16 *)virtualAddress,
			length / 2, false);

		// take care if chunk size was odd, which means that 1 byte remains
		virtualAddress += length & ~1;
		*transferred += length & ~1;

		device->has_odd_byte = (length & 1) != 0;

		if (device->has_odd_byte)
			device->odd_byte = *virtualAddress;
	} else {
		// if we read one byte too much last time, push it into current chunk
		if (device->has_odd_byte) {
			*virtualAddress++ = device->odd_byte;
			--length;
		}

		SHOW_FLOW(4, "Reading PIO to %p, %d bytes", virtualAddress, length);

		controller->read_pio(channel_cookie, (uint16 *)virtualAddress,
			length / 2, false);

		// take care of odd chunk size;
		// in this case we read 1 byte to few!
		virtualAddress += length & ~1;
		*transferred += length & ~1;

		device->has_odd_byte = (length & 1) != 0;

		if (device->has_odd_byte) {
			uint8 buffer[2];

			// now read the missing byte; as we have to read 2 bytes at once,
			// we'll read one byte too much
			controller->read_pio(channel_cookie, (uint16 *)buffer, 1, false);

			*virtualAddress = buffer[0];
			device->odd_byte = buffer[1];

			*transferred += 2;
		}
	}

	return B_OK;
}


/*! Transmit physically continuous data */
static inline status_t
transfer_PIO_physcont(ide_device_info *device, addr_t physicalAddress,
	int length, bool write, int *transferred)
{
	// we must split up chunk into B_PAGE_SIZE blocks as we can map only
	// one page into address space at once
	while (length > 0) {
		addr_t virtualAddress;
		void* handle;
		int page_left, cur_len;
		status_t err;
		Thread* thread = thread_get_current_thread();

		SHOW_FLOW(4, "Transmitting to/from physical address %lx, %d bytes left",
			physicalAddress, length);

		thread_pin_to_current_cpu(thread);
		if (vm_get_physical_page_current_cpu(physicalAddress, &virtualAddress,
				&handle) != B_OK) {
			thread_unpin_from_current_cpu(thread);
			// ouch: this should never ever happen
			set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_INTERNAL_FAILURE);
			return B_ERROR;
		}

		// if chunks starts in the middle of a page, we have even less then
		// a page left
		page_left = B_PAGE_SIZE - physicalAddress % B_PAGE_SIZE;

		SHOW_FLOW(4, "page_left=%d", page_left);

		cur_len = min_c(page_left, length);

		SHOW_FLOW(4, "cur_len=%d", cur_len);

		err = transfer_PIO_virtcont(device, (uint8 *)virtualAddress,
			cur_len, write, transferred);

		vm_put_physical_page_current_cpu(virtualAddress, handle);
		thread_unpin_from_current_cpu(thread);

		if (err != B_OK)
			return err;

		length -= cur_len;
		physicalAddress += cur_len;
	}

	return B_OK;
}


/*! Transfer PIO block from/to buffer */
static inline int
transfer_PIO_block(ide_device_info *device, int length, bool write, int *transferred)
{
	// data is usually split up into multiple scatter/gather blocks
	while (length > 0) {
		int left_bytes, cur_len;
		status_t err;

		if (device->left_sg_elem == 0)
			// ups - buffer too small (for ATAPI data, this is OK)
			return ERR_TOO_BIG;

		// we might have transmitted part of a scatter/entry already!
		left_bytes = device->cur_sg_elem->size - device->cur_sg_ofs;

		cur_len = min_c(left_bytes, length);

		err = transfer_PIO_physcont(device,
			(addr_t)device->cur_sg_elem->address + device->cur_sg_ofs,
			cur_len, write, transferred);

		if (err != B_OK)
			return err;

		if (left_bytes <= length) {
			// end of one scatter/gather block reached
			device->cur_sg_ofs = 0;
			++device->cur_sg_elem;
			--device->left_sg_elem;
		} else {
			// still in the same block
			device->cur_sg_ofs += cur_len;
		}

		length -= cur_len;
	}

	return B_OK;
}


/*! Write zero data (required for ATAPI if we ran out of data) */

static void
write_discard_PIO(ide_device_info *device, int length)
{
	ide_bus_info *bus = device->bus;
	uint8 buffer[32];

	memset(buffer, 0, sizeof(buffer));

	// we transmit 32 zero-bytes at once
	// (not very efficient but easy to implement - you get what you deserve
	//  when you don't provide enough buffer)
	while (length > 0) {
		int cur_len;

		// if device asks for odd number of bytes, append an extra byte to
		// make length even (this is the "length + 1" term)
		cur_len = min_c(length + 1, (int)(sizeof(buffer))) / 2;

		bus->controller->write_pio(bus->channel_cookie, (uint16 *)buffer, cur_len, false);

		length -= cur_len * 2;
	}
}


/*! Read PIO data and discard it (required for ATAPI if buffer was too small) */
static void
read_discard_PIO(ide_device_info *device, int length)
{
	ide_bus_info *bus = device->bus;
	uint8 buffer[32];

	// discard 32 bytes at once	(see write_discard_PIO)
	while (length > 0) {
		int cur_len;

		// read extra byte if length is odd (that's the "length + 1")
		cur_len = min_c(length + 1, (int)sizeof(buffer)) / 2;

		bus->controller->read_pio(bus->channel_cookie, (uint16 *)buffer, cur_len, false);

		length -= cur_len * 2;
	}
}


/*! write PIO data
	return: there are 3 possible results
	NO_ERROR - everything's nice and groovy
	ERR_TOO_BIG - data buffer was too short, remaining data got discarded
	B_ERROR - something serious went wrong, sense data was set
*/
status_t
write_PIO_block(ide_qrequest *qrequest, int length)
{
	ide_device_info *device = qrequest->device;
	int transferred;
	status_t err;

	transferred = 0;
	err = transfer_PIO_block(device, length, true, &transferred);

	qrequest->request->data_resid -= transferred;

	if (err != ERR_TOO_BIG)
		return err;

	// there may be a pending odd byte - transmit that now
	if (qrequest->device->has_odd_byte) {
		uint8 buffer[2];

		buffer[0] = device->odd_byte;
		buffer[1] = 0;

		device->has_odd_byte = false;

		qrequest->request->data_resid -= 1;
		transferred += 2;

		device->bus->controller->write_pio(device->bus->channel_cookie, (uint16 *)buffer, 1, false);
	}

	// "transferred" may actually be larger then length because the last odd-byte
	// is sent together with an extra zero-byte
	if (transferred >= length)
		return err;

	// Ouch! the device asks for data but we haven't got any left.
	// Sadly, this behaviour is OK for ATAPI packets, but there is no
	// way to tell the device that we don't have any data left;
	// only solution is to send zero bytes, though it's BAD BAD BAD
	write_discard_PIO(qrequest->device, length - transferred);
	return ERR_TOO_BIG;
}


/*!	read PIO data
	return: see write_PIO_block
*/
status_t
read_PIO_block(ide_qrequest *qrequest, int length)
{
	ide_device_info *device = qrequest->device;
	int transferred;
	status_t err;

	transferred = 0;
	err = transfer_PIO_block(qrequest->device, length, false, &transferred);

	qrequest->request->data_resid -= transferred;

	// if length was odd, there's an extra byte waiting in device->odd_byte
	if (device->has_odd_byte) {
		// discard byte
		device->has_odd_byte = false;
		// adjust res_id as the extra byte didn't reach the buffer
		++qrequest->request->data_resid;
	}

	if (err != ERR_TOO_BIG)
		return err;

	// the device returns more data then the buffer can store;
	// for ATAPI this is OK - we just discard remaining bytes (there
	// is no way to tell ATAPI about that, but we "only" waste time)

	// perhaps discarding the extra odd-byte was sufficient
	if (transferred >= length)
		return err;

	SHOW_FLOW(3, "discarding after %d bytes", transferred);
	read_discard_PIO(qrequest->device, length - transferred);
	return ERR_TOO_BIG;
}
