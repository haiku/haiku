/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "ancillary_data.h"

#include <stdlib.h>
#include <string.h>

#include <new>

#include <util/DoublyLinkedList.h>


#define MAX_ANCILLARY_DATA_SIZE	128


struct ancillary_data : DoublyLinkedListLinkImpl<ancillary_data> {
	void* Data()
	{
		return (char*)this + _ALIGN(sizeof(ancillary_data));
	}

	static ancillary_data* FromData(void* data)
	{
		return (ancillary_data*)((char*)data - _ALIGN(sizeof(ancillary_data)));
	}

	ancillary_data_header	header;
	void (*destructor)(const ancillary_data_header*, void*);
};

typedef DoublyLinkedList<ancillary_data> ancillary_data_list;

struct ancillary_data_container {
	ancillary_data_list	data_list;
};


ancillary_data_container*
create_ancillary_data_container()
{
	return new(std::nothrow) ancillary_data_container;
}


void
delete_ancillary_data_container(ancillary_data_container* container)
{
	if (container == NULL)
		return;

	while (ancillary_data* data = container->data_list.RemoveHead()) {
		if (data->destructor != NULL)
			data->destructor(&data->header, data->Data());
		free(data);
	}
}


/*!
	Adds ancillary data to the given container.

	\param container The container.
	\param header Description of the data.
	\param data If not \c NULL, the data are copied into the allocated storage.
	\param destructor If not \c NULL, this function will be invoked with the
		data as parameter when the container is destroyed.
	\param _allocatedData Will be set to the storage allocated for the data.
	\return \c B_OK when everything goes well, another error code otherwise.
*/
status_t
add_ancillary_data(ancillary_data_container* container,
	const ancillary_data_header* header, const void* data,
	void (*destructor)(const ancillary_data_header*, void*),
	void** _allocatedData)
{
	// check parameters
	if (header == NULL)
		return B_BAD_VALUE;

	if (header->len > MAX_ANCILLARY_DATA_SIZE)
		return ENOBUFS;

	// allocate buffer
	void *dataBuffer = malloc(_ALIGN(sizeof(ancillary_data)) + header->len);
	if (dataBuffer == NULL)
		return B_NO_MEMORY;

	// init and attach the structure
	ancillary_data *ancillaryData = new(dataBuffer) ancillary_data;
	ancillaryData->header = *header;
	ancillaryData->destructor = destructor;

	container->data_list.Add(ancillaryData);

	if (data != NULL)
		memcpy(ancillaryData->Data(), data, header->len);

	if (_allocatedData != NULL)
		*_allocatedData = ancillaryData->Data();

	return B_OK;
}


/*!
	Removes ancillary data from the given container. The associated memory is
	freed, i.e. the \a data pointer must no longer be used after calling this
	function. Depending on \a destroy, the destructor is invoked before freeing
	the data.

	\param container The container.
	\param data Pointer to the data to be removed (as returned by
		add_ancillary_data() or next_ancillary_data()).
	\param destroy If \c true, the destructor, if one was passed to
		add_ancillary_data(), is invoked for the data.
	\return \c B_OK when everything goes well, another error code otherwise.
*/
status_t
remove_ancillary_data(ancillary_data_container* container, void* data,
	bool destroy)
{
	if (data == NULL)
		return B_BAD_VALUE;

	ancillary_data *ancillaryData = ancillary_data::FromData(data);

	container->data_list.Remove(ancillaryData);

	if (destroy && ancillaryData->destructor != NULL) {
		ancillaryData->destructor(&ancillaryData->header,
			ancillaryData->Data());
	}

	free(ancillaryData);

	return B_OK;
}


/*!
	Moves all ancillary data from container \c from to the end of the list of
	ancillary data of container \c to.

	\param from The container from which to remove the ancillary data.
	\param to The container to which to add the ancillary data.
	\return A pointer to the first of the moved ancillary data, if any, \c NULL
		otherwise.
*/
void *
move_ancillary_data(ancillary_data_container* from,
	ancillary_data_container* to)
{
	if (from == NULL || to == NULL)
		return NULL;

	ancillary_data *ancillaryData = from->data_list.Head();
	to->data_list.MoveFrom(&from->data_list);

	return ancillaryData != NULL ? ancillaryData->Data() : NULL;
}


/*!
	Returns the next ancillary data. When iterating through the data, initially
	a \c NULL pointer shall be passed as \a previousData, subsequently the
	previously returned data pointer. After the last item, \c NULL is returned.

	Note, that it is not safe to call remove_ancillary_data() for a data item
	and then pass that pointer to this function. First get the next item, then
	remove the previous one.

	\param container The container.
	\param previousData The pointer to the previous data returned by this
		function. Initially \c NULL shall be passed.
	\param header Pointer to allocated storage into which the data description
		is written. May be \c NULL.
	\return A pointer to the next ancillary data in the container. \c NULL after
		the last one.
*/
void*
next_ancillary_data(ancillary_data_container* container, void* previousData,
	ancillary_data_header* _header)
{
	ancillary_data *ancillaryData;

	if (previousData == NULL) {
		ancillaryData = container->data_list.Head();
	} else {
		ancillaryData = ancillary_data::FromData(previousData);
		ancillaryData = container->data_list.GetNext(ancillaryData);
	}

	if (ancillaryData == NULL)
		return NULL;

	if (_header != NULL)
		*_header = ancillaryData->header;

	return ancillaryData->Data();
}
