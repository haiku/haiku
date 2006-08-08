/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_UTILITIES_H
#define NET_UTILITIES_H


#include <net_buffer.h>
#include <net_datalink.h>

#include <stdlib.h>

class Checksum {
	public:
		struct BufferHelper {
			BufferHelper(net_buffer *_buffer, net_buffer_module_info *_bufferModule)
				:	buffer(_buffer),
					bufferModule(_bufferModule)
				{
				}
			net_buffer *buffer;
			net_buffer_module_info *bufferModule;
		};

		Checksum();
	
		Checksum& operator<<(uint8 val);
		Checksum& operator<<(uint16 val);
		Checksum& operator<<(uint32 val);
		Checksum& operator<<(const BufferHelper &bufferHelper);
	
		operator uint16();

	private:	
		uint32 fSum;
};

inline Checksum::Checksum()
	: fSum(0)
{
}

inline Checksum& Checksum::operator<<(uint8 _val) {
#if B_HOST_IS_LENDIAN
	fSum += _val;
#else
	uint16 val = _val;
	fSum += val << 8;
#endif
	return *this;
}

inline Checksum& Checksum::operator<<(uint16 val) {
	fSum += val;
	return *this;
}

inline Checksum& Checksum::operator<<(uint32 val) {
	fSum += (val & 0xFFFF) + (val >> 16);
	return *this;
}

inline Checksum& Checksum::operator<<(const BufferHelper &bufferHelper) {
	net_buffer *buffer = bufferHelper.buffer;
	fSum += bufferHelper.bufferModule->checksum(buffer, 0, buffer->size, false);
	return *this;
}

inline Checksum::operator uint16() {
	while (fSum >> 16) {
		fSum = (fSum & 0xffff) + (fSum >> 16);
	}
	uint16 result = (uint16)fSum;
	result ^= 0xFFFF;
	return result;
}


// helper class that prints an address (and optionally a port) into a buffer that 
// is automatically freed at end of scope:
class AddressString {
public:
	inline AddressString(net_domain *domain, const sockaddr *address, 
		bool printPort = false)
		: fBuffer(NULL)
	{
		domain->address_module->print_address(address, &fBuffer, printPort);
	}

	inline AddressString(net_domain *domain, const sockaddr &address, 
		bool printPort = false)
		: fBuffer(NULL)
	{
		domain->address_module->print_address(&address, &fBuffer, printPort);
	}

	inline ~AddressString()
	{
		free(fBuffer);
	}
	
	inline char *Data()
	{
		return fBuffer;
	}
private:
	char *fBuffer;
};

#endif	// NET_UTILITIES_H
