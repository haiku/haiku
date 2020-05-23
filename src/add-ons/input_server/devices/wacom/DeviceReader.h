/*
 * Copyright 2005-2008 Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef USB_DEVICE_MONITOR_H
#define USB_DEVICE_MONITOR_H


// This class is designed to work with the generic USB input device driver
// the driver creates an entry in /dev/input/???
// clients can open and read from the device file, the driver will than
// block on waiting for an interrupt transfer (timeout is 500 ms) and write
// the received data to the supplied buffer. In front of the buffer, it will
// write vendorID, productID and maxPacketSize, so you can read from the file
// with a buffer of just enough size to contain these fields, in this case, no
// interrupt transfer will be triggered, and the client can then configure itself
// with a more appropriate setup.

#include <SupportDefs.h>

class BFile;
class BString;

class DeviceReader {
 public:
								DeviceReader();
	virtual						~DeviceReader();

								// initializes the object
								// by trying to read from the supplied device file
								// on success (B_OK), all member variables will be set
								// and the object is ready for operation
								// on failure, a hopefully meaningful error is returned
	virtual	status_t			SetTo(const char* path);

	virtual	status_t			InitCheck() const;
	
			const char*			DevicePath() const;
			BFile*				DeviceFile() const;

			// query the device for information
			uint16				VendorID() const;
			uint16				ProductID() const;

			size_t				MaxPacketSize() const;

			// trigger an interrupt transfer and write the data in the buffer
			// it should be save to call this function with
			// size != MaxPacketSize, remaining bytes will be zero'd out
			ssize_t				ReadData(uint8* data,
									const size_t size) const;

 protected:
			void				_Unset();

			char*				fDevicePath;
			BFile*				fDeviceFile;

			uint16				fVendorID;
			uint16				fProductID;
			size_t				fMaxPacketSize;
};
#endif // USB_DEVICE_MONITOR_H
