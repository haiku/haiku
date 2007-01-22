
#include <USBKit.h>
#include <usbraw.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
 
// USBDevice -------------------------------------------------------------------------

USBDevice::USBDevice(const char *path)
{
	fd = -1;
	serial_string = NULL;
	manufacturer_string = NULL;
	product_string = NULL;
	active = NULL;
	ishub = false;
	location = NULL;
	
	SetTo(path);
}

USBDevice::~USBDevice()
{
	Release();	
}

const char *
USBDevice::Location(void) const
{
	return (const char *) location;
}

bool 
USBDevice::IsHub(void) const
{
	return ishub;
}

// Release allocated resources (for destructor or reinit)
void USBDevice::Release(void)
{
	int i;
	USBConfiguration *cfg;
	
	ishub = false;
	
	if(fd != -1) {
		close(fd);
		fd = -1;
	}
	
	if(location){
		free(location);
		location = NULL;
	}
	
	if(serial_string) {
		delete[] serial_string;
		serial_string = NULL;
	}
	
	if(manufacturer_string) {
		delete[] manufacturer_string;
		manufacturer_string = NULL;
	}
	
	if(product_string) {
		delete[] product_string;
		product_string = NULL;
	}
	
	for(i=0;(cfg = (USBConfiguration *) configurations.ItemAt(i)); i++){
		delete cfg;
	}
	configurations.MakeEmpty();
}

void
USBDevice::PopulateInterface(USBInterface *ifc, usb_interface_descriptor *descr)
{
	uint i;
	memcpy(&(ifc->descriptor),descr,sizeof(usb_interface_descriptor));
	for(i=0;i<ifc->CountEndpoints();i++){
		usb_raw_command cmd;
		usb_endpoint_descriptor descr;
		cmd.endpoint.descr = &descr;
		cmd.endpoint.config_num = ifc->configuration->configuration_num;
		cmd.endpoint.interface_num = ifc->interface_num;
		cmd.endpoint.endpoint_num = i;
		ioctl(fd,get_endpoint_descriptor,&cmd,sizeof(cmd));
		if(!cmd.endpoint.status){
			USBEndpoint *ept = new USBEndpoint(i,ifc);
			memcpy(&(ept->descriptor),&descr,sizeof(usb_endpoint_descriptor));
			ept->endpoint_num = i;
			ifc->endpoints.AddItem(ept);
		}		
	}
}

void 
USBDevice::PopulateConfig(USBConfiguration *conf, usb_configuration_descriptor *descr)
{
	uint i;
	memcpy(&(conf->descriptor),descr,sizeof(usb_configuration_descriptor));
	for(i=0;i<conf->CountInterfaces();i++){
		usb_raw_command cmd;
		usb_interface_descriptor descr;
		cmd.interface.descr = &descr;
		cmd.interface.config_num = conf->configuration_num;
		cmd.interface.interface_num = i;
		ioctl(fd,get_interface_descriptor,&cmd,sizeof(cmd));
		if(!cmd.interface.status){
			USBInterface *ifc = new USBInterface(i,conf);
			ifc->interface_num = i;
			PopulateInterface(ifc,&descr);
			conf->interfaces.AddItem(ifc);
		}		
	}
}

void
USBDevice::PopulateDevice(void)
{		
	uint i;
	for(i=0;i<CountConfigurations();i++){
		usb_raw_command cmd;
		usb_configuration_descriptor descr;
		cmd.config.descr = &descr;
		cmd.config.config_num = i;
		ioctl(fd,get_configuration_descriptor,&cmd,sizeof(cmd));
		if(!cmd.config.status){
			USBConfiguration *conf = new USBConfiguration(i,this);
			PopulateConfig(conf,&descr);
			configurations.AddItem(conf);
		}
	}
}


status_t 
USBDevice::InitCheck(void)
{
	return (fd != -1) ? B_OK : B_ERROR;
}

status_t 
USBDevice::SetTo(const char *path)
{
	usb_raw_command cmd;
	
	Release();
	
	if(strncmp("/dev/bus/usb/",path,12)) {
		fprintf(stderr,"USBKit: Not a usb raw device '%s'\n",path);
		return B_ERROR;
	}
	
	if((fd = open(path,O_RDWR)) < 0) {
		fprintf(stderr,"USBKit: Cannot open '%s'\n",path);
		return B_ERROR;
	}
	
	if(ioctl(fd, check_version, &cmd, sizeof(cmd))){
		fprintf(stderr,"USBKit: Device won't verify version\n");
		close(fd);
		fd = -1;
		return B_ERROR;
	}
	
	if(cmd.generic.status != USB_RAW_DRIVER_VERSION){
		fprintf(stderr,"USBKit: Driver version 0x%04x != expected version 0x%04x\n",
				cmd.generic.status, USB_RAW_DRIVER_VERSION);
		close(fd);
		fd = -1;
		return B_ERROR;
	}
			
	cmd.device.descr = &descriptor;
	if(ioctl(fd,get_device_descriptor,&cmd,sizeof(cmd))){
		fprintf(stderr,"USBKit: Device won't return device descriptor\n");
		fd = -1;
		return B_ERROR;
	} else {
		PopulateDevice();
	}

	location = strdup(path+12);
	if(location[strlen(location)-1]=='b'){
		ishub = true;
		location[strlen(location)-4] = 0;
	}
	return B_OK;
}

const usb_device_descriptor *
USBDevice::Descriptor(void) const
{
	return &descriptor;
}

uint16 
USBDevice::USBVersion(void) const
{
	return descriptor.usb_version;
}

uint8 
USBDevice::Class(void) const
{
	return descriptor.device_class;
}

uint8 
USBDevice::Subclass(void) const
{
	return descriptor.device_subclass;
}

uint8 
USBDevice::Protocol(void) const
{
	return descriptor.device_protocol;
}

uint8 
USBDevice::MaxEndpoint0PacketSize(void) const
{
	return descriptor.max_packet_size_0;
}

uint16 
USBDevice::VendorID(void) const
{
	return descriptor.vendor_id;
}

uint16 
USBDevice::ProductID(void) const
{
	return descriptor.product_id;
}

uint16 
USBDevice::Version(void) const
{
	return descriptor.device_version;
}

const char *
USBDevice::ManufacturerString(void) const
{
	if(descriptor.manufacturer){
		if(manufacturer_string) delete[] manufacturer_string;
		manufacturer_string = DecodeStringDescriptor(descriptor.manufacturer);
		if(manufacturer_string){
			return (const char *) manufacturer_string;
		}
	}
	return (const char *) "";
}

const char *
USBDevice::ProductString(void) const
{
	if(descriptor.product){
		if(product_string) delete[] product_string;
		product_string = DecodeStringDescriptor(descriptor.product);
		if(product_string) {
			return (const char *) product_string;
		}
	}
	return (const char *) "";
}

const char *
USBDevice::SerialNumberString(void) const
{
	if(descriptor.serial_number){
		if(serial_string) delete[] serial_string;
		serial_string = DecodeStringDescriptor(descriptor.serial_number);
		if(serial_string){
			return (const char *) serial_string;
		}
	} 
	return (const char *) "";
}

char *
USBDevice::DecodeStringDescriptor(uint32 n) const
{
	char *s;
	size_t l;
	uint i;
	union {
		usb_string_descriptor str;
		uint16 raw[128];
	} x;
	
	l = GetStringDescriptor(n, &x.str, 255);
	if(l < 3) return NULL;
	if((l != x.str.length) || (x.str.length > 255)) return NULL;
	l = (l-2)/2;
	s = new char[l+1];
	for(i=0;i<l;i++) {
		s[i] = x.raw[i+1];
	}
	s[i] = 0;
	return s;
}

size_t
USBDevice::GetStringDescriptor(uint32 n, usb_string_descriptor *desc, size_t length) const
{
	usb_raw_command raw;
	raw.string.descr = desc;
	raw.string.string_num = n;
	raw.string.length = length;
	
	if(ioctl(fd,get_string_descriptor,&raw,sizeof(raw)) || (raw.string.status)){
		return 0;
	} else {
		return raw.string.length;
	}
	return 0;
}


uint32 
USBDevice::CountConfigurations(void) const
{
	if(fd == -1){
		return 0;
	} else {
		return descriptor.num_configurations;
	}
}

const USBConfiguration *
USBDevice::ActiveConfiguration(void) const
{
	return active;
}

const USBConfiguration *
USBDevice::ConfigurationAt(uint32 n) const
{
	return (USBConfiguration *) configurations.ItemAt(n);
}

status_t 
USBDevice::SetConfiguration(const USBConfiguration *conf)
{
	usb_raw_command cmd;
	cmd.config.config_num = conf->configuration_num;
	if(ioctl(fd, set_configuration, &cmd, sizeof(cmd))){
		return B_ERROR;
	} else {
		if(!cmd.generic.status) active = (USBConfiguration *) conf;
		return cmd.generic.status ? B_ERROR : B_OK;
	}
}

status_t 
USBDevice::ControlTransfer(uint8 request_type, uint8 request, uint16 value, 
						   uint16 index, uint16 length, void *data) const
{
	usb_raw_command cmd;
	cmd.control.request_type = request_type;
	cmd.control.request = request;
	cmd.control.value = value;
	cmd.control.index = index;
	cmd.control.length = length;
	cmd.control.data = data;
	
	if(ioctl(fd, do_control, &cmd, sizeof(cmd))){
		return B_ERROR;
	} else {
		return cmd.generic.status ? B_ERROR : B_OK;
	}
}

status_t 
USBDevice::InterruptTransfer(const USBEndpoint *e, void *data, size_t length) const
{
	if(!e || (e->Device() != this)) return B_ERROR;
	
	usb_raw_command cmd;
	cmd.interrupt.interface = e->interface->interface_num;
	cmd.interrupt.endpoint = e->endpoint_num;
	cmd.interrupt.data = data;
	cmd.interrupt.length = length;
	
	if(ioctl(fd, do_interrupt, &cmd, sizeof(cmd))){
		return B_ERROR;
	} else {
		return cmd.generic.status ? B_ERROR : B_OK;
	}
}

status_t 
USBDevice::BulkTransfer(const USBEndpoint *e, void *data, size_t length) const
{
	if(!e || (e->Device() != this)) return B_ERROR;
	
	usb_raw_command cmd;
	cmd.bulk.interface = e->interface->interface_num;
	cmd.bulk.endpoint = e->endpoint_num;
	cmd.bulk.data = data;
	cmd.bulk.length = length;
	
	if(ioctl(fd, do_bulk, &cmd, sizeof(cmd))){
		return B_ERROR;
	} else {
		return cmd.generic.status ? B_ERROR : B_OK;
	}
}

