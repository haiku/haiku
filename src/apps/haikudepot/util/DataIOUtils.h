/*
 * Copyright 2018-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef DATA_IO_UTILS_H
#define DATA_IO_UTILS_H


#include <DataIO.h>


class DataIOUtils {
public:
	static	status_t			CopyAll(BDataIO* target, BDataIO* source);
};


/*!	This is a data source (read only) that is restricted to a certain size. An
	example of where this is used is with reading out the data from a tar
	stream.  The tar logic knows how long the data is and so is able to provide
	a client this constrained view of the data that is only able to read a
	constrained quantity of data from the delegate data source.  This prevents
	overflow reads from the tar file.
*/

class ConstraintedDataIO : public BDataIO {
public:
								ConstraintedDataIO(BDataIO* delegate,
									size_t limit);
	virtual						~ConstraintedDataIO();

	virtual	ssize_t				Read(void* buffer, size_t size);
	virtual	ssize_t				Write(const void* buffer, size_t size);

	virtual	status_t			Flush();

private:
			BDataIO*			fDelegate;
			size_t				fLimit;
};


class Base64DecodingDataIO : public BDataIO {
public:
								Base64DecodingDataIO(BDataIO* delegate,
									char char62 = '+', char char63 = '/');
	virtual						~Base64DecodingDataIO();

	virtual	ssize_t				Read(void* buffer, size_t size);
	virtual	ssize_t				Write(const void* buffer, size_t size);

	virtual	status_t			Flush();

private:
			status_t			_ReadSingleByte(void* buffer);
			status_t			_CharToInt(uint8 ch, uint8* value);

private:
			BDataIO*			fDelegate;
			char				fChar62;
			char				fChar63;

			uint8				fNextByteAssembly;
			uint8				fNextByteAssemblyBits;
};


#endif // DATA_IO_UTILS_H
