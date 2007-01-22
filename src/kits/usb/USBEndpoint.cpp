
// USBEndpoint ------------------------------------------------------------------------
 
#include <USBKit.h>

const USBInterface *
USBEndpoint::Interface(void) const
{
	return interface;
}

const USBConfiguration *
USBEndpoint::Configuration(void) const
{
	return interface->Configuration();
}

const USBDevice *
USBEndpoint::Device(void) const
{
	return interface->Configuration()->Device();
}

const usb_endpoint_descriptor *
USBEndpoint::Descriptor(void) const
{
	return &descriptor;
}

bool 
USBEndpoint::IsBulk(void) const
{
	return (descriptor.attributes & 0x03) == 0x02;
}

bool 
USBEndpoint::IsInterrupt(void) const
{
	return (descriptor.attributes & 0x03) == 0x03;
}

bool 
USBEndpoint::IsIsochronous(void) const
{
	return (descriptor.attributes & 0x03) == 0x01;
}

bool 
USBEndpoint::IsInput(void) const
{
	return (descriptor.endpoint_address & 0x01) == 0x01;
}

bool 
USBEndpoint::IsOutput(void) const
{
	return (descriptor.endpoint_address & 0x01) == 0x00;
}

uint16 
USBEndpoint::MaxPacketSize(void) const
{
	return descriptor.max_packet_size;
}

uint8 
USBEndpoint::Interval(void) const
{
	return descriptor.interval;
}

status_t 
USBEndpoint::InterruptTransfer(void *data, size_t length) const
{
	return Device()->InterruptTransfer(this,data,length);
}

status_t 
USBEndpoint::BulkTransfer(void *data, size_t length) const
{
	return Device()->BulkTransfer(this,data,length);
}

USBEndpoint::USBEndpoint(uint32 n, USBInterface *ifc)
{
	endpoint_num = n;
	interface = ifc;
}

USBEndpoint::~USBEndpoint()
{
	// nothing to do here really.
}

