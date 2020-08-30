/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 */

/**	Extra messaging utility functions */

#include <string.h>
#include <ByteOrder.h>

#include <MessageUtils.h>

namespace BPrivate {

uint32
CalculateChecksum(const uint8 *buffer, int32 size)
{
	uint32 sum = 0;
	uint32 temp = 0;

	while (size > 3) {
		sum += B_BENDIAN_TO_HOST_INT32(*(int32 *)buffer);
		buffer += 4;
		size -= 4;
	}

	while (size > 0) {
		temp = (temp << 8) + *buffer++;
		size -= 1;
	}

	return sum + temp;
}


/* entry_ref support functions */
status_t
entry_ref_flatten(char *buffer, size_t *size, const entry_ref *ref)
{
	if (*size < sizeof(ref->device) + sizeof(ref->directory))
		return B_BUFFER_OVERFLOW;

	memcpy((void *)buffer, (const void *)&ref->device, sizeof(ref->device));
	buffer += sizeof(ref->device);
	memcpy((void *)buffer, (const void *)&ref->directory, sizeof(ref->directory));
	buffer += sizeof(ref->directory);
	*size -= sizeof(ref->device) + sizeof(ref->directory);

	size_t nameLength = 0;
	if (ref->name) {
		nameLength = strlen(ref->name) + 1;
		if (*size < nameLength)
			return B_BUFFER_OVERFLOW;

		memcpy((void *)buffer, (const void *)ref->name, nameLength);
	}

	*size = sizeof(ref->device) + sizeof(ref->directory) + nameLength;

	return B_OK;
}


status_t
entry_ref_unflatten(entry_ref *ref, const char *buffer, size_t size)
{
	if (size < sizeof(ref->device) + sizeof(ref->directory)) {
		*ref = entry_ref();
		return B_BAD_VALUE;
	}

	memcpy((void *)&ref->device, (const void *)buffer, sizeof(ref->device));
	buffer += sizeof(ref->device);
	memcpy((void *)&ref->directory, (const void *)buffer,
		sizeof(ref->directory));
	buffer += sizeof(ref->directory);

	if (ref->device != ~(dev_t)0 && size > sizeof(ref->device)
			+ sizeof(ref->directory)) {
		ref->set_name(buffer);
		if (ref->name == NULL) {
			*ref = entry_ref();
			return B_NO_MEMORY;
		}
	} else
		ref->set_name(NULL);

	return B_OK;
}


status_t
entry_ref_swap(char *buffer, size_t size)
{
	if (size < sizeof(dev_t) + sizeof(ino_t))
		return B_BAD_VALUE;

	dev_t *dev = (dev_t *)buffer;
	*dev = B_SWAP_INT32(*dev);
	buffer += sizeof(dev_t);

	ino_t *ino = (ino_t *)buffer;
	*ino = B_SWAP_INT64(*ino);

	return B_OK;
}


/* node_ref support functions */
status_t
node_ref_flatten(char *buffer, size_t *size, const node_ref *ref)
{
	if (*size < sizeof(dev_t) + sizeof(ino_t))
		return B_BUFFER_OVERFLOW;

	memcpy((void *)buffer, (const void *)&ref->device, sizeof(ref->device));
	buffer += sizeof(ref->device);
	memcpy((void *)buffer, (const void *)&ref->node, sizeof(ref->node));
	buffer += sizeof(ref->node);

	return B_OK;
}


status_t
node_ref_unflatten(node_ref *ref, const char *buffer, size_t size)
{
	if (size < sizeof(dev_t) + sizeof(ino_t)) {
		*ref = node_ref();
		return B_BAD_VALUE;
	}

	memcpy((void *)&ref->device, (const void *)buffer, sizeof(dev_t));
	buffer += sizeof(dev_t);
	memcpy((void *)&ref->node, (const void *)buffer, sizeof(ino_t));
	buffer += sizeof(ino_t);

	return B_OK;
}


status_t
node_ref_swap(char *buffer, size_t size)
{
	if (size < sizeof(dev_t) + sizeof(ino_t))
		return B_BAD_VALUE;

	dev_t *dev = (dev_t *)buffer;
	*dev = B_SWAP_INT32(*dev);
	buffer += sizeof(dev_t);

	ino_t *ino = (ino_t *)buffer;
	*ino = B_SWAP_INT64(*ino);

	return B_OK;
}

} // namespace BPrivate
