/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Copyright 2006-2017, Haiku, Inc. All rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Alexander von Gluck IV <kallisti5@unixzen.com>
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
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	int32 count = 0;
	ppp_interface_id *interfaces = manager.Interfaces(&count, filter);
	
	if (!interfaces || count <= 0) {
		fprintf(stderr, "Error: Could not get interfaces information!\n");
		return -1;
	}

	fprintf(stderr, "Get %" B_PRId32 " ppp interfaces, first is %" B_PRIu32 "!\n",
		count, interfaces[0]);

	ppp_interface_info_t info;
	PPPInterface interface;
	
	printf("Listing PPP interfaces:\n");
	
	// print out information for each interface
	for (int32 index = 0; index < count; index++) {
		interface.SetTo(interfaces[index]);
		if (interface.InitCheck() == B_OK) {
			interface.GetInterfaceInfo(&info);
			printf("\n");
			
			// type and unit (if it has one)
			if (info.info.if_unit >= 0) {
				printf("Type: Visible\n");
				printf("\tInterface: ppp%" B_PRId32 "\n", info.info.if_unit);
			} else
				printf("Type: Hidden\n");
			
			printf("\tName: %s\n", info.info.name);
			
			// ID
			printf("\tID: %" B_PRIu32 "\n", interface.ID());
			
			// mode
			printf("\tMode: ");
			if (info.info.mode == PPP_CLIENT_MODE)
				printf("Client\n");
			else if (info.info.mode == PPP_SERVER_MODE)
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
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.CreateInterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not create interface: %s!\n", name);
		return -1;
	}
	
	printf("Created interface with ID: %" B_PRIu32 "\n", interface.ID());
	
	ppp_interface_info_t info;
	interface.GetInterfaceInfo(&info);
	
	if (info.info.if_unit >= 0)
		printf("Interface: ppp%" B_PRId32 "\n", info.info.if_unit);
	else
		printf("This interface is hidden! You can delete it by typing:\n"
			"pppconfig delete %" B_PRIu32 "\n", interface.ID());
	
	if (bringUp) {
		interface.Up();
		printf("Connecting in background...\n");
	}
	
	return 0;
}


static
status_t
connect(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	if (!interface.Up()) {
		fprintf(stderr, "Error: Could not connect!\n");
		return -1;
	}
	
	printf("Connecting in background...\n");
	
	return 0;
}


static
status_t
setuser(const char *name, const char* user)
{
	if (!name || strlen(name) == 0)
		return -1;
	
	if (!user || strlen(user) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	if (!interface.SetUsername(user)) {
		fprintf(stderr, "Error: Could not SetUsername %s!\n", user);
		return -1;
	}
	
	return 0;
}


static
status_t
setpass(const char *name, const char* pass)
{
	if (!name || strlen(name) == 0)
		return -1;
	
	if (!pass || strlen(pass) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	if (!interface.SetPassword(pass)) {
		fprintf(stderr, "Error: Could not SetUsername %s!\n", pass);
		return -1;
	}
	
	return 0;
}


static
status_t
setaskbeforeconnect(const char *name, const char* connect)
{
	if (!name || strlen(name) == 0)
		return -1;
	
	bool askBeforeConnecting = false;
	if (connect || !strcmp(connect, "true") || !strcmp(connect, "ask") ||
		!strcmp(connect, "yes") || !strcmp(connect, "y"))
		askBeforeConnecting = true;


	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	if (!interface.SetAskBeforeConnecting(askBeforeConnecting)) {
		fprintf(stderr, "Error: Could not connect %s!\n", connect);
		return -1;
	}
	
	return 0;
}


static
status_t
getstatistics(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_statistics pppStatistics;
	if (interface.GetStatistics(&pppStatistics) != B_OK) {
		fprintf(stderr, "Error: Could not getstatistics: %s!\n", name);
		return -1;
	}

	return 0;
}


static
status_t
hassettings(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	driver_settings settings;
	if (interface.HasSettings(&settings) != B_OK) {
		fprintf(stderr, "Error: Could not getstatistics: %s!\n", name);
		return -1;
	}

	return 0;
}


static
status_t
enablereports(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_report_type type = PPP_ALL_REPORTS;
	thread_id thread = 0;
        int32 flags = 0;

	if (interface.EnableReports(type, thread, flags) != true) {
		fprintf(stderr, "Error: Could not EnableReports: %s!\n", name);
		return -1;
	}

	return 0;
}


static
status_t
disablereports(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_report_type type = PPP_ALL_REPORTS;
	thread_id thread = 0;

	if (interface.DisableReports(type, thread) != true) {
		fprintf(stderr, "Error: Could not EnableReports: %s!\n", name);
		return -1;
	}

	return 0;
}


static
status_t
controlchild(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_simple_handler_info_t info;

	if (interface.ControlChild(&info, 0, PPPC_GET_SIMPLE_HANDLER_INFO) != true) {
		fprintf(stderr, "Error: Could not PPPC_GET_SIMPLE_HANDLER_INFO: %s!\n", name);
		return -1;
	}

	printf("LCPExtensionHandler: %s\n", info.info.name);
	printf("isEnabled: %d\n", info.info.isEnabled);

	if (interface.ControlChild(&info, 0, PPPC_ENABLE) != true) {
		fprintf(stderr, "Error: Could not PPPC_ENABLE: %s!\n", name);
		return -1;
	}
	return 0;
}


static
status_t
controllcpextension(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_simple_handler_info_t info;

	if (interface.ControlLCPExtension(&info, 1, PPPC_GET_SIMPLE_HANDLER_INFO) != true) {
		fprintf(stderr, "Error: Could not PPPC_GET_SIMPLE_HANDLER_INFO: %s!\n", name);
		return -1;
	}

	printf("LCPExtensionHandler: %s\n", info.info.name);
	printf("isEnabled: %d\n", info.info.isEnabled);

	if (interface.ControlLCPExtension(&info, 1, PPPC_ENABLE) != true) {
		fprintf(stderr, "Error: Could not PPPC_ENABLE: %s!\n", name);
		return -1;
	}
	return 0;
}


static
status_t
controloptionhandler(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_simple_handler_info_t info;

	if (interface.ControlOptionHandler(&info, 0, PPPC_GET_SIMPLE_HANDLER_INFO) != true) {
		fprintf(stderr, "Error: Could not PPPC_GET_SIMPLE_HANDLER_INFO: %s!\n", name);
		return -1;
	}

	printf("protocol: %s\n", info.info.name);
	printf("isEnabled: %d\n", info.info.isEnabled);

	if (interface.ControlOptionHandler(&info, 0, PPPC_ENABLE) != true) {
		fprintf(stderr, "Error: Could not PPPC_ENABLE: %s!\n", name);
		return -1;
	}
	return 0;
}


static
status_t
controlprotocol(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_protocol_info_t info;

	if (interface.ControlProtocol(&info, 0, PPPC_GET_PROTOCOL_INFO) != true) {
		fprintf(stderr, "Error: Could not PPPC_GET_PROTOCOL_INFO: %s!\n", name);
		return -1;
	}

	printf("protocol: %s\n", info.info.name);
	printf("type: %s\n", info.info.type);
	printf("activationPhase: %d\n", info.info.activationPhase);
	printf("addressFamily: %" B_PRId32 "\n", info.info.addressFamily);
	printf("flags: %" B_PRId32 "\n", info.info.flags);
	printf("side: %d\n", info.info.side);
	printf("level: %d\n", info.info.level);
	printf("connectionPhase: %d\n", info.info.connectionPhase);
	printf("protocolNumber: %d\n", info.info.protocolNumber);
	printf("isEnabled: %d\n", info.info.isEnabled);
	printf("isUpRequested: %d\n", info.info.isUpRequested);

	if (interface.ControlProtocol(&info, 0, PPPC_ENABLE) != true) {
		fprintf(stderr, "Error: Could not PPPC_ENABLE: %s!\n", name);
		return -1;
	}
	return 0;
}


static
status_t
controldevice(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;

	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	ppp_device_info_t info;

	if (interface.ControlDevice(&info) != true) {
		fprintf(stderr, "Error: Could not ControlDevice: %s!\n", name);
		return -1;
	}

	printf("name: %s\n", info.info.name);
	printf("MTU: %" B_PRIu32 "\n", info.info.MTU);
	printf("inputTransferRate: %" B_PRIu32 "\n", info.info.inputTransferRate);
	printf("outputTransferRate: %" B_PRIu32 "\n", info.info.outputTransferRate);
	printf("outputBytesCount: %" B_PRIu32 "\n", info.info.outputBytesCount);
	printf("isUp: %d\n", info.info.isUp);

	return 0;
}


static
status_t
disconnect(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	PPPInterface interface(manager.InterfaceWithName(name));
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
	
	if (!interface.Down()) {
		fprintf(stderr, "Error: Could not disconnect!\n");
		return -1;
	}
	
	return 0;
}


static
status_t
delete_interface(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	if (!manager.DeleteInterface(name)) {
		fprintf(stderr, "Error: Could not delete interface!\n");
		return -1;
	}
	
	return 0;
}


static
status_t
show_details(const char *name)
{
	if (!name || strlen(name) == 0)
		return -1;
	
	PPPManager manager;
	if (manager.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not load interface manager!\n");
		return -1;
	}
	
	ppp_interface_id ID = manager.InterfaceWithName(name);
	if (ID <= 0) {
		fprintf(stderr, "Error: Could not find interface: %s!\n", name);
		return -1;
	}
		
	PPPInterface interface(ID);
	if (interface.InitCheck() != B_OK) {
		fprintf(stderr, "Error: Could not find interface ID: %" B_PRIu32 "!\n", ID);
		return -1;
	}
	
	ppp_interface_info_t info;
	// printf("ppp_interface_info_t addr:%p\n", &info);
	interface.GetInterfaceInfo(&info);
	
	// type and name (if it has one)
	if (info.info.if_unit >= 0) {
		printf("Type: Visible\n");
		printf("Interface: ppp%" B_PRId32 "\n", info.info.if_unit);
	} else
		printf("Type: Hidden\n");
	
	printf("Name: %s\n", info.info.name);
	
	// ID
	printf("ID: %" B_PRIu32 "\n", interface.ID());
	
	// ConnectOnDemand
	printf("ConnectOnDemand: %s\n", info.info.doesConnectOnDemand ?
		"Enabled" : "Disabled");
	
	// AutoReconnect
	printf("AutoReconnect: %s\n", info.info.doesAutoReconnect ?
		"Enabled" : "Disabled");
	
	// MRU and interfaceMTU
	printf("MRU: %" B_PRIu32 "\n", info.info.MRU);
	printf("Interface MTU: %" B_PRIu32 "\n", info.info.interfaceMTU);
	
	// mode
	printf("Mode: ");
	if (info.info.mode == PPP_CLIENT_MODE)
		printf("Client\n");
	else if (info.info.mode == PPP_SERVER_MODE)
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
	if (argc == 2) {
		if (!strcmp(argv[1], "show") || !strcmp(argv[1], "-a"))
			return show(PPP_ALL_INTERFACES);
		else
			return print_help();
	}if (argc == 3) {
		if (!strcmp(argv[1], "init"))
			return create(argv[2], false);
		else if (!strcmp(argv[1], "create"))
			return create(argv[2], true);
		else if (!strcmp(argv[1], "connect"))
			return connect(argv[2]);
		else if (!strcmp(argv[1], "disconnect"))
			return disconnect(argv[2]);
		else if (!strcmp(argv[1], "delete"))
			return delete_interface(argv[2]);
		else if (!strcmp(argv[1], "details"))
			return show_details(argv[2]);
		else if (!strcmp(argv[1], "getstatistics"))
			return getstatistics(argv[2]);
		else if (!strcmp(argv[1], "hassettings"))
			return hassettings(argv[2]);
		else if (!strcmp(argv[1], "enablereports"))
			return enablereports(argv[2]);
		else if (!strcmp(argv[1], "disablereports"))
			return disablereports(argv[2]);
		else if (!strcmp(argv[1], "controldevice"))
			return controldevice(argv[2]);
		else if (!strcmp(argv[1], "controlprotocol"))
			return controlprotocol(argv[2]);
		else if (!strcmp(argv[1], "controloptionhandler"))
			return controloptionhandler(argv[2]);
		else if (!strcmp(argv[1], "controllcpextension"))
			return controllcpextension(argv[2]);
		else if (!strcmp(argv[1], "controlchild"))
			return controlchild(argv[2]);
		else
			return print_help();
	} if (argc == 4) {
		if (!strcmp(argv[1], "setuser"))
			return setuser(argv[2], argv[3]);
		else if (!strcmp(argv[1], "setpass"))
			return setpass(argv[2], argv[3]);
		else if (!strcmp(argv[1], "setaskbeforeconnect"))
			return setaskbeforeconnect(argv[2], argv[3]);
		else
			return print_help();
	} else
		return print_help();
}
