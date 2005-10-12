/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#include <cstdio>
#include <String.h>
#include <driver_settings.h>

#include <PPPInterface.h>
#include <PPPManager.h>


static const char sVersion[] = "0.12 pre-alpha";
static const char sPPPInterfaceModuleName[] = PPP_INTERFACE_MODULE_NAME;


static
status_t
print_help()
{
	fprintf(stderr, "Haiku Network Team: pppconfig: sVersion %s\n", sVersion);
	fprintf(stderr, "With pppconfig you can create and manage PPP connections.\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "pppconfig show | -a\n");
	fprintf(stderr, "pppconfig init <name>\n");
	fprintf(stderr, "pppconfig create <name>\n");
	fprintf(stderr, "pppconfig connect <name|interface|id>\n");
	fprintf(stderr, "pppconfig disconnect <name|interface|id>\n");
	fprintf(stderr, "pppconfig delete <name|interface|id>\n");
	fprintf(stderr, "pppconfig details <name|interface|id>\n");
	fprintf(stderr, "\t<name> must be an interface description file\n");
	
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
	ppp_interface_id *interfaces = manager.Interfaces(&count, filter);
	
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
			
			// type and unit (if it has one)
			if(info.info.if_unit >= 0) {
				printf("Type: Visible\n");
				printf("\tInterface: ppp%ld\n", info.info.if_unit);
			} else
				printf("Type: Hidden\n");
			
			printf("\tName: %s\n", info.info.name);
			
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
			printf("\tStatus: ");
			switch(info.info.phase) {
				case PPP_ESTABLISHED_PHASE:
					printf("Connected\n");
				break;
				
				case PPP_DOWN_PHASE:
					printf("Disconnected\n");
				break;
				
				case PPP_TERMINATION_PHASE:
					printf("Disconnecting\n");
				break;
				
				default:
					printf("Connecting\n");
			}
		}
	}
	
	delete interfaces;
	
	return 0;
}


static
status_t
create(const char *name, bool bringUp = true)
{
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.CreateInterfaceWithName(name));
	if(interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not create interface: %s!\n", name);
		return -1;
	}
	
	printf("Created interface with ID: %ld\n", interface.ID());
	
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	
	if(info.info.if_unit >= 0)
		printf("Interface: ppp%ld\n", info.info.if_unit);
	else
		printf("This interface is hidden! You can delete it by typing:\n"
			"pppconfig delete %ld\n", interface.ID());
	
	if(bringUp) {
		interface.Up();
		printf("Connecting in background...\n");
	}
	
	return 0;
}


static
status_t
connect(const char *name)
{
	if(!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
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
	if(!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
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
	if(!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	if(!manager.DeleteInterface(manager.InterfaceWithName(name))) {
		fprintf(stderr, "Error: Could not delete interface!\n");
		return -1;
	}
	
	return 0;
}


static
status_t
show_details(const char *name)
{
	if(!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if(manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if(interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	
	// type and name (if it has one)
	if(info.info.if_unit >= 0) {
		printf("Type: Visible\n");
		printf("Interface: ppp%ld\n", info.info.if_unit);
	} else
		printf("Type: Hidden\n");
	
	printf("Name: %s\n", info.info.name);
	
	// ID
	printf("ID: %ld\n", interface.ID());
	
	// ConnectOnDemand
	printf("ConnectOnDemand: %s\n", info.info.doesConnectOnDemand ?
		"Enabled" : "Disabled");
	
	// AutoReconnect
	printf("AutoReconnect: %s\n", info.info.doesAutoReconnect ?
		"Enabled" : "Disabled");
	
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
	printf("\tStatus: ");
	switch(info.info.phase) {
		case PPP_ESTABLISHED_PHASE:
			printf("Connected\n");
		break;
		
		case PPP_DOWN_PHASE:
			printf("Disconnected\n");
		break;
		
		case PPP_TERMINATION_PHASE:
			printf("Disconnecting\n");
		break;
		
		default:
			printf("Connecting\n");
	}
	
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
