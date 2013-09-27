/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__DATA_WRITERS_H_
#define _PACKAGE__HPKG__PRIVATE__DATA_WRITERS_H_


#include <package/hpkg/DataOutput.h>
#include <package/hpkg/ZlibCompressor.h>


namespace BPackageKit {

namespace BHPKG {


class BErrorOutput;


namespace BPrivate {


class AbstractDataWriter {
public:
								AbstractDataWriter();
	virtual						~AbstractDataWriter();

			uint64				BytesWritten() const
									{ return fBytesWritten; }

	virtual	status_t			WriteDataNoThrow(const void* buffer,
									size_t size) = 0;

			void				WriteDataThrows(const void* buffer,
									size_t size);

protected:
			uint64				fBytesWritten;
};


class FDDataWriter : public AbstractDataWriter {
public:
								FDDataWriter(int fd, off_t offset,
									BErrorOutput* errorOutput);

	virtual	status_t			WriteDataNoThrow(const void* buffer,
									size_t size);

			off_t				Offset() const
									{ return fOffset; }

private:
			int					fFD;
			off_t				fOffset;
			BErrorOutput*		fErrorOutput;
};


class ZlibDataWriter : public AbstractDataWriter, private BDataOutput {
public:
								ZlibDataWriter(AbstractDataWriter* dataWriter);

			void				Init();
			void				Finish();

	virtual	status_t			WriteDataNoThrow(const void* buffer,
									size_t size);

private:
	// BDataOutput
	virtual	status_t			WriteData(const void* buffer, size_t size);

private:
			AbstractDataWriter*	fDataWriter;
			ZlibCompressor		fCompressor;
};


// inline implementations


inline void
AbstractDataWriter::WriteDataThrows(const void* buffer, size_t size)
{
	status_t error = WriteDataNoThrow(buffer, size);
	if (error != B_OK)
		throw status_t(error);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__DATA_WRITERS_H_
