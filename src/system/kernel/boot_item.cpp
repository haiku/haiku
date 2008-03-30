/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <boot_item.h>
#include <util/DoublyLinkedList.h>
#include <util/kernel_cpp.h>

#include <string.h>


// ToDo: the boot items are not supposed to be changed after kernel startup
//		so no locking is done. So for now, we need to be careful with adding
//		new items.

struct boot_item : public DoublyLinkedListLinkImpl<boot_item> {
	const char	*name;
	void		*data;
	size_t		size;
};

typedef DoublyLinkedList<boot_item> ItemList;


static ItemList sItemList;


status_t
add_boot_item(const char *name, void *data, size_t size)
{
	boot_item *item = new(nothrow) boot_item;
	if (item == NULL)
		return B_NO_MEMORY;

	item->name = name;
	item->data = data;
	item->size = size;

	sItemList.Add(item);
	return B_OK;
}


void *
get_boot_item(const char *name, size_t *_size)
{
	if (name == NULL || name[0] == '\0')
		return NULL;

	// search item
	for (ItemList::Iterator it = sItemList.GetIterator(); it.HasNext();) {
		boot_item *item = it.Next();

		if (!strcmp(name, item->name)) {
			if (_size != NULL)
				*_size = item->size;

			return item->data;
		}
	}

	return NULL;
}


status_t
boot_item_init(void)
{
	new(&sItemList) ItemList;
		// static initializers do not work in the kernel,
		// so we have to do it here, manually

	return B_OK;
}
