
#include <USBKit.h>

// USBInterface ----------------------------------------------------------------------

uint32 
USBInterface::CountEndpoints(void) const
{
	return descriptor.num_endpoints;
}

const USBEndpoint *
USBInterface::EndpointAt(uint32 n) const
{
	return (USBEndpoint *) endpoints.ItemAt(n);
}

const usb_interface_descriptor *
USBInterface::Descriptor(void) const
{
	return &descriptor;
}

uint8 
USBInterface::Class(void) const
{
	return descriptor.interface_class;
}

uint8 
USBInterface::Subclass(void) const
{
	return descriptor.interface_subclass;
}

uint8 
USBInterface::Protocol(void) const
{
	return descriptor.interface_protocol;
}

const char *
USBInterface::InterfaceString(void) const
{
	return NULL;
}

const USBConfiguration *
USBInterface::Configuration(void) const
{
	return configuration;
}

const USBDevice *
USBInterface::Device(void) const
{
	return configuration->Device();
}

USBInterface::USBInterface(uint32 n, USBConfiguration *cfg)
{
	interface_num = n;
	configuration = cfg;
}

USBInterface::~USBInterface()
{
	int i;
	USBEndpoint *ept;
		
	for(i=0;(ept = (USBEndpoint *) endpoints.ItemAt(i)); i++){
		delete ept;
	}
	endpoints.MakeEmpty();
}
