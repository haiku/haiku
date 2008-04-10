/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_ADDRESS_H
#define UNIX_ADDRESS_H

#include <sys/un.h>

#include <SupportDefs.h>


// NOTE: We support the standard FS address space as well as the alternative
// internal address space Linux features (sun_path[0] is 0, followed by 5 hex
// digits, without null-termination). The latter one is nice to have, because
// the address lookup is quick (hash table lookup, instead of asking the VFS to
// resolve the path), and we don't have to pollute the FS when auto-binding
// sockets (e.g. on connect()).


#define	INTERNAL_UNIX_ADDRESS_LEN	(2 + 1 + 5)
	// sun_len + sun_family + null byte + 5 hex digits


struct vnode;


class UnixAddress {
public:
	UnixAddress()
	{
		Unset();
	}

	UnixAddress(const UnixAddress& other)
	{
		*this = other;
	}

	UnixAddress(int32 internalID)
	{
		SetTo(internalID);
	}

	UnixAddress(dev_t volumeID, ino_t nodeID, struct vnode* vnode)
	{
		SetTo(volumeID, nodeID, vnode);
	}

	void SetTo(int32 internalID)
	{
		fInternalID = internalID;
		fVolumeID = -1;
		fNodeID = -1;
		fVnode = NULL;
	}

	void SetTo(dev_t volumeID, ino_t nodeID, struct vnode* vnode)
	{
		fInternalID = -1;
		fVolumeID = volumeID;
		fNodeID = nodeID;
		fVnode = vnode;
	}

	void Unset()
	{
		fInternalID = -1;
		fVolumeID = -1;
		fNodeID = -1;
		fVnode = NULL;
	}

	bool IsValid() const
	{
		return fInternalID >= 0 || fVolumeID >= 0;
	}

	bool IsInternalAddress() const
	{
		return fInternalID >= 0;
	}

	int32 InternalID() const
	{
		return fInternalID;
	}

	int32 VolumeID() const
	{
		return fVolumeID;
	}

	int32 NodeID() const
	{
		return fNodeID;
	}

	struct vnode* Vnode() const
	{
		return fVnode;
	}

	uint32 HashCode() const
	{
		return fInternalID >= 0
			? fInternalID
			: uint32(fVolumeID) ^ uint32(fNodeID);
	}

	char* ToString(char *buffer, size_t bufferSize) const;

	UnixAddress& operator=(const UnixAddress& other)
	{
		fInternalID = other.fInternalID;
		fVolumeID = other.fVolumeID;
		fNodeID = other.fNodeID;
		fVnode = other.fVnode;
		return *this;
	}

	bool operator==(const UnixAddress& other) const
	{
		return fInternalID >= 0
			? fInternalID == other.fInternalID
			: fVolumeID == other.fVolumeID
				&& fNodeID == other.fNodeID;
	}

	bool operator!=(const UnixAddress& other) const
	{
		return !(*this == other);
	}

	static bool IsEmptyAddress(const sockaddr_un& address)
	{
		return address.sun_len == sizeof(sockaddr)
			&& address.sun_path[0] == '\0' && address.sun_path[1] == '\0';
	}

	static int32 InternalID(const sockaddr_un& address)
	{
		if (address.sun_len < INTERNAL_UNIX_ADDRESS_LEN
				|| address.sun_path[0] != '\0') {
			return B_BAD_VALUE;
		}

		// parse the ID
		int32 id = 0;

		for (int32 i = 0; i < 5; i++) {
			char c = address.sun_path[i + 1];
			if (c >= '0' && c <= '9')
				id = (id << 4) + (c - '0');
			else if (c >= 'a' && c <= 'f')
				id = (id << 4) + 10 + (c - 'a');
			else
				return B_BAD_VALUE;
		}

		return id;
	}

private:
	// fat interface: If fInternalID is >= 0, it's an address in the internal
	// namespace, otherwise a FS address.
	int32			fInternalID;
	dev_t			fVolumeID;
	ino_t			fNodeID;
	struct vnode*	fVnode;
};


#endif	// UNIX_ADDRESS_H
