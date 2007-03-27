// kernel_emu.cpp

#include "kernel_emu.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "RequestPort.h"
#include "Requests.h"
#include "RequestThread.h"
#include "UserlandFSServer.h"
#include "UserlandRequestHandler.h"


// Taken from the Haiku Storage Kit (storage_support.cpp)
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
static status_t
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
UserlandFS::KernelEmu::new_path(const char *path, char **copy)
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
UserlandFS::KernelEmu::free_path(char *p)
{
	free(p);
}


// #pragma mark -


// get_port_and_fs
static status_t
get_port_and_fs(RequestPort** port, FileSystem** fileSystem)
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
status_t
UserlandFS::KernelEmu::notify_listener(int32 operation, uint32 details,
	mount_id device, vnode_id oldDirectory, vnode_id directory,
	vnode_id node, const char* oldName, const char* name)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifyListenerRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->operation = operation;
	request->details = details;
	request->device = device;
	request->oldDirectory = oldDirectory;
	request->directory = directory;
	request->node = node;
	error = allocator.AllocateString(request->oldName, oldName);
	if (error != B_OK)
		return error;
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
status_t
UserlandFS::KernelEmu::notify_select_event(selectsync *sync, uint32 ref,
	uint8 event, bool unspecifiedEvent)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifySelectEventRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->sync = sync;
	request->ref = ref;
	request->event = event;
	request->unspecifiedEvent = unspecifiedEvent;

	// send the request
	UserlandRequestHandler handler(fileSystem, NOTIFY_SELECT_EVENT_REPLY);
	NotifySelectEventReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	if (reply->error != B_OK)
		return reply->error;
	return error;
}

// send_notification
status_t
UserlandFS::KernelEmu::notify_query(port_id targetPort, int32 token,
	int32 operation, mount_id device, vnode_id directory, const char* name,
	vnode_id node)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	NotifyQueryRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->port = targetPort;
	request->token = token;
	request->operation = operation;
	request->device = device;
	request->directory = directory;
	request->node = node;
	error = allocator.AllocateString(request->name, name);
	if (error != B_OK)
		return error;

	// send the request
	UserlandRequestHandler handler(fileSystem, NOTIFY_QUERY_REPLY);
	NotifyQueryReply* reply;
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
status_t
UserlandFS::KernelEmu::get_vnode(mount_id nsid, vnode_id vnid, fs_vnode* data)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
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
status_t
UserlandFS::KernelEmu::put_vnode(mount_id nsid, vnode_id vnid)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
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
status_t
UserlandFS::KernelEmu::new_vnode(mount_id nsid, vnode_id vnid, fs_vnode data)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
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

// publish_vnode
status_t
UserlandFS::KernelEmu::publish_vnode(mount_id nsid, vnode_id vnid,
	fs_vnode data)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	PublishVNodeRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;
	request->node = data;

	// send the request
	UserlandRequestHandler handler(fileSystem, PUBLISH_VNODE_REPLY);
	PublishVNodeReply* reply;
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
status_t
UserlandFS::KernelEmu::remove_vnode(mount_id nsid, vnode_id vnid)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
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
status_t
UserlandFS::KernelEmu::unremove_vnode(mount_id nsid, vnode_id vnid)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
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

// get_vnode_removed
status_t
UserlandFS::KernelEmu::get_vnode_removed(mount_id nsid, vnode_id vnid,
	bool* removed)
{
	// get the request port and the file system
	RequestPort* port;
	FileSystem* fileSystem;
	status_t error = get_port_and_fs(&port, &fileSystem);
	if (error != B_OK)
		return error;

	// prepare the request
	RequestAllocator allocator(port->GetPort());
	GetVNodeRemovedRequest* request;
	error = AllocateRequest(allocator, &request);
	if (error != B_OK)
		return error;

	request->nsid = nsid;
	request->vnid = vnid;

	// send the request
	UserlandRequestHandler handler(fileSystem, GET_VNODE_REMOVED_REPLY);
	GetVNodeRemovedReply* reply;
	error = port->SendRequest(&allocator, &handler, (Request**)&reply);
	if (error != B_OK)
		return error;
	RequestReleaser requestReleaser(port, reply);

	// process the reply
	*removed = reply->removed;
	return reply->error;
}

// #pragma mark -

// kernel_debugger
void
UserlandFS::KernelEmu::kernel_debugger(const char *message)
{
	debugger(message);
}

// vpanic
void
UserlandFS::KernelEmu::vpanic(const char *format, va_list args)
{
	char buffer[1024];
	strcpy(buffer, "PANIC: ");
	int32 prefixLen = strlen(buffer);
	int bufferSize = sizeof(buffer) - prefixLen;

	// no vsnprintf() on PPC
	#if defined(__INTEL__)
		vsnprintf(buffer + prefixLen, bufferSize - 1, format, args);
	#else
		vsprintf(buffer + prefixLen, format, args);
	#endif

	buffer[sizeof(buffer) - 1] = '\0';
	debugger(buffer);
}

// panic
void
UserlandFS::KernelEmu::panic(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vpanic(format, args);
	va_end(args);
}

// vdprintf
void
UserlandFS::KernelEmu::vdprintf(const char *format, va_list args)
{
	vprintf(format, args);
}

// dprintf
void
UserlandFS::KernelEmu::dprintf(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	dprintf(format, args);
	va_end(args);
}

void
UserlandFS::KernelEmu::dump_block(const char *buffer, int size,
	const char *prefix)
{
	// TODO: Implement!
}

// parse_expression
//ulong
//parse_expression(char *str)
//{
//	return 0;
//}

// add_debugger_command
int
UserlandFS::KernelEmu::add_debugger_command(char *name,
	int (*func)(int argc, char **argv), char *help)
{
	return B_OK;
}

// remove_debugger_command
int
UserlandFS::KernelEmu::remove_debugger_command(char *name,
	int (*func)(int argc, char **argv))
{
	return B_OK;
}

// parse_expression
uint32
UserlandFS::KernelEmu::parse_expression(const char *string)
{
	return 0;
}


// kprintf
//void
//kprintf(const char *format, ...)
//{
//}

// spawn_kernel_thread
thread_id
UserlandFS::KernelEmu::spawn_kernel_thread(thread_entry function,
	const char *threadName, long priority, void *arg)
{
	return spawn_thread(function, threadName, priority, arg);
}
