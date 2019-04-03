/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>
#include <util/kernel_cpp.h>

#include <string.h>


/** Gets all device types of the specified type by doing a 
 *	depth-first search of the OpenFirmware device tree.
 *	If a root != 0 is given, the function only traverses the subtree spanned
 *	by the root (inclusively). Otherwise the whole device tree is searched.
 *
 *	The cookie has to be initialized to zero.
 */
status_t
of_get_next_device(intptr_t *_cookie, intptr_t root, const char *type,
	char *path, size_t pathSize)
{
	intptr_t node = *_cookie;

	while (true) {
		intptr_t next;

		if (node == 0) {
			// node is NULL, meaning that this is the initial function call.
			// If a root was supplied, we take that, otherwise the device tree
			// root.
			if (root != 0)
				node = root;
			else
				node = of_peer(0);

			if (node == OF_FAILED)
				return B_ERROR;
			if (node == 0)
				return B_ENTRY_NOT_FOUND;

			// We want to visit the root first.
			next = node;				
		} else
			next = of_child(node);

		if (next == OF_FAILED)
			return B_ERROR;

		if (next == 0) {
			// no child node found
			next = of_peer(node);
			if (next == OF_FAILED)
				return B_ERROR;

			while (next == 0) {
				// no peer node found, we are using the device
				// tree itself as our search stack

				next = of_parent(node);
				if (next == OF_FAILED)
					return B_ERROR;

				if (next == root || next == 0) {
					// We have searched the whole device tree
					return B_ENTRY_NOT_FOUND;
				}

				// look into the next tree
				node = next;
				next = of_peer(node);
			}
		}

		*_cookie = node = next;

		char nodeType[16];
		int length;
		if (of_getprop(node, "device_type", nodeType, sizeof(nodeType))
				== OF_FAILED
			|| strcmp(nodeType, type)
			|| (length = of_package_to_path(node, path, pathSize - 1))
					== OF_FAILED) {
			continue;
		}

		path[length] = '\0';
		return B_OK;
	}
}

