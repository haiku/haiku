//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <String.h>
#include <driver_settings.h>

#include <PPPInterface.h>
#include <PPPManager.h>


static const char version[] = "0.1 pre-alpha";
static const char ppp_interface_module_name[] = PPP_INTERFACE_MODULE_NAME;


// R5 only: strlcat is needed by driver_settings API
/** Concatenates the source string to the destination, writes
 *	as much as "maxLength" bytes to the dest string.
 *	Always null terminates the string as long as maxLength is
 *	larger than the dest string.
 *	Returns the length of the string that it tried to create
 *	to be able to easily detect string truncation.
 */
static
size_t
strlcat(char *dest, const char *source, size_t maxLength)
{
	size_t destLength = strnlen(dest, maxLength);

	// This returns the wrong size, but it's all we can do
	if (maxLength == destLength)
		return destLength + strlen(source);

	dest += destLength;
	maxLength -= destLength;

	size_t i = 0;
	for (; i < maxLength - 1 && source[i]; i++) {
		dest[i] = source[i];
	}

	dest[i] = '\0';

	return destLength + i + strlen(source + i);
}


static
status_t
print_help()
{
	fprintf(stderr, "OpenBeOS Network Team: pppconfig: version %s\n", version);
	fprintf(stderr, "With pppconfig you can create and manage PPP connections.\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "pppconfig show | -a\n");
	fprintf(stderr, "pppconfig init pppidf\n");
	fprintf(stderr, "pppconfig create pppidf\n");
	fprintf(stderr, "pppconfig connect interface\n");
	fprintf(stderr, "pppconfig disconnect interface\n");
	fprintf(stderr, "pppconfig delete interface\n");
	fprintf(stderr, "pppconfig details interface\n");
	fprintf(stderr, "\tpppidf is the interface description file\n");
	fprintf(stderr, "\tinterface is either a name or an id\n");
	
	return -1;
}


static
status_t
show(ppp_interface_filter filter = PPP_REGISTERED_INTERFACES)
{
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	int32 count;
	interface_id *interfaces = manager.Interfaces(&count, filter);
	
	if(!interfaces) {
		fprintf(stderr, "Error: Could not get interfaces information!\n");
		return -1;
	}
	
	ppp_interface_info_t info;
	PPPInterface interface;
	
	printf("Listing PPP interfaces:\n");
	
	// print out information for each interface
	for(int32 index = 0; index < count; index++) {
		interface.SetTo(interfaces[index]);
		if(interface.InitCheck() == B_OK) {
			interface.GetInterfaceInfo(&info);
			printf("\n");
			
			// type and name (if it has one)
			if(info.info.if_unit >= 0) {
				printf("Type: Visible\n");
				printf("\tName: ppp%ld\n", info.info.if_unit);
			} else
				printf("Type: Hidden\n");
			
			// ID
			printf("\tID: %ld\n", interface.ID());
			
			// mode
			printf("\tMode: ");
			if(info.info.mode == PPP_CLIENT_MODE)
				printf("Client\n");
			else if(info.info.mode == PPP_SERVER_MODE)
				printf("Server\n");
			else
				printf("Unknown\n");
			
			// status
			printf("\tStatus: %s\n",
				info.info.phase == PPP_ESTABLISHED_PHASE ? "Connected" : "Disconnected");
		}
	}
	
	delete interfaces;
	
	return 0;
}


static
status_t
create(const char *pppidf, bool bringUp = true)
{
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	BString name = "pppidf/";
	name << pppidf;
	
	void *handle = load_driver_settings(name.String());
	if(!handle) {
		fprintf(stderr, "Error: Could not load PPP interface description file!\n");
		return -1;
	}
	
	const driver_settings *settings = get_driver_settings(handle);
	if(!settings) {
		fprintf(stderr, "Error: Could not decode PPP interface description file!\n");
		unload_driver_settings(handle);
		return -1;
	}
	
	PPPInterface interface(manager.CreateInterface(settings));
	if(interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not create interface!\n");
		unload_driver_settings(handle);
		return -1;
	}
	
	printf("Created interface with ID: %ld\n", interface.ID());
	
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	
	if(info.info.if_unit >= 0)
		printf("Name: ppp%ld\n", info.info.if_unit);
	else
		printf("This interface is hidden! You can delete it by typing:\n"
			"pppconfig delete %ld\n", interface.ID());
	
	if(bringUp && !info.info.doesDialOnDemand) {
		interface.Up();
		printf("Connecting in background...\n");
	}
	
	unload_driver_settings(handle);
	
	return 0;
}


static
status_t
connect(const char *name)
{
	// Name may either be an interface name (ppp0, etc.) or an interface ID.
	
	if(!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface;
	
	if(!strncmp(name, "ppp", 3) && strlen(name) > 3 && isdigit(name[3]))
		interface = manager.InterfaceWithUnit(atoi(name + 3));
	else if(isdigit(name[0]))
		interface = atoi(name);
	else
		return print_help();
	
	if(interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	if(!interface.Up()) {
		fprintf(stderr, "Error: Could not connect!\n");
		return -1;
	}
	
	printf("Connecting in background...\n");
	
	return 0;
}


static
status_t
disconnect(const char *name)
{
	// Name may either be an interface name (ppp0, etc.) or an interface ID.
	
	if(!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface;
	
	if(!strncmp(name, "ppp", 3) && strlen(name) > 3 && isdigit(name[3]))
		interface = manager.InterfaceWithUnit(atoi(name + 3));
	else if(isdigit(name[0]))
		interface = atoi(name);
	else
		return print_help();
	
	if(interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	if(!interface.Down()) {
		fprintf(stderr, "Error: Could not disconnect!\n");
		return -1;
	}
	
	return 0;
}


static
status_t
delete_interface(const char *name)
{
	// Name may either be an interface name (ppp0, etc.) or an interface ID.
	
	if(!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	interface_id interface;
	
	if(!strncmp(name, "ppp", 3) && strlen(name) > 3 && isdigit(name[3]))
		interface = manager.InterfaceWithUnit(atoi(name + 3));
	else if(isdigit(name[0]))
		interface = atoi(name);
	else
		return print_help();
	
	if(!manager.DeleteInterface(interface)) {
		fprintf(stderr, "Error: Could not delete interface!\n");
		return -1;
	}
	
	return 0;
}


static
status_t
show_details(const char *name)
{
	// Name may either be an interface name (ppp0, etc.) or an interface ID.
	
	if(!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface;
	
	if(!strncmp(name, "ppp", 3) && strlen(name) > 3 && isdigit(name[3]))
		interface = manager.InterfaceWithUnit(atoi(name + 3));
	else if(isdigit(name[0]))
		interface = atoi(name);
	else
		return print_help();
	
	if(interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	
	// type and name (if it has one)
	if(info.info.if_unit >= 0) {
		printf("Type: Visible\n");
		printf("Name: ppp%ld\n", info.info.if_unit);
	} else
		printf("Type: Hidden\n");
	
	// ID
	printf("ID: %ld\n", interface.ID());
	
	// DialOnDemand
	printf("DialOnDemand: %s\n", info.info.doesDialOnDemand ? "Enabled" : "Disabled");
	
	// AutoRedial
	printf("AutoRedial: %s\n", info.info.doesAutoRedial ? "Enabled" : "Disabled");
	
	// MRU and interfaceMTU
	printf("MRU: %ld\n", info.info.MRU);
	printf("Interface MTU: %ld\n", info.info.interfaceMTU);
	
	// mode
	printf("Mode: ");
	if(info.info.mode == PPP_CLIENT_MODE)
		printf("Client\n");
	else if(info.info.mode == PPP_SERVER_MODE)
		printf("Server\n");
	else
		printf("Unknown\n");
	
	// status
	printf("Status: %s\n",
		info.info.phase == PPP_ESTABLISHED_PHASE ? "Connected" : "Disconnected");
	
	return 0;
}


int
main(int argc, char *argv[])
{
	if(argc == 2) {
		if(!strcmp(argv[1], "show") || !strcmp(argv[1], "-a"))
			return show(PPP_ALL_INTERFACES);
		else
			return print_help();
	}if(argc == 3) {
		if(!strcmp(argv[1], "init"))
			return create(argv[2], false);
		else if(!strcmp(argv[1], "create"))
			return create(argv[2], true);
		else if(!strcmp(argv[1], "connect"))
			return connect(argv[2]);
		else if(!strcmp(argv[1], "disconnect"))
			return disconnect(argv[2]);
		else if(!strcmp(argv[1], "delete"))
			return delete_interface(argv[2]);
		else if(!strcmp(argv[1], "details"))
			return show_details(argv[2]);
		else
			return print_help();
	} else
		return print_help();
}
