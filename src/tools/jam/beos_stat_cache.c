// beos_stat_cache.c

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <OS.h>

#include "beos_stat_cache.h"
#include "pathsys.h"
#include "StatCacheServer.h"

#define SET_ERRNO_AND_RETURN(error) {	\
	if ((error) == B_OK)				\
		return 0;						\
	errno = (error);					\
	return -1;							\
}

// get_server_port
static
port_id
get_server_port()
{
	static port_id id = -1;
	static bool initialized = false;
	if (!initialized) {
		id = find_port(STAT_CACHE_SERVER_PORT_NAME);
		initialized = true;
	}
	return id;
}

// get_reply_port
static
port_id
get_reply_port()
{
	static port_id id = -1;
	if (id < 0)
		id = create_port(1, "stat cache reply port");
	return id;
}

// send_request
static
status_t
send_request(int32 command, const char *path)
{
	port_id requestPort = get_server_port();
	port_id replyPort = get_reply_port();
	stat_cache_request request;
	int requestSize;

	// get request port
	if (requestPort < 0)
		return requestPort;
	// get reply port
	if (replyPort < 0)
		return replyPort;
	// normalize the path
	if (!path || !normalize_path(path, request.path, sizeof(request.path)))
		return B_BAD_VALUE;
	requestSize = (request.path + strlen(request.path) + 1) - (char*)&request;
	// send request
	request.replyPort = replyPort;
	request.command = command;
	return write_port(requestPort, 0, &request, requestSize);
}

// receive_reply
static
status_t
receive_reply(void **_reply, int32 *_replySize, void *buffer, int32 replySize)
{
	port_id replyPort = get_reply_port();
	ssize_t bytesRead;
	void *reply;
	int32 code;

	// get reply port
	if (replyPort < 0)
		return replyPort;

	// get the reply size
	if (!buffer) {
		replySize = port_buffer_size(replyPort);
		if (replySize < 0)
			return replySize;
	}

	// allocate reply
	if (buffer) {
		reply = buffer;
	} else {
		reply = malloc(replySize);
		if (!reply)
			return B_NO_MEMORY;
	}

	// read the reply
	bytesRead = read_port(replyPort, &code, reply, replySize);
	if (bytesRead < 0) {
		if (!buffer)
			free(reply);
		return bytesRead;
	}
	if (bytesRead != replySize) {
		if (!buffer)
			free(reply);
		return B_ERROR;
	}

	if (_reply)
		*_reply = reply;
	if (_replySize)
		*_replySize = replySize;
	return B_OK;
}

// beos_stat_cache_stat
int
beos_stat_cache_stat(const char *filename, struct stat *st)
{
	stat_cache_stat_reply reply;
	status_t error;

	// fall back to standard, if there is no server
	if (get_server_port() < 0)
		return stat(filename, st);

	// send the request
	error = send_request(STAT_CACHE_COMMAND_STAT, filename);
	if (error != B_OK)
		SET_ERRNO_AND_RETURN(error);

	// get the reply
	error = receive_reply(NULL, NULL, &reply, sizeof(reply));
	if (error != B_OK)
		error = reply.error;
	if (error != B_OK)
		SET_ERRNO_AND_RETURN(error);

	*st = reply.st;
	return 0;
}

// beos_stat_cache_opendir
DIR *
beos_stat_cache_opendir(const char *dirName)
{
	stat_cache_readdir_reply *reply;
	int32 replySize;
	status_t error;

	// fall back to standard, if there is no server
	if (get_server_port() < 0)
		return opendir(dirName);

	// send the request
	error = send_request(STAT_CACHE_COMMAND_READDIR, dirName);
	if (error != B_OK) {
		errno = error;
		return NULL;
	}

	// get the reply
	error = receive_reply((void**)&reply, &replySize, NULL, 0);
	if (error != B_OK)
		error = reply->error;
	if (error != B_OK) {
		free(reply);
		errno = error;
		return NULL;
	}

	reply->clientData = reply->buffer;

	// a bit ugly, but anyway... 
	return (DIR*)reply;
}

// beos_stat_cache_readdir
struct dirent *
beos_stat_cache_readdir(DIR *dir)
{
	stat_cache_readdir_reply *reply;
	struct dirent *entry;

	// fall back to standard, if there is no server
	if (get_server_port() < 0)
		return readdir(dir);

	reply = (stat_cache_readdir_reply*)dir;
	if (reply->entryCount == 0)
		return NULL;

	entry = (struct dirent*)reply->clientData;

	// get the next entry
	if (--reply->entryCount > 0)
		reply->clientData = (uint8*)entry + entry->d_reclen;

	return entry;
}

// beos_stat_cache_closedir
int
beos_stat_cache_closedir(DIR *dir)
{
	stat_cache_readdir_reply *reply;

	// fall back to standard, if there is no server
	if (get_server_port() < 0)
		return closedir(dir);

	reply = (stat_cache_readdir_reply*)dir;
	free(reply);
	return 0;
}

