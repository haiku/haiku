/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_HEX_DUMP_H
#define _KERNEL_DEBUG_HEX_DUMP_H


#include <debug.h>


namespace BKernel {


enum {
	HEX_DUMP_FLAG_OMIT_ADDRESS	= 0x01
};


class HexDumpDataProvider {
public:
	virtual						~HexDumpDataProvider();

	virtual	bool				HasMoreData() const = 0;
	virtual	uint8				NextByte() = 0;
	virtual	bool				GetAddressString(char* buffer,
									size_t bufferSize) const;
};


class HexDumpBufferDataProvider : public HexDumpDataProvider {
public:
								HexDumpBufferDataProvider(const void* data,
									size_t dataSize);

	virtual	bool				HasMoreData() const;
	virtual	uint8				NextByte();
	virtual	bool				GetAddressString(char* buffer,
									size_t bufferSize) const;

private:
			const uint8*		fData;
			size_t				fDataSize;
};


void	print_hex_dump(HexDumpDataProvider& data, size_t maxBytes,
			uint32 flags = 0);
void	print_hex_dump(const void* data, size_t maxBytes, uint32 flags = 0);


}	// namespace BKernel


#endif	/* _KERNEL_DEBUG_HEX_DUMP_H */
