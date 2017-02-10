//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file SimulatedStream.cpp
*/

#include "SimulatedStream.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "UdfDebug.h"

SimulatedStream::SimulatedStream(DataStream &stream)
	: fPosition(0)
	, fStream(stream)
{
}

status_t
SimulatedStream::InitCheck() const
{
	return fStream.InitCheck();
}

ssize_t
SimulatedStream::Read(void *_buffer, size_t size)
{
	uint8 *buffer = reinterpret_cast<uint8*>(_buffer);
	status_t error = buffer ? B_OK : B_BAD_VALUE;
	ssize_t bytesTotal = 0;
	while (error == B_OK && size > 0) {
		data_extent extent;
		error = _GetExtent(fPosition, size, extent);
		if (!error) {
			if (extent.size > 0) {
				ssize_t bytes = fStream.ReadAt(extent.offset, buffer, extent.size);
				if (bytes >= 0) {
					size -= bytes;
					fPosition += bytes;
					buffer += bytes;
					bytesTotal += bytes;
				} else {
					error = status_t(bytes);
				}
			} else {
				// end of simulated stream
				break;
			}
		}		
	}
	return !error ? bytesTotal : ssize_t(error);
}

ssize_t
SimulatedStream::ReadAt(off_t pos, void *_buffer, size_t size)
{
	uint8 *buffer = reinterpret_cast<uint8*>(_buffer);
	status_t error = buffer ? B_OK : B_BAD_VALUE;
	ssize_t bytesTotal = 0;
	while (error == B_OK && size > 0) {
		data_extent extent;
		error = _GetExtent(pos, size, extent);
		if (!error) {
			if (extent.size > 0) {
				ssize_t bytes = fStream.ReadAt(extent.offset, buffer, extent.size);
				if (bytes >= 0) {
					size -= bytes;
					pos += bytes;
					buffer += bytes;
					bytesTotal += bytes;
				} else {
					error = status_t(bytes);
				}
			} else {
				// end of simulated stream
				break;
			}
		}		
	}
	return !error ? bytesTotal : ssize_t(error);
}

/*! \brief Writes \a size bytes worth of data from \a buffer at the current
	position in the stream, incrementing the stream's position marker as it goes.
*/
ssize_t
SimulatedStream::Write(const void *_buffer, size_t size)
{
	const uint8 *buffer = reinterpret_cast<const uint8*>(_buffer);
	status_t error = buffer ? B_OK : B_BAD_VALUE;
	ssize_t bytesTotal = 0;
	while (error == B_OK && size > 0) {
		data_extent extent;
		error = _GetExtent(fPosition, size, extent);
		if (!error) {
			if (extent.size > 0) {
				ssize_t bytes = fStream.WriteAt(extent.offset, buffer, extent.size);
				if (bytes >= 0) {
					size -= bytes;
					fPosition += bytes;
					buffer += bytes;
					bytesTotal += bytes;
				} else {
					error = status_t(bytes);
				}
			} else {
				// end of simulated stream
				break;
			}
		}		
	}
	return !error ? bytesTotal : ssize_t(error);
} 

/*! \brief Writes \a size bytes worth of data from \a buffer at position
	\a pos in the stream without incrementing the stream's position marker.
*/
ssize_t
SimulatedStream::WriteAt(off_t pos, const void *_buffer, size_t size)
{
	const uint8 *buffer = reinterpret_cast<const uint8*>(_buffer);
	status_t error = buffer ? B_OK : B_BAD_VALUE;
	ssize_t bytesTotal = 0;
	while (error == B_OK && size > 0) {
		data_extent extent;
		error = _GetExtent(pos, size, extent);
		if (!error) {
			if (extent.size > 0) {
				ssize_t bytes = fStream.WriteAt(extent.offset, buffer, extent.size);
				if (bytes >= 0) {
					size -= bytes;
					pos += bytes;
					buffer += bytes;
					bytesTotal += bytes;
				} else {
					error = status_t(bytes);
				}
			} else {
				// end of simulated stream
				break;
			}
		}		
	}
	return !error ? bytesTotal : ssize_t(error);
}

/*! \brief Writes \a size bytes worth of data from \a data at the current
	position in the stream, incrementing the stream's position marker as it goes.
*/
ssize_t
SimulatedStream::Write(BDataIO &data, size_t size)
{
	DEBUG_INIT_ETC("SimulatedStream", ("size: %ld", size));
	status_t error = B_OK;
	ssize_t bytesTotal = 0;
	while (error == B_OK && size > 0) {
		data_extent extent;
		error = _GetExtent(fPosition, size, extent);
		if (!error) {
			if (extent.size > 0) {
				PRINT(("writing to underlying stream (offset: %llu, size: %ld)\n", extent.offset, extent.size));
				ssize_t bytes = fStream.WriteAt(extent.offset, data, extent.size);
				if (bytes >= 0) {
					size -= bytes;
					fPosition += bytes;
					bytesTotal += bytes;
				} else {
					error = status_t(bytes);
				}
			} else {
				// end of simulated stream
				break;
			}
		}		
	}
	RETURN(!error ? bytesTotal : ssize_t(error));
}

/*! \brief Writes \a size bytes worth of data from \a data at position
	\a pos in the stream without incrementing the stream's position marker.
*/
ssize_t
SimulatedStream::WriteAt(off_t pos, BDataIO &data, size_t size)
{
	status_t error = B_OK;
	ssize_t bytesTotal = 0;
	while (error == B_OK && size > 0) {
		data_extent extent;
		error = _GetExtent(pos, size, extent);
		if (!error) {
			if (extent.size > 0) {
				ssize_t bytes = fStream.WriteAt(extent.offset, data, extent.size);
				if (bytes >= 0) {
					size -= bytes;
					pos += bytes;
					bytesTotal += bytes;
				} else {
					error = status_t(bytes);
				}
			} else {
				// end of simulated stream
				break;
			}
		}		
	}
	return !error ? bytesTotal : ssize_t(error);
}

/*! \brief Writes \a size bytes worth of zeros at the current position 
	in the stream, incrementing the stream's position marker as it goes.
*/
ssize_t
SimulatedStream::Zero(size_t size)
{
	status_t error = B_OK;
	ssize_t bytesTotal = 0;
	while (error == B_OK && size > 0) {
		data_extent extent;
		error = _GetExtent(fPosition, size, extent);
		if (!error) {
			if (extent.size > 0) {
				ssize_t bytes = fStream.ZeroAt(extent.offset, extent.size);
				if (bytes >= 0) {
					size -= bytes;
					fPosition += bytes;
					bytesTotal += bytes;
				} else {
					error = status_t(bytes);
				}
			} else {
				// end of simulated stream
				break;
			}
		}		
	}
	return !error ? bytesTotal : ssize_t(error);
}

/*! \brief Writes \a size bytes worth of zeros at position \a pos
	in the stream without incrementing the stream's position marker.
*/
ssize_t
SimulatedStream::ZeroAt(off_t pos, size_t size)
{
	status_t error = B_OK;
	ssize_t bytesTotal = 0;
	while (error == B_OK && size > 0) {
		data_extent extent;
		error = _GetExtent(pos, size, extent);
		if (!error) {
			if (extent.size > 0) {
				ssize_t bytes = fStream.ZeroAt(extent.offset, extent.size);
				if (bytes >= 0) {
					size -= bytes;
					pos += bytes;
					bytesTotal += bytes;
				} else {
					error = status_t(bytes);
				}
			} else {
				// end of simulated stream
				break;
			}
		}		
	}
	return !error ? bytesTotal : ssize_t(error);
}	

off_t
SimulatedStream::Seek(off_t pos, uint32 seek_mode)
{
	off_t size = _Size();
	switch (seek_mode) {
		case SEEK_SET:
			fPosition = pos;
			break;
		case SEEK_CUR:
			fPosition += pos;
			break;
		case SEEK_END:
			fPosition = size + pos;
			break;
		default:
			break;
	}
	// Range check
	if (fPosition < 0)
		fPosition = 0;
	else if (fPosition > size && SetSize(fPosition) != B_OK) 				
		fPosition = size;
	return fPosition;
}
