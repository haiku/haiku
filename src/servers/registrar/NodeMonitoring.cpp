//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

// A fake implementation of the node monitoring functions.
// They start a remote application that functions as an adapter for the
// functions in libbe.so.
// Note: We can't use the node monitoring provided by libbe directly, since
// it makes sure, that the target is local using BMessenger::Target(), which
// can't work.

#include <stdlib.h>
#include <string.h>

#include <AppMisc.h>
#include <image.h>
#include <Locker.h>
#include <Message.h>
#include <Messenger.h>
#include <NodeMonitor.h>
#include <OS.h>

#include "Debug.h"

// NodeMonitor
class NodeMonitor {
public:
	NodeMonitor();
	~NodeMonitor();

	status_t GetMessenger(BMessenger &monitor);
	status_t WatchingRequest(const node_ref *node, uint32 flags,
							 BMessenger target, bool start);

private:
	bool		fInitialized;
	status_t	fStatus;
	thread_id	fThread;
	team_id		fTeam;
	BMessenger	fMessenger;
	BLocker		fLock;
};

static NodeMonitor gNodeMonitor;

// constructor
NodeMonitor::NodeMonitor()
	: fInitialized(false),
	  fStatus(B_ERROR),
	  fThread(-1),
	  fTeam(-1),
	  fMessenger(),
	  fLock()
{
}

// destructor
NodeMonitor::~NodeMonitor()
{
	fLock.Lock();
	if (fInitialized) {
		if (fMessenger.IsValid())
			fMessenger.SendMessage(B_QUIT_REQUESTED);
		else if (fThread >= 0)
			kill_thread(fThread);
	}
	fLock.Unlock();
}

// GetMessenger
status_t
NodeMonitor::GetMessenger(BMessenger &monitor)
{
	fLock.Lock();
	if (!fInitialized) {
		fInitialized = true;
		// get NodeMonitor path
		char path[B_PATH_NAME_LENGTH];
		fStatus = BPrivate::get_app_path(path);
		if (fStatus == B_OK) {
			if (char *leaf = strstr(path, "obos_registrar")) {
				strcpy(leaf, "../../bin/NodeMonitor");
			} else
				fStatus = B_ERROR;
		}
		// start the NodeMonitor
		if (fStatus == B_OK) {
			const char *argv[] = { path, NULL };
			fThread = load_image(1, argv,
								 const_cast<const char**>(environ));
			if (fThread >= 0) {
				resume_thread(fThread);
				thread_info info;
				fStatus = get_thread_info(fThread, &info);
				if (fStatus == B_OK)
					fTeam = info.team;
			} else
				fStatus = fThread;
		}
		// find the app looper port
		port_id port = -1;
		if (fStatus == B_OK) {
			snooze(200000);
			port_info info;
			int32 cookie = 0;
			fStatus = B_ERROR;
			while (get_next_port_info(fTeam, &cookie, &info) == B_OK) {
				if (!strcmp(info.name, "AppLooperPort")) {
					fStatus = B_OK;
					port = info.port;
					break;
				}
			}
		}
		// get a messenger
		if (fStatus == B_OK) {
			struct {
				port_id	fPort;
				int32	fHandlerToken;
				team_id	fTeam;
				int32	extra0;
				int32	extra1;
				bool	fPreferredTarget;
				bool	extra2;
				bool	extra3;
				bool	extra4;
			} fakeMessenger;
			fakeMessenger.fPort = port;
			fakeMessenger.fHandlerToken = -1;
			fakeMessenger.fTeam = fTeam;
			fakeMessenger.fPreferredTarget = true;
			fMessenger = *(BMessenger*)&fakeMessenger;
			if (!fMessenger.IsValid())
				fStatus = B_ERROR;
		}
	}
	// set result
	if (fStatus == B_OK)
		monitor = fMessenger;
	fLock.Unlock();
	return fStatus;
}

// WatchingRequest
status_t
NodeMonitor::WatchingRequest(const node_ref *node, uint32 flags,
							 BMessenger target, bool start)
{
	BMessenger monitor;
	status_t error = GetMessenger(monitor);
	// prepare request message
	BMessage request;
	if (error == B_OK) {
		if (start) {
			request.what = 'wtch';
			request.AddInt32("device", node->device);
			request.AddInt64("node", node->node);
			request.AddInt32("flags", (int32)flags);
		} else {
			request.what = 'hctw';
		}
		request.AddMessenger("target", target);
	}
	// send request
	BMessage reply;
	if (error == B_OK)
		error = monitor.SendMessage(&request, &reply);
	// analyze reply
	if (error == B_OK) {
		status_t result;
		error = reply.FindInt32("result", &result);
		if (error == B_OK)
			error = result;
	}
	return error;
}

// watch_node
status_t
watch_node(const node_ref *node, uint32 flags, BMessenger target)
{
	status_t error = B_OK;
	if (flags == B_STOP_WATCHING)
		error = stop_watching(target);
	else {
		node_ref fakeNode;
		if (!node)
			node = &fakeNode;
		error = gNodeMonitor.WatchingRequest(node, flags, target, true);
	}
	return error;
}

// watch_node
status_t
watch_node(const node_ref *node, uint32 flags, const BHandler *handler,
		   const BLooper *looper)
{
	status_t error = (handler || looper ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BMessenger target(handler, looper);
		error = watch_node(node, flags, target);
	}
	return error;
}

// stop_watching
status_t
stop_watching(BMessenger target)
{
	return gNodeMonitor.WatchingRequest(NULL, 0, target, false);
}

// stop_watching
status_t
stop_watching(const BHandler *handler, const BLooper *looper)
{
	status_t error = (handler || looper ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BMessenger target(handler, looper);
		error = stop_watching(target);
	}
	return error;
}

