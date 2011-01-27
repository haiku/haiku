/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_OUTPUT_H
#define DATA_OUTPUT_H


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


class DataOutput {
public:
	virtual						~DataOutput();

	virtual	status_t			WriteData(const void* buffer, size_t size) = 0;
};


class BufferDataOutput : public DataOutput {
public:
								BufferDataOutput(void* buffer, size_t size);

			size_t				BytesWritten() const { return fBytesWritten; }

	virtual	status_t			WriteData(const void* buffer, size_t size);

private:
			void*				fBuffer;
			size_t				fSize;
			size_t				fBytesWritten;
};


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit


#endif	// DATA_OUTPUT_H
