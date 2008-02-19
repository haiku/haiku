/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#include "DeviceReader.h"

#include <malloc.h>
#include <string.h>

#include <File.h>

#include "MasterServerDevice.h"


static ssize_t kHeaderSize = 8;


// constructor
DeviceReader::DeviceReader()
	: fDevicePath(NULL),
	  fDeviceFile(NULL),
	  fVendorID(0),
	  fProductID(0),
	  fMaxPackedSize(0)
{
}

// destructor
DeviceReader::~DeviceReader()
{
	_Unset();
}

// SetTo
status_t
DeviceReader::SetTo(const char* path)
{
	status_t ret = B_BAD_VALUE;
	if (path) {
		_Unset();
		fDevicePath = strdup(path);
		fDeviceFile = new BFile(path, B_READ_ONLY);
		ret = fDeviceFile->InitCheck();
		if (ret >= B_OK) {
			// read 8 bytes from the file and initialize
			// the rest of the object variables
			uint8 buffer[kHeaderSize];
			ret = fDeviceFile->Read(buffer, kHeaderSize);
			if (ret == kHeaderSize) {
				ret = B_OK;
				uint16* ids = (uint16*)buffer;
				fVendorID = ids[0];
				fProductID = ids[1];
				uint32* ps = (uint32*)buffer;
				fMaxPackedSize = ps[1];
			} else {
				_Unset();
			}
		}
	}
	return ret;
}

// InitCheck
status_t
DeviceReader::InitCheck() const
{
	return fDeviceFile && fDevicePath ? fDeviceFile->InitCheck() : B_NO_INIT;
}

// DevicePath
const char*
DeviceReader::DevicePath() const
{
	return fDevicePath;
}

// DeviceFile
BFile*
DeviceReader::DeviceFile() const
{
	return fDeviceFile;
}

// VendorID
uint16
DeviceReader::VendorID() const
{
	return fVendorID;
}

// ProductID
uint16
DeviceReader::ProductID() const
{
	return fProductID;
}

// MaxPacketSize
size_t
DeviceReader::MaxPacketSize() const
{
	return fMaxPackedSize;
}

// ReadData
ssize_t
DeviceReader::ReadData(uint8* data, const size_t size) const
{
	if (!fDeviceFile || fMaxPackedSize <= 0 || fMaxPackedSize > 128)
		return B_NO_INIT;
	status_t ret = fDeviceFile->InitCheck();
	if (ret < B_OK)
		return (ssize_t)ret;

	ssize_t requested = fMaxPackedSize + kHeaderSize;
	uint8 buffer[requested];
	ssize_t read = fDeviceFile->Read(buffer, requested);
	if (read > kHeaderSize) {
		// make sure we don't copy too many bytes
		size_t bytesToCopy = min_c(size, read - (size_t)kHeaderSize);
PRINT(("requested: %ld, read: %ld, user wants: %lu, copy bytes: %ld\n",
	requested, read, size, bytesToCopy));
		memcpy(data, buffer + kHeaderSize, bytesToCopy);
		// zero out any remaining bytes
		if (size > bytesToCopy)
			memset(data + bytesToCopy, 0, size - bytesToCopy);
		// operation could be considered successful
//		read = bytesToCopy;
//		if (read != (ssize_t)size)
//			PRINT(("user wanted: %lu, returning: %ld\n", size, read));
		read = size;
			// pretend we could read as many bytes as requested
	} else if (read == kHeaderSize || (status_t)read == B_TIMED_OUT) {
		// it's ok if the operation timed out
		memset(data, 0, size);
		read = (size_t)B_OK;
	} else {
		PRINT(("requested: %ld, read: %ld, user wants: %lu\n",
			requested, read, size));
	}

	return read;
}

// _Unset
void
DeviceReader::_Unset()
{
	free(fDevicePath);
	fDevicePath = NULL;
	delete fDeviceFile;
	fDeviceFile = NULL;
	fVendorID = 0;
	fProductID = 0;
	fMaxPackedSize = 0;
}
