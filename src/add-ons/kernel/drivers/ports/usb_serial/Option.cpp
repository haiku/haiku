/*
 * Copyright 2011-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "Option.h"


OptionDevice::OptionDevice(usb_device device, uint16 vendorID,
	uint16 productID, const char *description)
	:
	ACMDevice(device, vendorID, productID, description)
{
	TRACE_FUNCALLS("> OptionDevice found: %s\n", description);
}


status_t
OptionDevice::AddDevice(const usb_configuration_info *config)
{
	TRACE_FUNCALLS("> OptionDevice::AddDevice(%08x, %08x)\n", this, config);

	status_t status = B_OK;
	return status;
}


status_t
OptionDevice::ResetDevice()
{
	TRACE_FUNCALLS("> OptionDevice::ResetDevice(%08x)\n", this);
	return B_OK;
}
