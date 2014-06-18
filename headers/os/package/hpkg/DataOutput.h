/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__DATA_OUTPUT_H_
#define _PACKAGE__HPKG__DATA_OUTPUT_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


class BDataOutput {
public:
	virtual						~BDataOutput();

	virtual	status_t			WriteData(const void* buffer, size_t size) = 0;
};


class BBufferDataOutput : public BDataOutput {
public:
								BBufferDataOutput(void* buffer, size_t size);

			size_t				BytesWritten() const { return fBytesWritten; }

	virtual	status_t			WriteData(const void* buffer, size_t size);

private:
			void*				fBuffer;
			size_t				fSize;
			size_t				fBytesWritten;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__DATA_OUTPUT_H_
