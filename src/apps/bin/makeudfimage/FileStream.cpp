//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//----------------------------------------------------------------------

/*! \file OutputFile.cpp

	BFile subclass with some handy new output methods. (implementation)
*/

#include "OutputFile.h"

#include <stdlib.h>
#include <string.h>

OutputFile::OutputFile(const char *path, uint32 open_mode)
	: BFile(path, open_mode)
{
}

ssize_t
OutputFile::Write(const void *buffer, size_t size)
{
	return BFile::Write(buffer, size);
} 

ssize_t
OutputFile::WriteAt(off_t pos, const void *buffer, size_t size)
{
	return BFile::WriteAt(pos, buffer, size);
}

/*! \brief Writes \a size bytes worth of data from \a data at the current
	position in the file, incrementing the file's position marker as it goes.
	
	\param bufferSize The size of the memory buffer to be used for writing.
*/
ssize_t
OutputFile::Write(BDataIO &data, size_t size, size_t bufferSize)
{
	status_t error = bufferSize > 0 ? B_OK : B_BAD_VALUE;
	size_t bytes = 0;
	if (!error) {
		void *buffer = malloc(bufferSize);
		error = buffer ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Fudge the buffer size from here out if the requested
			// number of bytes to write is smaller than the buffer
			bufferSize = (size < bufferSize ? size : bufferSize);
			// Zero
			memset(buffer, 0, bufferSize);
			// Write
			while (bytes < size) {
				ssize_t bytesRead = data.Read(buffer, bufferSize);
				if (bytesRead >= 0) {
					ssize_t bytesWritten = BFile::Write(buffer, bytesRead);
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
	
	\param bufferSize The size of the memory buffer to be used for writing.
*/
ssize_t
OutputFile::WriteAt(off_t pos, BDataIO &data, size_t size, size_t bufferSize)
{
	status_t error = bufferSize > 0 ? B_OK : B_BAD_VALUE;
	size_t bytes = 0;
	if (!error) {
		void *buffer = malloc(bufferSize);
		error = buffer ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Fudge the buffer size from here out if the requested
			// number of bytes to write is smaller than the buffer
			bufferSize = (size < bufferSize ? size : bufferSize);
			// Zero
			memset(buffer, 0, bufferSize);
			// Write
			while (bytes < size) {
				ssize_t bytesRead = data.Read(buffer, bufferSize);
				if (bytesRead >= 0) {
					ssize_t bytesWritten = BFile::WriteAt(pos, buffer, bytesRead);
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
	
	\param bufferSize The size of the memory buffer to be used for writing.
*/
ssize_t
OutputFile::Zero(size_t size, size_t bufferSize)
{
	status_t error = bufferSize > 0 ? B_OK : B_BAD_VALUE;
	size_t bytes = 0;
	if (!error) {
		void *buffer = malloc(bufferSize);
		error = buffer ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Fudge the buffer size from here out if the requested
			// number of bytes to write is smaller than the buffer
			bufferSize = (size < bufferSize ? size : bufferSize);
			// Zero
			memset(buffer, 0, bufferSize);
			// Write
			while (bytes < size) {
				ssize_t bytesWritten = BFile::Write(buffer, bufferSize);
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
	
	\param bufferSize The size of the memory buffer to be used for writing.
*/
ssize_t
OutputFile::ZeroAt(off_t pos, size_t size, size_t bufferSize)
{
	status_t error = bufferSize > 0 ? B_OK : B_BAD_VALUE;
	size_t bytes = 0;
	if (!error) {
		void *buffer = malloc(bufferSize);
		error = buffer ? B_OK : B_NO_MEMORY;
		if (!error) {
			// Fudge the buffer size from here out if the requested
			// number of bytes to write is smaller than the buffer
			bufferSize = (size < bufferSize ? size : bufferSize);
			// Zero
			memset(buffer, 0, bufferSize);
			// Write
			while (bytes < size) {
				ssize_t bytesWritten = BFile::WriteAt(pos, buffer, bufferSize);
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
