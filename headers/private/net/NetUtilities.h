/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_UTILITIES_H
#define NET_UTILITIES_H


#include <netinet/in.h>
#include <stdlib.h>

#include <net_buffer.h>
#include <net_datalink.h>


class Checksum {
public:
	struct BufferHelper {
		BufferHelper(net_buffer* _buffer, net_buffer_module_info* _bufferModule)
			:
			buffer(_buffer),
			bufferModule(_bufferModule)
		{
		}
		
		net_buffer* buffer;
		net_buffer_module_info* bufferModule;
	};

	Checksum()
		:
		fSum(0)
	{
	}

	inline Checksum& operator<<(uint8 value)
	{
#if B_HOST_IS_LENDIAN
		fSum += value;
#else
		fSum += (uint16)value << 8;
#endif
		return *this;
	}

	inline Checksum& operator<<(uint16 value)
	{
		fSum += value;
		return *this;
	}

	inline Checksum& operator<<(uint32 value)
	{
		fSum += (value & 0xffff) + (value >> 16);
		return *this;
	}

	inline Checksum& operator<<(const BufferHelper& bufferHelper)
	{
		net_buffer* buffer = bufferHelper.buffer;
		fSum += bufferHelper.bufferModule->checksum(buffer, 0, buffer->size,
			false);
		return *this;
	}

	inline operator uint16()
	{
		while (fSum >> 16) {
			fSum = (fSum & 0xffff) + (fSum >> 16);
		}
		uint16 result = (uint16)fSum;
		result ^= 0xFFFF;
		return result;
	}

	static uint16 PseudoHeader(net_address_module_info* addressModule,
		net_buffer_module_info* bufferModule, net_buffer* buffer,
		uint16 protocol);

private:
	uint32 fSum;
};


inline uint16
Checksum::PseudoHeader(net_address_module_info* addressModule,
	net_buffer_module_info* bufferModule, net_buffer* buffer, uint16 protocol)
{
	Checksum checksum;
	addressModule->checksum_address(&checksum, buffer->source);
	addressModule->checksum_address(&checksum, buffer->destination);
	checksum << (uint16)htons(protocol) << (uint16)htons(buffer->size)
		<< Checksum::BufferHelper(buffer, bufferModule);
	return checksum;
}


/*!	Helper class that prints an address (and optionally a port) into a buffer
	that is automatically freed at end of scope.
*/
class AddressString {
public:
	AddressString(net_domain* domain, const sockaddr* address,
		bool printPort = false)
		:
		fBuffer(NULL)
	{
		domain->address_module->print_address(address, &fBuffer, printPort);
	}

	AddressString(net_domain* domain, const sockaddr& address,
		bool printPort = false)
		:
		fBuffer(NULL)
	{
		domain->address_module->print_address(&address, &fBuffer, printPort);
	}

	AddressString(net_address_module_info* address_module,
		const sockaddr* address, bool printPort = false)
		:
		fBuffer(NULL)
	{
		address_module->print_address(address, &fBuffer, printPort);
	}

	~AddressString()
	{
		free(fBuffer);
	}

	const char* Data() const
	{
		return fBuffer;
	}

private:
	char* fBuffer;
};


#endif	// NET_UTILITIES_H
