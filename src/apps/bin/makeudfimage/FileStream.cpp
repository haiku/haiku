//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file FileStream.cpp
*/

#include "FileStream.h"

#include <stdlib.h>
#include <string.h>

FileStream::FileStream(const char *path, uint32 open_mode)
	: fFile(path, open_mode)
{
}

status_t
FileStream::InitCheck() const
{
	return fFile.InitCheck();
}

ssize_t
FileStream::Read(void *buffer, size_t size)
{
	return fFile.Read(buffer, size);
}

ssize_t
FileStream::ReadAt(off_t pos, void *buffer, size_t size)
{
	return fFile.ReadAt(pos, buffer, size);
}

ssize_t
FileStream::Write(const void *buffer, size_t size)
{
	return fFile.Write(buffer, size);
} 

ssize_t
FileStream::WriteAt(off_t pos, const void *buffer, size_t size)
{
	return fFile.WriteAt(pos, buffer, size);
}

/*! \brief Writes \a size bytes worth of data from \a data at the current
	position in the file, incrementing the file's position marker as it goes.
*/
ssize_t
FileStream::Write(BDataIO &data, size_t size)
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
					ssize_t bytesWritten = fFile.Write(buffer, bytesRead);
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
FileStream::WriteAt(off_t pos, BDataIO &data, size_t size)
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
					ssize_t bytesWritten = fFile.WriteAt(pos, buffer, bytesRead);
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
FileStream::Zero(size_t size)
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
				ssize_t bytesWritten = fFile.Write(buffer, bufferSize);
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
FileStream::ZeroAt(off_t pos, size_t size)
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
				ssize_t bytesWritten = fFile.WriteAt(pos, buffer, bufferSize);
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
FileStream::Seek(off_t position, uint32 seek_mode)
{
	return fFile.Seek(position, seek_mode);
}

off_t
FileStream::Position() const
{
	return fFile.Position();
}

status_t
FileStream::SetSize(off_t size)
{
	return fFile.SetSize(size);
}
