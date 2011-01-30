/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__DATA_READER_H_
#define _PACKAGE__HPKG__DATA_READER_H_


#include <SupportDefs.h>


namespace BPackageKit {

namespace BHPKG {


class BDataReader {
public:
	virtual						~BDataReader();

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size) = 0;
};


class BFDDataReader : public BDataReader {
public:
								BFDDataReader(int fd);

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);

private:
			int					fFD;
};


class BAttributeDataReader : public BDataReader {
public:
								BAttributeDataReader(int fd,
									const char* attribute, uint32 type);

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);

private:
			int					fFD;
			uint32				fType;
			const char*			fAttribute;
};


class BBufferDataReader : public BDataReader {
public:
								BBufferDataReader(const void* data, size_t size);

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);

private:
			const void*			fData;
			size_t				fSize;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__DATA_READER_H_
