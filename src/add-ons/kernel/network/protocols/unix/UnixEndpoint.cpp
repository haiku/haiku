/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <new>

#include "UnixEndpoint.h"

#include "UnixAddressManager.h"
#include "UnixDatagramEndpoint.h"
#include "UnixStreamEndpoint.h"


#define UNIX_ENDPOINT_DEBUG_LEVEL	1
#define UNIX_DEBUG_LEVEL    		UNIX_ENDPOINT_DEBUG_LEVEL
#include "UnixDebug.h"


status_t
UnixEndpoint::Create(net_socket* socket, UnixEndpoint** _endpoint)
{
	TRACE("[%" B_PRId32 "] UnixEndpoint::Create(%p, %p)\n", find_thread(NULL),
		socket, _endpoint);

	if (socket == NULL || _endpoint == NULL)
		return B_BAD_ADDRESS;

	switch (socket->type) {
		case SOCK_STREAM:
			*_endpoint = new(std::nothrow) UnixStreamEndpoint(socket, false);
			break;
		case SOCK_DGRAM:
			*_endpoint = new(std::nothrow) UnixDatagramEndpoint(socket);
			break;
		case SOCK_SEQPACKET:
			*_endpoint = new(std::nothrow) UnixStreamEndpoint(socket, true);
			break;
		default:
			return EPROTOTYPE;
	}

	return *_endpoint == NULL ? B_NO_MEMORY : B_OK;
}


UnixEndpoint::UnixEndpoint(net_socket* socket)
	:
	ProtocolSocket(socket),
	fAddress(),
	fAddressHashLink()
{
	TRACE("[%" B_PRId32 "] %p->UnixEndpoint::UnixEndpoint()\n",
		find_thread(NULL), this);

	mutex_init(&fLock, "unix endpoint");
}


UnixEndpoint::~UnixEndpoint()
{
	TRACE("[%" B_PRId32 "] %p->UnixEndpoint::UnixEndpoint()\n",
		find_thread(NULL), this);

	mutex_destroy(&fLock);
}


status_t
UnixEndpoint::_Bind(const struct sockaddr_un* address)
{
	TRACE("[%" B_PRId32 "] %p->UnixEndpoint::_Bind(\"%s\")\n",
		find_thread(NULL), this,
		ConstSocketAddress(&gAddressModule, (struct sockaddr*)address).AsString().Data());

	if (address->sun_path[0] == '\0') {
		UnixAddressManagerLocker addressLocker(gAddressManager);

		// internal address space (or empty address)
		int32 internalID;
		if (UnixAddress::IsEmptyAddress(*address))
			internalID = gAddressManager.NextUnusedInternalID();
		else
			internalID = UnixAddress::InternalID(*address);
		if (internalID < 0)
			RETURN_ERROR(internalID);

		status_t error = _Bind(internalID);
		if (error != B_OK)
			RETURN_ERROR(error);

		sockaddr_un* outAddress = (sockaddr_un*)&socket->address;
		outAddress->sun_path[0] = '\0';
		sprintf(outAddress->sun_path + 1, "%05" B_PRIx32, internalID);
		outAddress->sun_len = INTERNAL_UNIX_ADDRESS_LEN;
			// null-byte + 5 hex digits

		gAddressManager.Add(this);
	} else {
		// FS address space
		size_t pathLen = strnlen(address->sun_path, sizeof(address->sun_path));
		if (pathLen == 0 || pathLen == sizeof(address->sun_path))
			RETURN_ERROR(B_BAD_VALUE);

		struct vnode* vnode;
		status_t error = vfs_create_special_node(address->sun_path,
			NULL, S_IFSOCK | 0644, 0, !gStackModule->is_syscall(), NULL,
			&vnode);
		if (error != B_OK)
			RETURN_ERROR(error == B_FILE_EXISTS ? EADDRINUSE : error);

		error = _Bind(vnode);
		if (error != B_OK) {
			vfs_put_vnode(vnode);
			RETURN_ERROR(error);
		}

		size_t addressLen = address->sun_path + pathLen + 1 - (char*)address;
		memcpy(&socket->address, address, addressLen);
		socket->address.ss_len = addressLen;

		UnixAddressManagerLocker addressLocker(gAddressManager);
		gAddressManager.Add(this);
	}

	RETURN_ERROR(B_OK);
}


status_t
UnixEndpoint::_Bind(struct vnode* vnode)
{
	struct stat st;
	status_t error = vfs_stat_vnode(vnode, &st);
	if (error != B_OK)
		RETURN_ERROR(error);

	fAddress.SetTo(st.st_dev, st.st_ino, vnode);
	RETURN_ERROR(B_OK);
}


status_t
UnixEndpoint::_Bind(int32 internalID)
{
	fAddress.SetTo(internalID);
	RETURN_ERROR(B_OK);
}


status_t
UnixEndpoint::_Unbind()
{
	UnixAddressManagerLocker addressLocker(gAddressManager);
	gAddressManager.Remove(this);
	if (struct vnode* vnode = fAddress.Vnode())
		vfs_put_vnode(vnode);

	fAddress.Unset();
	RETURN_ERROR(B_OK);
}
