/*
 * Copyright 2009-2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__DATA_READER_H_
#define _PACKAGE__HPKG__DATA_READER_H_


#include <SupportDefs.h>


class BDataIO;


namespace BPackageKit {

namespace BHPKG {


class BDataReader {
public:
	virtual						~BDataReader();

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size) = 0;
};


class BAbstractBufferedDataReader : public BDataReader {
public:
	virtual						~BAbstractBufferedDataReader();

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);
	virtual	status_t			ReadDataToOutput(off_t offset, size_t size,
									BDataIO* output) = 0;
};


class BFDDataReader : public BDataReader {
public:
								BFDDataReader(int fd);

			void				SetFD(int fd);

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


class BBufferDataReader : public BAbstractBufferedDataReader {
public:
								BBufferDataReader(const void* data, size_t size);

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);
	virtual	status_t			ReadDataToOutput(off_t offset, size_t size,
									BDataIO* output);

private:
			const void*			fData;
			size_t				fSize;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__DATA_READER_H_
