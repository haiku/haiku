
#include <USBKit.h>

// USBConfiguration ------------------------------------------------------------------

uint32 
USBConfiguration::CountInterfaces(void) const
{
	return descriptor.number_interfaces;
}

const USBInterface *
USBConfiguration::InterfaceAt(uint32 n) const
{
	return (USBInterface *) interfaces.ItemAt(n);
}

const usb_configuration_descriptor *
USBConfiguration::Descriptor(void) const
{
	return &descriptor;
}

const USBDevice *
USBConfiguration::Device(void) const
{
	return device;
}

USBConfiguration::USBConfiguration(uint n, USBDevice *dev)
{
	configuration_num = n;
	device = dev;
}

USBConfiguration::~USBConfiguration()
{
	int i;
	USBInterface *ifc;
		
	for(i=0;(ifc = (USBInterface *) interfaces.ItemAt(i)); i++){
		delete ifc;
	}
	interfaces.MakeEmpty();
}

