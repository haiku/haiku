/*
 * Copyright 2007-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */
#ifndef NET_ADDRESS_UTILITIES_H
#define NET_ADDRESS_UTILITIES_H


#include <NetUtilities.h>
#include <net_datalink.h>

#include <sys/socket.h>


class SocketAddress {
public:
	SocketAddress(net_address_module_info* module, sockaddr* address)
		:
		fModule(module),
		fAddress(address)
	{
	}

	SocketAddress(net_address_module_info* module, sockaddr_storage* address)
		:
		fModule(module),
		fAddress((sockaddr*)address)
	{
	}

	SocketAddress(const SocketAddress& address)
		:
		fModule(address.fModule),
		fAddress(address.fAddress)
	{
	}

	void SetAddressTo(sockaddr* address)
	{
		fAddress = address;
	}

	bool IsEmpty(bool checkPort) const
	{
		return fModule->is_empty_address(fAddress, checkPort);
	}

	uint32 HashPair(const sockaddr* second) const
	{
		return fModule->hash_address_pair(fAddress, second);
	}

	bool EqualTo(const sockaddr* address, bool checkPort = false) const
	{
		if (checkPort)
			return fModule->equal_addresses_and_ports(fAddress, address);

		return fModule->equal_addresses(fAddress, address);
	}

	bool EqualTo(const SocketAddress& second, bool checkPort = false) const
	{
		return EqualTo(second.fAddress, checkPort);
	}

	bool EqualPorts(const sockaddr* second) const
	{
		return fModule->equal_ports(fAddress, second);
	}

	bool MatchMasked(const sockaddr* address, const sockaddr* mask) const
	{
		return fModule->equal_masked_addresses(fAddress, address, mask);
	}

	uint16 Port() const
	{
		return fModule->get_port(fAddress);
	}

	status_t SetTo(const sockaddr* from)
	{
		return fModule->set_to(fAddress, from);
	}

	status_t SetTo(const sockaddr_storage* from)
	{
		return SetTo((sockaddr*)from);
	}

	status_t CopyTo(sockaddr* to) const
	{
		return fModule->set_to(to, fAddress);
	}

	status_t CopyTo(sockaddr_storage* to) const
	{
		return CopyTo((sockaddr*)to);
	}

	status_t SetPort(uint16 port)
	{
		return fModule->set_port(fAddress, port);
	}

	status_t SetToEmpty()
	{
		return fModule->set_to_empty_address(fAddress);
	}

	status_t UpdateTo(const sockaddr* from)
	{
		return fModule->update_to(fAddress, from);
	}

	AddressString AsString(bool printPort = false) const
	{
		return AddressString(fModule, fAddress, printPort);
	}

	const char* AsString(char* buffer, size_t bufferSize,
		bool printPort = false) const
	{
		fModule->print_address_buffer(fAddress, buffer, bufferSize, printPort);
		return buffer;
	}

	const sockaddr* operator*() const { return fAddress; }
	sockaddr* operator*() { return fAddress; }

	net_address_module_info* Module() const { return fModule; }

private:
	net_address_module_info*	fModule;
	sockaddr*					fAddress;
};


class ConstSocketAddress {
public:
	ConstSocketAddress(net_address_module_info* module,
		const sockaddr* address)
		:
		fModule(module),
		fAddress(address)
	{
	}

	ConstSocketAddress(net_address_module_info* module,
		const sockaddr_storage* address)
		:
		fModule(module),
		fAddress((sockaddr*)address)
	{
	}

	ConstSocketAddress(const ConstSocketAddress& address)
		:
		fModule(address.fModule),
		fAddress(address.fAddress)
	{
	}

	ConstSocketAddress(const SocketAddress& address)
		:
		fModule(address.Module()),
		fAddress(*address)
	{
	}

	void SetAddressTo(const sockaddr* address)
	{
		fAddress = address;
	}

	bool IsEmpty(bool checkPort) const
	{
		return fModule->is_empty_address(fAddress, checkPort);
	}

	uint32 HashPair(const sockaddr* second) const
	{
		return fModule->hash_address_pair(fAddress, second);
	}

	bool EqualTo(const sockaddr* address, bool checkPort = false) const
	{
		if (checkPort)
			return fModule->equal_addresses_and_ports(fAddress, address);

		return fModule->equal_addresses(fAddress, address);
	}

	bool EqualPorts(const sockaddr* second) const
	{
		return fModule->equal_ports(fAddress, second);
	}

	bool MatchMasked(const sockaddr* address, const sockaddr* mask) const
	{
		return fModule->equal_masked_addresses(fAddress, address, mask);
	}

	uint16 Port() const
	{
		return fModule->get_port(fAddress);
	}

	status_t CopyTo(sockaddr* to) const
	{
		return fModule->set_to(to, fAddress);
	}

	status_t CopyTo(sockaddr_storage* to) const
	{
		return CopyTo((sockaddr*)to);
	}

	AddressString AsString(bool printPort = false) const
	{
		return AddressString(fModule, fAddress, printPort);
	}

	const char* AsString(char* buffer, size_t bufferSize,
		bool printPort = false) const
	{
		fModule->print_address_buffer(fAddress, buffer, bufferSize, printPort);
		return buffer;
	}

	const sockaddr* operator*() const { return fAddress; }

private:
	net_address_module_info*	fModule;
	const sockaddr*				fAddress;
};


class SocketAddressStorage : public SocketAddress {
public:
	SocketAddressStorage(net_address_module_info* module)
		:
		SocketAddress(module, &fAddressStorage)
	{
	}

private:
	sockaddr_storage fAddressStorage;
};


#endif	// NET_ADDRESS_UTILITIES_H
