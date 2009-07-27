/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_ADDRESS_MANAGER_H
#define UNIX_ADDRESS_MANAGER_H

#include <lock.h>
#include <util/OpenHashTable.h>

#include "UnixAddress.h"
#include "UnixEndpoint.h"


struct UnixAddressHashDefinition {
	typedef UnixAddress		KeyType;
	typedef UnixEndpoint	ValueType;

	size_t HashKey(const KeyType& key) const
	{
		return key.HashCode();
	}

	size_t Hash(UnixEndpoint* endpoint) const
	{
		return HashKey(endpoint->Address());
	}

	bool Compare(const KeyType& key, UnixEndpoint* endpoint) const
	{
		return key == endpoint->Address();
	}

	UnixEndpoint*& GetLink(UnixEndpoint* endpoint) const
	{
		return endpoint->HashTableLink();
	}
};


class UnixAddressManager {
public:
	UnixAddressManager()
	{
		mutex_init(&fLock, "unix address manager");
	}

	~UnixAddressManager()
	{
		mutex_destroy(&fLock);
	}

	status_t Init()
	{
		return fBoundEndpoints.Init();
	}

	bool Lock()
	{
		return mutex_lock(&fLock) == B_OK;
	}

	void Unlock()
	{
		mutex_unlock(&fLock);
	}

	UnixEndpoint* Lookup(const UnixAddress& address) const
	{
		return fBoundEndpoints.Lookup(address);
	}

	void Add(UnixEndpoint* endpoint)
	{
		fBoundEndpoints.Insert(endpoint);
	}

	void Remove(UnixEndpoint* endpoint)
	{
		fBoundEndpoints.Remove(endpoint);
	}

	int32 NextInternalID()
	{
		int32 id = fNextInternalID;
		fNextInternalID = (id + 1) & 0xfffff;
		return id;
	}

	int32 NextUnusedInternalID()
	{
		for (int32 i = 0xfffff; i >= 0; i--) {
			int32 id = NextInternalID();
			UnixAddress address(id);
			if (Lookup(address) == NULL)
				return id;
		}

		return ENOBUFS;
	}

private:
	typedef BOpenHashTable<UnixAddressHashDefinition, false> EndpointTable;

	mutex			fLock;
	EndpointTable	fBoundEndpoints;
	int32			fNextInternalID;
};


typedef AutoLocker<UnixAddressManager> UnixAddressManagerLocker;


#endif	// UNIX_ADDRESS_MANAGER_H
