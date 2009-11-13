/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef DATA_READER_H
#define DATA_READER_H


#include <SupportDefs.h>


class DataReader {
public:
	virtual						~DataReader();

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size) = 0;
};


class FDDataReader : public DataReader {
public:
								FDDataReader(int fd);

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);

private:
			int					fFD;
};


class BufferDataReader : public DataReader {
public:
								BufferDataReader(const void* data, size_t size);

	virtual	status_t			ReadData(off_t offset, void* buffer,
									size_t size);

private:
			const void*			fData;
			size_t				fSize;
};


#endif	// DATA_READER_H
