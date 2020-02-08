/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Kyle Ambroff-Kao, kyle@ambroffkao.com
 */
#include "TestServer.h"

#include <netinet/in.h>
#include <posix/libgen.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <AutoDeleter.h>
#include <TestShell.h>


namespace {

template <typename T>
std::string to_string(T value)
{
	std::ostringstream s;
	s << value;
	return s.str();
}


void exec(const std::vector<std::string>& args)
{
	const char** argv = new const char*[args.size() + 1];
	ArrayDeleter<const char*> _(argv);

	for (size_t i = 0; i < args.size(); ++i) {
		argv[i] = args[i].c_str();
	}
	argv[args.size()] = NULL;

	execv(args[0].c_str(), const_cast<char* const*>(argv));
}

}


TestServer::TestServer(TestServerMode mode)
	:
	fMode(mode),
	fRunning(false),
	fChildPid(-1),
	fSocketFd(-1),
	fServerPort(0)
{
}


TestServer::~TestServer()
{
	if (fChildPid != -1) {
		::kill(fChildPid, SIGTERM);

		pid_t result = -1;
		while (result != fChildPid) {
			result = ::waitpid(fChildPid, NULL, 0);
		}
	}

	if (fSocketFd != -1) {
		::close(fSocketFd);
		fSocketFd = -1;
	}
}


// The job of this method is to spawn a child process that will later be killed
// by the destructor. The steps are roughly:
//
// 1. If the child server process is already running, return early
// 2. Choose a random TCP port by binding to the loopback interface.
// 3. Spawn a child Python process to run testserver.py.
// 4. Return immediately allowing the tests to be performed by the caller of
//    TestServer::StartIfNotRunning(). We don't have to wait for the child
//    process to start up because the socket has already been created. The
//    tests will block until accept() is called in the child.
status_t TestServer::StartIfNotRunning()
{
	if (fRunning == true) {
		return B_OK;
	}

	// Bind to a random unused TCP port.
	{
		// Create socket with port 0 to get an unused one selected by the
		// kernel.
		int socket_fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (socket_fd == -1) {
			fprintf(
				stderr,
				"ERROR: Unable to create socket: %s\n",
				strerror(errno));
			return B_ERROR;
		}

		fSocketFd = socket_fd;

		// We may quickly reclaim the same socket between test runs, so allow
		// for reuse.
		{
			int reuse = 1;
			int result = ::setsockopt(
				socket_fd,
				SOL_SOCKET,
				SO_REUSEPORT,
				&reuse,
				sizeof(reuse));
			if (result == -1) {
				fprintf(
					stderr,
					"ERROR: Unable to set socket options on fd %d: %s\n",
					socket_fd,
					strerror(errno));
				return B_ERROR;
			}
		}

		// Bind to loopback 127.0.0.1
		struct sockaddr_in server_address;
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		int bind_result = ::bind(
			socket_fd,
			reinterpret_cast<struct sockaddr*>(&server_address),
			sizeof(server_address));
		if (bind_result == -1) {
			fprintf(
				stderr,
				"ERROR: Unable to bind to loopback interface: %s\n",
				strerror(errno));
			return B_ERROR;
		}

		// Listen is apparently required before getsockname will work.
		if (::listen(socket_fd, 32) == -1) {
			fprintf(stderr, "ERROR: listen() failed: %s\n", strerror(errno));
			return B_ERROR;
		}

		// Now get the port from the socket.
		socklen_t server_address_length = sizeof(server_address);
		::getsockname(
			socket_fd,
			reinterpret_cast<struct sockaddr*>(&server_address),
			&server_address_length);
		fServerPort = ntohs(server_address.sin_port);
	}

	fprintf(stderr, "Binding to port %d for test server\n", fServerPort);

	pid_t child = ::fork();
	if (child < 0)
		return B_ERROR;

	if (child > 0) {
		// The child process has started. It may take a short amount of time
		// before the child process is ready to call accept(), but that's OK.
		//
		// Since the socket has already been created above, the tests will not
		// get ECONNREFUSED and will block until the child process calls
		// accept(). So we don't have to busy loop here waiting for a
		// connection to the child.
		fRunning = true;
		fChildPid = child;
		return B_OK;
	}

	// This is the child process. We can exec the server process.
	char* testFileSource = strdup(__FILE__);
	MemoryDeleter _(testFileSource);

	std::string testSrcDir(dirname(testFileSource));
	std::string testServerScript = testSrcDir + "/" + "testserver.py";

	std::string socket_fd_string = to_string(fSocketFd);
	std::string server_port_string = to_string(fServerPort);

	std::vector<std::string> child_process_args;
	child_process_args.push_back("/bin/python3");
	child_process_args.push_back(testServerScript);
	child_process_args.push_back("--port");
	child_process_args.push_back(server_port_string);
	child_process_args.push_back("--fd");
	child_process_args.push_back(socket_fd_string);

	if (fMode == TEST_SERVER_MODE_HTTPS) {
		child_process_args.push_back("--use-tls");
	}

	exec(child_process_args);

	// If we reach this point we failed to load the Python image.
	fprintf(
		stderr,
		"Unable to spawn %s: %s\n",
		testServerScript.c_str(),
		strerror(errno));
	exit(1);
}


BUrl TestServer::BaseUrl() const
{
	std::string scheme;
	switch(fMode) {
	case TEST_SERVER_MODE_HTTP:
		scheme = "http://";
		break;

	case TEST_SERVER_MODE_HTTPS:
		scheme = "https://";
		break;
	}

	std::string baseUrl = scheme + "127.0.0.1:" + to_string(fServerPort) + "/";
	return BUrl(baseUrl.c_str());
}
