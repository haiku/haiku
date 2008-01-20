/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#include "DeviceReader.h"

#include <malloc.h>
#include <string.h>

#include <File.h>


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
			uint8 buffer[8];
			ret = fDeviceFile->Read(buffer, 8);
			if (ret == 8) {
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
status_t
DeviceReader::ReadData(uint8* data, size_t size) const
{
	status_t ret = B_NO_INIT;
	if (fDeviceFile) {
		ret = fDeviceFile->InitCheck();
		if (ret >= B_OK) {
			uint8* buffer = new uint8[fMaxPackedSize + 8];
			ret = fDeviceFile->Read(buffer, fMaxPackedSize + 8);
			if (ret == (ssize_t)(fMaxPackedSize + 8)) {
				// make sure we don't copy too many bytes
				size_t length = min_c(size, fMaxPackedSize);
				memcpy(data, buffer + 8, length);
				// zero out any remaining bytes
				if (size > fMaxPackedSize)
					memset(data + length, 0, size - fMaxPackedSize);
				// operation could be considered successful
				ret = length;
			} else if (ret == 8 || ret == B_TIMED_OUT) {
				// it's ok if the operation timed out
				memset(data, 0, size);
				ret = B_OK;
			}
			delete[] buffer;
		}
	}
	return ret;
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
