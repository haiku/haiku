/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * Copyright 2008 Mika Lindqvist, monni1995_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <malloc.h>

#include "h2upper.h"
#include "h2util.h"
#include "h2transactions.h"

#include <bluetooth/HCI/btHCI_acl.h>
#include <bluetooth/HCI/btHCI_command.h>
#include <bluetooth/HCI/btHCI_event.h>
#include <bluetooth/HCI/btHCI_sco.h>

#define BT_DEBUG_THIS_MODULE
#include <btDebug.h>


void*
nb_get_whole_buffer(net_buffer* nbuf)
{
	void* conPointer;
	status_t err;
#if 0
	// the job could be already done
	// !!! it could be trash from other upper protocols...
	if (nbuf->COOKIEFIELD != NULL)
		return (void*)nbuf->COOKIEFIELD;
#endif
	err = nb->direct_access(nbuf, 0, nbuf->size, &conPointer);

	if (err != B_OK) {
		panic("expected to be contiguous");
		#if 0
		// We are gonna need a realocation
		nbuf->COOKIEFIELD = (uint32) malloc(nbuf->size);
		if (nbuf->COOKIEFIELD == NULL)
			goto fail;

		err = nb->write(nbuf, 0, (void*) nbuf->COOKIEFIELD, nbuf->size);
		if (err != B_OK)
			goto free;

		conPointer = (void*)nbuf->COOKIEFIELD;
		#endif
	}

	return conPointer;
#if 0
free:
	free((void*) nbuf->COOKIEFIELD);
fail:
	return NULL;
#endif
}


void
nb_destroy(net_buffer* nbuf)
{
	if (nbuf == NULL)
		return;
#if 0
	// Free possible allocated
	if (nbuf->COOKIEFIELD != NULL)
		free((void*)nbuf->COOKIEFIELD);
#endif
	// TODO check for survivers...
	if (nb != NULL)
		nb->free(nbuf);

}


// Extract the expected size of the packet
// TODO: This might be inefficient as at the moment of the creation
// of the net_buffer this information is known and it could be stored 
#if 0
ssize_t
get_expected_size(net_buffer* nbuf)
{

	if (nbuf == NULL)
		panic("Analizing NULL packet");

	switch (nbuf->protocol) {

		case BT_COMMAND: {
			struct hci_command_header* header = nb_get_whole_buffer(nbuf);
			return header->clen + sizeof(struct hci_command_header);
		}

		case BT_EVENT: {
			struct hci_event_header* header = nb_get_whole_buffer(nbuf);
			return header->elen + sizeof(struct hci_event_header);
		}

		case BT_ACL: {
			struct hci_acl_header* header = nb_get_whole_buffer(nbuf);
			return header->alen + sizeof(struct hci_acl_header);
		}

		case BT_SCO: {
			struct hci_sco_header* header = nb_get_whole_buffer(nbuf);
			return header->slen + sizeof(struct hci_sco_header);
		}

		default:
			panic(BLUETOOTH_DEVICE_DEVFS_NAME ":no protocol specified for ");
		break;
	}

	return -1;
}
#endif


#if 0
#pragma mark - room util -
#endif


void
init_room(struct list* l)
{
	list_init(l);
}


void*
alloc_room(struct list* l, size_t size)
{

	void* item = list_get_first_item(l);

	if (item == NULL)
		item = (void*) malloc(size);

	return item;

}


void
reuse_room(struct list* l, void* room)
{
	list_add_item(l, room);
}


void
purge_room(struct list* l)
{
	void* item;

	while ((item = list_remove_head_item(l)) != NULL) {
		free(item);
	}
}
