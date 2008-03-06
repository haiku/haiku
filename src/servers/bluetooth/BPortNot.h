/*
 * Copyright 2007 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 */

#ifndef _BT_PORT_NOTIFICATION
#define _BT_PORT_NOTIFICATION

#include <OS.h>

class BluetoothServer;

class BPortNot {

public:
	BPortNot(BluetoothServer* app, const char* name) ;
	void loop();

	port_id 		fPort;	
	port_id 		sdp_request_port;
	thread_id		sdp_server_thread;

	BluetoothServer* 		    ourapp;		
};

#endif
