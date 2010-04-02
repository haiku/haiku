/*
 * Copyright 2009 Oliver Ruiz Dorantes, oliver.ruiz.dorantes_at_gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PORTLISTENER_H_
#define PORTLISTENER_H_

#include <OS.h>

template <
	typename TYPE,
	ssize_t MAX_MESSAGE_SIZE = 256,
	size_t MAX_MESSAGE_DEEP	= 16,
	uint32 PRIORITY	= B_URGENT_DISPLAY_PRIORITY>
class PortListener {
public:
	typedef status_t (*port_listener_func)(TYPE*, int32, size_t);

	PortListener(const char* name, port_listener_func handler)
	{
		fInformation.func = handler;
		fInformation.port = &fPort;

		fPortName = strdup(name);

		fThreadName = (char*)malloc(strlen(name) + strlen(" thread") + 1);
		fThreadName = strcpy(fThreadName, fPortName);
		fThreadName = strcat(fThreadName, " thread");

		InitCheck();
	}


	~PortListener()
	{
		status_t status;

		close_port(fPort);
		// Closing the port	should provoke the thread to finish
		wait_for_thread(fThread, &status);

		free(fThreadName);
		free(fPortName);
	}


	status_t Trigger(int32 code)
	{
			return write_port(fPort, code, NULL, 0);
	}


	status_t Trigger(int32 code, TYPE* buffer, size_t size)
	{
		if (buffer == NULL)
			return B_ERROR;

		return write_port(fPort, code, buffer, size);
	}


	status_t InitCheck()
	{
		// Create Port
		fPort = find_port(fPortName);
		if (fPort == B_NAME_NOT_FOUND) {
			fPort = create_port(MAX_MESSAGE_DEEP, fPortName);
		}

		if (fPort < B_OK)
			return fPort;

		#ifdef KERNEL_LAND
		// if this is the case you better stay with kernel
		set_port_owner(fPort, B_SYSTEM_TEAM);
		#endif

		// Create Thread
		fThread = find_thread(fThreadName);
		if (fThread < B_OK) {
#ifdef KERNEL_LAND
			fThread = spawn_kernel_thread((thread_func)&PortListener<TYPE,
				MAX_MESSAGE_SIZE, MAX_MESSAGE_DEEP, PRIORITY>::threadFunction,
				fThreadName, PRIORITY, &fInformation);
#else
			fThread = spawn_thread((thread_func)&PortListener<TYPE,
				MAX_MESSAGE_SIZE, MAX_MESSAGE_DEEP, PRIORITY>::threadFunction,
				fThreadName, PRIORITY, &fInformation);
#endif
		}

		if (fThread < B_OK)
			return fThread;

		return B_OK;
	}


	status_t Launch()
	{
		status_t check = InitCheck();

		if (check < B_OK)
			return check;

		return resume_thread(fThread);
	}


	status_t Stop()
	{
		status_t status;

		close_port(fPort);
		// Closing the port	should provoke the thread to finish
		wait_for_thread(fThread, &status);

		return status;
	}


private:

	struct PortListenerInfo {
		port_id* port;
		port_listener_func func;
	} fInformation;

	port_id fPort;
	thread_id fThread;
	char* fThreadName;
	char* fPortName;

	static int32 threadFunction(void* data)
	{
		ssize_t	ssizePort;
		ssize_t	ssizeRead;
		status_t status = B_OK;
		int32 code;

		port_id* port = ((struct PortListenerInfo*)data)->port;
		port_listener_func handler = ((struct PortListenerInfo*)data)->func;


		TYPE* buffer = (TYPE*)malloc(MAX_MESSAGE_SIZE);

		while ((ssizePort = port_buffer_size(*port)) != B_BAD_PORT_ID) {

			if (ssizePort <= 0) {
				snooze(500 * 1000);
				continue;
			}

			if (ssizePort >	MAX_MESSAGE_SIZE) {
				snooze(500 * 1000);
				continue;
			}

			ssizeRead = read_port(*port, &code, (void*)buffer, ssizePort);

			if (ssizeRead != ssizePort)
				continue;

			status = handler(buffer, code,	ssizePort);

			if (status != B_OK)
				break;

		}

		#ifdef DEBUG_PORTLISTENER
		#ifdef KERNEL_LAND
		dprintf("Error in PortListener handler=%s port=%s\n", strerror(status),
			strerror(ssizePort));
		#else
		printf("Error in PortListener handler=%s port=%s\n", strerror(status),
			strerror(ssizePort));
		#endif
		#endif

		free(buffer);

		if (ssizePort == B_BAD_PORT_ID) // the port disappeared
			return ssizePort;

		return status;
	}

}; // PortListener

#endif // PORTLISTENER_H_
