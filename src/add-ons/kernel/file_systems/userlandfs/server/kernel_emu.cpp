// kernel_emu.cpp

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <fsproto.h>
#include <KernelExport.h>
#include <OS.h>

#include "RequestPort.h"
#include "Requests.h"
#include "RequestThread.h"
#include "UserlandFSServer.h"
#include "UserlandRequestHandler.h"

// Taken from the OBOS Storage Kit (storage_support.cpp)
/*! The length of the first component is returned as well as the index at
	which the next one starts. These values are only valid, if the function
	returns \c B_OK.
	\param path the path to be parsed
	\param length the variable the length of the first component is written
		   into
	\param nextComponent the variable the index of the next component is
		   written into. \c 0 is returned, if there is no next component.
	\return \c B_OK, if \a path is not \c NULL, \c B_BAD_VALUE otherwise
*/
static
status_t
parse_first_path_component(const char *path, int32& length,
						   int32& nextComponent)
{
	status_t error = (path ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		int32 i = 0;
		// find first '/' or end of name
		for (; path[i] != '/' && path[i] != '\0'; i++);
		// handle special case "/..." (absolute path)
		if (i == 0 && path[i] != '\0')
			i = 1;
		length = i;
		// find last '/' or end of name
		for (; path[i] == '/' && path[i] != '\0'; i++);
		if (path[i] == '\0')	// this covers "" as well
			nextComponent = 0;
		else
			nextComponent = i;
	}
	return error;
}

// new_path
int
new_path(const char *path, char **copy)
{
	// check errors and special cases
	if (!copy)
		return B_BAD_VALUE;
	if (!path) {
		*copy = NULL;
		return B_OK;
	}
	int32 len = strlen(path);
	if (len < 1)
		return B_ENTRY_NOT_FOUND;
	bool appendDot = (path[len - 1] == '/');
	if (appendDot)
		len++;
	if (len >= B_PATH_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	// check the path components
	const char *remainder = path;
	int32 length, nextComponent;
	do {
		status_t error
			= parse_first_path_component(remainder, length, nextComponent);
		if (error != B_OK)
			return error;
		if (length >= B_FILE_NAME_LENGTH)
			error = B_NAME_TOO_LONG;
		remainder += nextComponent;
	} while (nextComponent != 0);
	// clone the path
	char *copiedPath = (char*)malloc(len + 1);
	if (!copiedPath)
		return B_NO_MEMORY;
	strcpy(copiedPath, path);
	// append a dot, if desired
	if (appendDot) {
		copiedPath[len] = '.';
		copiedPath[len] = '\0';
	}
	*copy = copiedPath;
	return B_OK;
}

// free_path
void
free_path(char *p)
{
	free(p);
}

// #pragma mark -

// get_port_and_fs
static
status_t
get_port_and_fs(RequestPort** port, UserFileSystem** fileSystem)
{
	// get the request thread
	RequestThread* thread = RequestThread::GetCurrentThread();
	if (thread) {
		*port = thread->GetPort();
		*fileSystem = thread->GetFileSystem();
	} else {
		*port = UserlandFSServer::GetNotificationRequestPort();
		*fileSystem = UserlandFSServer::GetFileSystem();
		if (!*port || !*fileSystem)
			return B_BAD_VALUE;
	}
	return B_OK;
}

// notify_listener
int
notify_listener(int op, nspace_id nsid, vnode_id vnida, vnode_id vnidb,
	vnode_id vnidc, const char *name)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifyListenerRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->operation = op;
	request->nsid = nsid;
	request->vnida = vnida;
	request->vnidb = vnidb;
	request->vnidc = vnidc;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;
	// send the request
	UserlandRequestHandler handler(fileSystem, NOTIFY_LISTENER_REPLY);
	NotifyListenerReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// notify_select_event
void
notify_select_event(selectsync *sync, uint32 ref)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifySelectEventRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return;
	request->sync = sync;
	request->ref = ref;
	// send the request
	UserlandRequestHandler handler(fileSystem, NOTIFY_SELECT_EVENT_REPLY);
	NotifySelectEventReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return;
	RequestReleaser requestReleaser(port, reply);
	// process the reply: nothing to do
}

// send_notification
int
send_notification(port_id targetPort, long token, ulong what, long op,
	nspace_id nsida, nspace_id nsidb, vnode_id vnida, vnode_id vnidb,
	vnode_id vnidc, const char *name)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	SendNotificationRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->port = targetPort;
	request->token = token;
	request->what = what;
	request->operation = op;
	request->nsida = nsida;
	request->nsidb = nsidb;
	request->vnida = vnida;
	request->vnidb = vnidb;
	request->vnidc = vnidc;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;
	// send the request
	UserlandRequestHandler handler(fileSystem, SEND_NOTIFICATION_REPLY);
	SendNotificationReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// #pragma mark -

// get_vnode
_EXPORT
int
get_vnode(nspace_id nsid, vnode_id vnid, void** data)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	GetVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->nsid = nsid;
	request->vnid = vnid;
	// send the request
	UserlandRequestHandler handler(fileSystem, GET_VNODE_REPLY);
	GetVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	*data = reply->node;
	return error;
}

// put_vnode
_EXPORT
int
put_vnode(nspace_id nsid, vnode_id vnid)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	PutVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->nsid = nsid;
	request->vnid = vnid;
	// send the request
	UserlandRequestHandler handler(fileSystem, PUT_VNODE_REPLY);
	PutVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// new_vnode
_EXPORT
int
new_vnode(nspace_id nsid, vnode_id vnid, void* data)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NewVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->nsid = nsid;
	request->vnid = vnid;
	request->node = data;
	// send the request
	UserlandRequestHandler handler(fileSystem, NEW_VNODE_REPLY);
	NewVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// remove_vnode
_EXPORT
int
remove_vnode(nspace_id nsid, vnode_id vnid)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	RemoveVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->nsid = nsid;
	request->vnid = vnid;
	// send the request
	UserlandRequestHandler handler(fileSystem, REMOVE_VNODE_REPLY);
	RemoveVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// unremove_vnode
_EXPORT
int
unremove_vnode(nspace_id nsid, vnode_id vnid)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	UnremoveVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->nsid = nsid;
	request->vnid = vnid;
	// send the request
	UserlandRequestHandler handler(fileSystem, UNREMOVE_VNODE_REPLY);
	UnremoveVNodeReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// is_vnode_removed
_EXPORT
int
is_vnode_removed(nspace_id nsid, vnode_id vnid)
{
	// get the request port and the file system
	RequestPort* port;
	UserFileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;
	// prepare the request
	RequestAllocator allocator(port->GetPort());
	IsVNodeRemovedRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;
	request->nsid = nsid;
	request->vnid = vnid;
	// send the request
	UserlandRequestHandler handler(fileSystem, IS_VNODE_REMOVED_REPLY);
	IsVNodeRemovedReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);
	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return reply->result;
}

// #pragma mark -

// kernel_debugger
_EXPORT
void
kernel_debugger(const char *message)
{
	debugger(message);
}

// panic
_EXPORT
void
panic(const char *format, ...)
{
	char buffer[1024];
	strcpy(buffer, "PANIC: ");
	int32 prefixLen = strlen(buffer);
	int bufferSize = sizeof(buffer) - prefixLen;
	va_list args;
	va_start(args, format);
	// no vsnprintf() on PPC
	#if defined(__INTEL__)
		vsnprintf(buffer + prefixLen, bufferSize - 1, format, args);
	#else
		vsprintf(buffer + prefixLen, format, args);
	#endif
	va_end(args);
	buffer[sizeof(buffer) - 1] = '\0';
	debugger(buffer);
}

// parse_expression
_EXPORT
ulong
parse_expression(char *str)
{
	return 0;
}

// add_debugger_command
_EXPORT
int
add_debugger_command(char *name, int (*func)(int argc, char **argv),
	char *help)
{
	return B_OK;
}

// remove_debugger_command
_EXPORT
int
remove_debugger_command(char *name, int (*func)(int argc, char **argv))
{
	return B_OK;
}

// kprintf
_EXPORT
void
kprintf(const char *format, ...)
{
}

// spawn_kernel_thread
thread_id
spawn_kernel_thread(thread_entry function, const char *threadName,
	long priority, void *arg)
{
	return spawn_thread(function, threadName, priority, arg);
}

