//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file PositionIOStream.cpp
*/

#include "PositionIOStream.h"

#include <stdlib.h>
#include <string.h>

PositionIOStream::PositionIOStream(BPositionIO &stream)
	: fStream(stream)
{
}

ssize_t
PositionIOStream::Read(void *buffer, size_t size)
{
	return fStream.Read(buffer, size);
}

ssize_t
PositionIOStream::ReadAt(off_t pos, void *buffer, size_t size)
{
	return fStream.ReadAt(pos, buffer, size);
}

ssize_t
PositionIOStream::Write(const void *buffer, size_t size)
{
	return fStream.Write(buffer, size);
} 

ssize_t
PositionIOStream::WriteAt(off_t pos, const void *buffer, size_t size)
{
	return fStream.WriteAt(pos, buffer, size);
}

/*! \brief Writes \a size bytes worth of data from \a data at the current
	position in the file, incrementing the file's position marker as it goes.
*/
ssize_t
PositionIOStream::Write(BDataIO &data, size_t size)
{
	status_t error = kBufferSize > 0 ? B_OK : B_BAD_VALUE;
	size_t bytes = 0;
	if (!error) {
		void *buffer = malloc(kBufferSize);
		error = buffer ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Fudge the buffer size from here out if the requested
			// number of bytes to write is smaller than the buffer
			size_t bufferSize = (size < kBufferSize ? size : kBufferSize);
			// Zero
			memset(buffer, 0, bufferSize);
			// Write
			while (bytes < size) {
				ssize_t bytesRead = data.Read(buffer, bufferSize);
				if (bytesRead >= 0) {
					ssize_t bytesWritten = fStream.Write(buffer, bytesRead);
					if (bytesWritten >= 0) {
						bytes += bytesWritten;
					} else {
						error = status_t(bytesWritten);
						break;
					}
				} else {
					error = status_t(bytesRead);
					break;
				}
			}
		}		
		free(buffer);
	}	
	return error ? ssize_t(error) : ssize_t(bytes);	
}

/*! \brief Writes \a size bytes worth of data from \a data at position
	\a pos in the file without incrementing the file's position marker.
*/
ssize_t
PositionIOStream::WriteAt(off_t pos, BDataIO &data, size_t size)
{
	status_t error = kBufferSize > 0 ? B_OK : B_BAD_VALUE;
	size_t bytes = 0;
	if (!error) {
		void *buffer = malloc(kBufferSize);
		error = buffer ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Fudge the buffer size from here out if the requested
			// number of bytes to write is smaller than the buffer
			size_t bufferSize = (size < kBufferSize ? size : kBufferSize);
			// Zero
			memset(buffer, 0, bufferSize);
			// Write
			while (bytes < size) {
				ssize_t bytesRead = data.Read(buffer, bufferSize);
				if (bytesRead >= 0) {
					ssize_t bytesWritten = fStream.WriteAt(pos, buffer, bytesRead);
					if (bytesWritten >= 0) {
						bytes += bytesWritten;
						pos += bytesWritten;
					} else {
						error = status_t(bytesWritten);
						break;
					}
				} else {
					error = status_t(bytesRead);
					break;
				}
			}
		}		
		free(buffer);
	}	
	return error ? ssize_t(error) : ssize_t(bytes);	
}

/*! \brief Writes \a size bytes worth of zeros at the current position 
	in the file, incrementing the file's position marker as it goes.
*/
ssize_t
PositionIOStream::Zero(size_t size)
{
	status_t error = kBufferSize > 0 ? B_OK : B_BAD_VALUE;
	size_t bytes = 0;
	if (!error) {
		void *buffer = malloc(kBufferSize);
		error = buffer ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Fudge the buffer size from here out if the requested
			// number of bytes to write is smaller than the buffer
			size_t bufferSize = (size < kBufferSize ? size : kBufferSize);
			// Zero
			memset(buffer, 0, bufferSize);
			// Write
			while (bytes < size) {
				ssize_t bytesWritten = fStream.Write(buffer, bufferSize);
				if (bytesWritten >= 0) {
					bytes += bytesWritten;
				} else {
					error = status_t(bytesWritten);
					break;
				}
			}
		}		
		free(buffer);
	}	
	return error ? ssize_t(error) : ssize_t(bytes);	
}

/*! \brief Writes \a size bytes worth of zeros at position \a pos
	in the file without incrementing the file's position marker.
*/
ssize_t
PositionIOStream::ZeroAt(off_t pos, size_t size)
{
	status_t error = kBufferSize > 0 ? B_OK : B_BAD_VALUE;
	size_t bytes = 0;
	if (!error) {
		void *buffer = malloc(kBufferSize);
		error = buffer ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Fudge the buffer size from here out if the requested
			// number of bytes to write is smaller than the buffer
			size_t bufferSize = (size < kBufferSize ? size : kBufferSize);
			// Zero
			memset(buffer, 0, bufferSize);
			// Write
			while (bytes < size) {
				ssize_t bytesWritten = fStream.WriteAt(pos, buffer, bufferSize);
				if (bytesWritten >= 0) {
					bytes += bytesWritten;
					pos += bytesWritten;
				} else {
					error = status_t(bytesWritten);
					break;
				}
			}
		}		
		free(buffer);
	}	
	return error ? ssize_t(error) : ssize_t(bytes);	
}	

off_t
PositionIOStream::Seek(off_t position, uint32 seek_mode)
{
	return fStream.Seek(position, seek_mode);
}

off_t
PositionIOStream::Position() const
{
	return fStream.Position();
}

status_t
PositionIOStream::SetSize(off_t size)
{
	return fStream.SetSize(size);
}
