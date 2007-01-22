
#include <usb/USBKit.h>
#include <stdio.h>

static void DumpInterface(const BUSBInterface *ifc)
{
	int i;
	const BUSBEndpoint *ept;
	if(!ifc) return;
	printf("    Class .............. %d\n",ifc->Class());
	printf("    Subclass ........... %d\n",ifc->Subclass());
	printf("    Protocol ........... %d\n",ifc->Protocol());
	for(i=0;i<ifc->CountEndpoints();i++){
		if(ept = ifc->EndpointAt(i)){
			printf("      [Endpoint %d]\n",i);
			printf("      MaxPacketSize .... %d\n",ept->MaxPacketSize());
			printf("      Interval ......... %d\n",ept->Interval());
			if(ept->IsBulk()){
				printf("      Type ............. Bulk\n");
			}	
			if(ept->IsIsochronous()){
				printf("      Type ............. Isochronous\n");
			}	
			if(ept->IsInterrupt()){
				printf("      Type ............. Interrupt\n");
			}	
			if(ept->IsInput()){
				printf("      Direction ........ Input\n");
			} else {
				printf("      Direction ........ Output\n");
			}	
		}
	}
}

static void DumpConfiguration(const BUSBConfiguration *conf)
{
	int i;
	if(!conf) return;
	for(i=0;i<conf->CountInterfaces();i++){
		printf("    [Interface %d]\n",i);
		DumpInterface(conf->InterfaceAt(i));
	}
}

static void DumpInfo(BUSBDevice &dev)
{
	int i;
	
	printf("[Device]\n");
	printf("Class .................. %d\n",dev.Class());
	printf("Subclass ............... %d\n",dev.Subclass());
	printf("Protocol ............... %d\n",dev.Protocol());
	printf("Vendor ID .............. 0x%04x\n",dev.VendorID());
	printf("Product ID ............. 0x%04x\n",dev.ProductID());
	printf("Version ................ 0x%04x\n",dev.Version());
	printf("Manufacturer String .... \"%s\"\n",dev.ManufacturerString());
	printf("Product String ......... \"%s\"\n",dev.ProductString());
	printf("Serial Number .......... \"%s\"\n",dev.SerialNumberString());
	
	for(i=0;i<dev.CountConfigurations();i++){
		printf("  [Configuration %d]\n",i);
		DumpConfiguration(dev.ConfigurationAt(i));
	}
}

int main(int argc, char *argv[])
{
	if(argc == 2){
		BUSBDevice dev(argv[1]);
		if(dev.InitCheck()){
			printf("Cannot open USB device: %s\n",argv[1]);
			return 1;
		} else {
			DumpInfo(dev);
			return 0;
		}
	} else {
		printf("Usage: info <device>\n");
		return 1;
	}
}
