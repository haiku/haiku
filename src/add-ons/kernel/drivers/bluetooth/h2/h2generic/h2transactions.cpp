/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "h2generic.h"
#include "h2transactions.h"
#include "h2upper.h"
#include "h2util.h"

#include <bluetooth/HCI/btHCI.h>
#include <bluetooth/HCI/btHCI_event.h>
#include <bluetooth/HCI/btHCI_acl.h>

#include <ByteOrder.h>

#include <string.h>

//#define DUMP_BUFFERS
#define BT_DEBUG_THIS_MODULE
#include <btDebug.h>


/* Forward declaration */

#ifndef HAIKU_TARGET_PLATFORM_HAIKU
void acl_tx_complete(void* cookie, uint32 status, void* data, uint32 actual_len);
void acl_rx_complete(void* cookie, uint32 status, void* data, uint32 actual_len);
void command_complete(void* cookie, uint32 status, void* data, uint32 actual_len);
void event_complete(void* cookie, uint32 status, void* data, uint32 actual_len);
#else
void acl_tx_complete(void* cookie, status_t status, void* data, size_t actual_len);
void acl_rx_complete(void* cookie, status_t status, void* data, size_t actual_len);
void command_complete(void* cookie, status_t status, void* data, size_t actual_len);
void event_complete(void* cookie, status_t status, void* data, size_t actual_len);
#endif


static status_t
assembly_rx(bt_usb_dev* bdev, bt_packet_t type, void* data, int count)
{
	bdev->stat.bytesRX += count;

	return btDevices->PostTransportPacket(bdev->hdev, type, data, count);

}


#if 0
#pragma mark --- RX Complete ---
#endif

void
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
event_complete(void* cookie, uint32 status, void* data, uint32 actual_len)
#else
event_complete(void* cookie, status_t status, void* data, size_t actual_len)
#endif
{
	bt_usb_dev* bdev = (bt_usb_dev*)cookie;
	// bt_usb_dev* bdev = fetch_device(cookie, 0); -> safer / slower option
	status_t error;

	debugf("cookie@%p status=%s len=%ld\n", cookie, strerror(status), actual_len);

	if (bdev == NULL)
		return;

	if (status == B_CANCELED || status == B_DEV_CRC_ERROR)
		return; // or not running anymore...

	if (status != B_OK || actual_len == 0)
		goto resubmit;

	if (assembly_rx(bdev, BT_EVENT, data, actual_len) == B_OK) {
		bdev->stat.successfulTX++;
	} else {
		bdev->stat.errorRX++;
	}

resubmit:

	error = usb->queue_interrupt(bdev->intr_in_ep->handle, data,
		max_c(HCI_MAX_EVENT_SIZE, bdev->max_packet_size_intr_in), event_complete, bdev);

	if (error != B_OK) {
		reuse_room(&bdev->eventRoom, data);
		bdev->stat.rejectedRX++;
		debugf("RX event resubmittion failed %s\n", strerror(error));
	} else {
		bdev->stat.acceptedRX++;
	}
}


void
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
acl_rx_complete(void* cookie, uint32 status, void* data, uint32 actual_len)
#else
acl_rx_complete(void* cookie, status_t status, void* data, size_t actual_len)
#endif
{
	bt_usb_dev* bdev = (bt_usb_dev*)cookie;
	// bt_usb_dev* bdev = fetch_device(cookie, 0); -> safer / slower option
	status_t error;

	if (bdev == NULL)
		return;

	if (status == B_CANCELED || status == B_DEV_CRC_ERROR)
		return; // or not running anymore...

	if (status != B_OK || actual_len == 0)
		goto resubmit;

	if (assembly_rx(bdev, BT_ACL, data, actual_len) == B_OK) {
		bdev->stat.successfulRX++;
	} else {
		bdev->stat.errorRX++;
	}

resubmit:

	error = usb->queue_bulk(bdev->bulk_in_ep->handle, data,
		max_c(HCI_MAX_FRAME_SIZE, bdev->max_packet_size_bulk_in),
		acl_rx_complete, (void*) bdev);

	if (error != B_OK) {
		reuse_room(&bdev->aclRoom, data);
		bdev->stat.rejectedRX++;
		debugf("RX acl resubmittion failed %s\n", strerror(error));
	} else {
		bdev->stat.acceptedRX++;
	}
}


#if 0
#pragma mark --- RX ---
#endif

status_t
submit_rx_event(bt_usb_dev* bdev)
{
	size_t size = max_c(HCI_MAX_EVENT_SIZE, bdev->max_packet_size_intr_in);
	void* buf = alloc_room(&bdev->eventRoom, size);
	status_t status;

	if (buf == NULL)
		return ENOMEM;

	status = usb->queue_interrupt(bdev->intr_in_ep->handle,	buf, size,
		event_complete, (void*)bdev);

	if (status != B_OK) {
		reuse_room(&bdev->eventRoom, buf); // reuse allocated one
		bdev->stat.rejectedRX++;
	} else {
		bdev->stat.acceptedRX++;
		debugf("Accepted RX Event %d\n", bdev->stat.acceptedRX);
	}

	return status;
}


status_t
submit_rx_acl(bt_usb_dev* bdev)
{
	size_t size = max_c(HCI_MAX_FRAME_SIZE, bdev->max_packet_size_bulk_in);
	void* buf = alloc_room(&bdev->aclRoom, size);
	status_t status;

	if (buf == NULL)
		return ENOMEM;

	status = usb->queue_bulk(bdev->bulk_in_ep->handle, buf, size,
		acl_rx_complete, bdev);

	if (status != B_OK) {
		reuse_room(&bdev->aclRoom, buf); // reuse allocated
		bdev->stat.rejectedRX++;
	} else {
		bdev->stat.acceptedRX++;
	}

	return status;
}


status_t
submit_rx_sco(bt_usb_dev* bdev)
{
	// not yet implemented
	return B_ERROR;
}


#if 0
#pragma mark --- TX Complete ---
#endif

void
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
command_complete(void* cookie, uint32 status, void* data, uint32 actual_len)
#else
command_complete(void* cookie, status_t status, void* data, size_t actual_len)
#endif
{
	snet_buffer* snbuf = (snet_buffer*)cookie;
	bt_usb_dev* bdev = (bt_usb_dev*)snb_cookie(snbuf);

	debugf("status = %ld len = %ld @%p\n", status, actual_len, data);

	if (status == B_OK) {
		bdev->stat.successfulTX++;
		bdev->stat.bytesTX += actual_len;
	} else {
		bdev->stat.errorTX++;
		// the packet has been lost, too late to requeue it
	}

	snb_park(&bdev->snetBufferRecycleTrash, snbuf);

#ifdef BT_RESCHEDULING_AFTER_COMPLETITIONS
	// TODO: check just the empty queues
	schedTxProcessing(bdev);
#endif
}


void
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
acl_tx_complete(void* cookie, uint32 status, void* data, uint32 actual_len)
#else
acl_tx_complete(void* cookie, status_t status, void* data, size_t actual_len)
#endif
{
	net_buffer* nbuf = (net_buffer*)cookie;
	bt_usb_dev* bdev = GET_DEVICE(nbuf);

	debugf("fetched=%p status=%ld type %lx %p\n", bdev, status, nbuf->type, data);

	if (status == B_OK) {
		bdev->stat.successfulTX++;
		bdev->stat.bytesTX += actual_len;
	} else {
		bdev->stat.errorTX++;
		// the packet has been lost, too late to requeue it
	}

	nb_destroy(nbuf);

#ifdef BT_RESCHEDULING_AFTER_COMPLETITIONS
	schedTxProcessing(bdev);
#endif
}


#if 0
#pragma mark --- TX ---
#endif

status_t
submit_tx_command(bt_usb_dev* bdev, snet_buffer* snbuf)
{
	uint8 bRequestType = bdev->ctrl_req;
	uint8 bRequest = 0;
	uint16 wIndex = 0;
	uint16 value = 0;
	uint16 wLength = B_HOST_TO_LENDIAN_INT16(snb_size(snbuf));
	status_t error;

	if (!GET_BIT(bdev->state, RUNNING)) {
		return B_DEV_NOT_READY;
	}

	// set cookie
	snb_set_cookie(snbuf, bdev);

	debugf("@%p\n", snb_get(snbuf));

	error = usb->queue_request(bdev->dev, bRequestType, bRequest,
		value, wIndex, wLength,	snb_get(snbuf),
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
		wLength,
#endif
		command_complete, (void*) snbuf);

	if (error != B_OK) {
		bdev->stat.rejectedTX++;
	} else {
		bdev->stat.acceptedTX++;
	}

	return error;
}


status_t
submit_tx_acl(bt_usb_dev* bdev, net_buffer* nbuf)
{
	status_t error;

	// set cookie
	SET_DEVICE(nbuf, bdev->hdev);

	if (!GET_BIT(bdev->state, RUNNING)) {
		return B_DEV_NOT_READY;
	}
	/*
	debugf("### Outgoing ACL: len = %ld\n", nbuf->size);
	for (uint32 index = 0 ; index < nbuf->size; index++ ) {
		dprintf("%x:",((uint8*)nb_get_whole_buffer(nbuf))[index]);
	}
	*/

	error = usb->queue_bulk(bdev->bulk_out_ep->handle, nb_get_whole_buffer(nbuf),
		nbuf->size, acl_tx_complete, (void*)nbuf);

	if (error != B_OK) {
		bdev->stat.rejectedTX++;
	} else {
		bdev->stat.acceptedTX++;
	}

	return error;
}


status_t
submit_tx_sco(bt_usb_dev* bdev)
{

	if (!GET_BIT(bdev->state, RUNNING)) {
		return B_DEV_NOT_READY;
	}

	// not yet implemented
	return B_ERROR;
}
