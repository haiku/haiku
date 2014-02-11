/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__DATA_OUTPUT_H_
#define _PACKAGE__HPKG__DATA_OUTPUT_H_


#include <DataIO.h>
#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


class BBufferDataOutput : public BDataIO {
public:
								BBufferDataOutput(void* buffer, size_t size);

			size_t				BytesWritten() const { return fBytesWritten; }

	virtual	ssize_t				Write(const void* buffer, size_t size);
	virtual ssize_t				Read(void* buffer, size_t size)
									{ return B_NOT_SUPPORTED; }

private:
			void*				fBuffer;
			size_t				fSize;
			size_t				fBytesWritten;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__DATA_OUTPUT_H_
