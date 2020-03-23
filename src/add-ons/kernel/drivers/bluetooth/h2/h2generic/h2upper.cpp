/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "h2upper.h"

#include <string.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/HCI/btHCI_transport.h>
#include <kernel.h>

#include "h2debug.h"
#include "h2generic.h"
#include "h2transactions.h"
#include "snet_buffer.h"


// TODO: split for commands and comunication (ACL & SCO)
void
sched_tx_processing(bt_usb_dev* bdev)
{
	net_buffer* nbuf;
	snet_buffer* snbuf;
	status_t err;

	TRACE("%s: (%p)\n", __func__, bdev);

	if (!TEST_AND_SET(&bdev->state, PROCESSING)) {
		// We are not processing in another thread so... START!!

		do {
			/* Do while this bit is on... so someone should set it before we
			 * stop the iterations
			 */
			bdev->state = CLEAR_BIT(bdev->state, SENDING);
			// check Commands
	#ifdef EMPTY_COMMAND_QUEUE
			while (!list_is_empty(&bdev->nbuffersTx[BT_COMMAND])) {
	#else
			if (!list_is_empty(&bdev->nbuffersTx[BT_COMMAND])) {
	#endif
				snbuf = (snet_buffer*)
					list_remove_head_item(&bdev->nbuffersTx[BT_COMMAND]);
				err = submit_tx_command(bdev, snbuf);
				if (err != B_OK) {
					// re-head it
					list_insert_item_before(&bdev->nbuffersTx[BT_COMMAND],
						list_get_first_item(&bdev->nbuffersTx[BT_COMMAND]),
						snbuf);
				}
			}

			// check ACl
	#define EMPTY_ACL_QUEUE
	#ifdef EMPTY_ACL_QUEUE
			while (!list_is_empty(&bdev->nbuffersTx[BT_ACL])) {
	#else
			if (!list_is_empty(&bdev->nbuffersTx[BT_ACL])) {
	#endif
				nbuf = (net_buffer*)
					list_remove_head_item(&bdev->nbuffersTx[BT_ACL]);
				err = submit_tx_acl(bdev, nbuf);
				if (err != B_OK) {
					// re-head it
					list_insert_item_before(&bdev->nbuffersTx[BT_ACL],
							list_get_first_item(&bdev->nbuffersTx[BT_ACL]),
							nbuf);
				}
			}

			if (!list_is_empty(&bdev->nbuffersTx[BT_SCO])) {
				// TODO to be implemented
			}

		} while (GET_BIT(bdev->state, SENDING));

		bdev->state = CLEAR_BIT(bdev->state, PROCESSING);

	} else {
		// We are processing so MARK that we need to still go on with that
		bdev->state = SET_BIT(bdev->state, SENDING);
	}
}


#if 0
// DEPRECATED
status_t
post_packet_up(bt_usb_dev* bdev, bt_packet_t type, void* buf)
{
	status_t err = B_ERROR;

	debugf("Frame up type=%d\n", type);

	if (type == BT_EVENT) {
		snet_buffer* snbuf = (snet_buffer*)buf;
		btCoreData->PostEvent(bdev->ndev, snb_get(snbuf),
			(size_t)snb_size(snbuf));
		snb_park(&bdev->snetBufferRecycleTrash, snbuf);
		debugf("to btDataCore len=%d\n", snb_size(snbuf));
	} else {
		net_buffer* nbuf = (net_buffer*) buf;
		// No need to free the buffer at allocation is gonna be reused
		btDevices->receive_data(bdev->ndev, &nbuf);
		TRACE("to net_device\n");
	}

	return err;
}
#endif


status_t
send_packet(hci_id hid, bt_packet_t type, net_buffer* nbuf)
{
	bt_usb_dev* bdev = fetch_device(NULL, hid);
	status_t err = B_OK;

	if (bdev == NULL)
		return B_ERROR;

	// TODO: check if device is actually ready for this
	// TODO: Lock Device

	if (nbuf != NULL) {
		if (type != nbuf->protocol) // a bit strict maybe
			panic("Upper layer has not filled correctly a packet");

		switch (type) {
			case BT_COMMAND:
			case BT_ACL:
			case BT_SCO:
				list_add_item(&bdev->nbuffersTx[type], nbuf);
				bdev->nbuffersPendingTx[type]++;
			break;
			default:
				ERROR("%s: Unknown packet type for sending %d\n", __func__,
					type);
				// TODO: free the net_buffer -> no, allow upper layer
				// handle it with the given error
				err = B_BAD_VALUE;
			break;
		}
	} else {
		TRACE("%s: tx sched provoked", __func__);
	}

	// TODO: check if device is actually ready for this
	// TODO: unlock device

	// sched in any case even if nbuf is null (provoke re-scheduling)
	sched_tx_processing(bdev);

	return err;
}


status_t
send_command(hci_id hid, snet_buffer* snbuf)
{
	bt_usb_dev* bdev = fetch_device(NULL, hid);
	status_t err = B_OK;

	if (bdev == NULL)
		return B_ERROR;

	// TODO: check if device is actually ready for this
	// TODO: mutex

	if (snbuf != NULL) {
		list_add_item(&bdev->nbuffersTx[BT_COMMAND], snbuf);
		bdev->nbuffersPendingTx[BT_COMMAND]++;
	} else {
		err = B_BAD_VALUE;
		TRACE("%s: tx sched provoked", __func__);
	}

	// TODO: check if device is actually ready for this
	// TODO: mutex

	/* sched in All cases even if nbuf is null (hidden way to provoke
	 * re-scheduling)
	 */
	sched_tx_processing(bdev);

	return err;
}
