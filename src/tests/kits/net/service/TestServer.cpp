/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Kyle Ambroff-Kao, kyle@ambroffkao.com
 */
#include "TestServer.h"

#include <errno.h>
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


// Return the path of a file path relative to this source file.
std::string TestFilePath(const std::string& relativePath)
{
	char *testFileSource = strdup(__FILE__);
	MemoryDeleter _(testFileSource);

	std::string testSrcDir(::dirname(testFileSource));

	return testSrcDir + "/" + relativePath;
}

}


RandomTCPServerPort::RandomTCPServerPort()
	:
	fInitStatus(B_NOT_INITIALIZED),
	fSocketFd(-1),
	fServerPort(0)
{
	// Create socket with port 0 to get an unused one selected by the
	// kernel.
	int socket_fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1) {
		fprintf(
			stderr,
			"ERROR: Unable to create socket: %s\n",
			strerror(errno));
		fInitStatus = B_ERROR;
		return;
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
			fInitStatus = errno;
			fprintf(
				stderr,
				"ERROR: Unable to set socket options on fd %d: %s\n",
				socket_fd,
				strerror(fInitStatus));
			return;
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
		fInitStatus = errno;
		fprintf(
			stderr,
			"ERROR: Unable to bind to loopback interface: %s\n",
			strerror(fInitStatus));
		return;
	}

	// Listen is apparently required before getsockname will work.
	if (::listen(socket_fd, 32) == -1) {
		fInitStatus = errno;
		fprintf(stderr, "ERROR: listen() failed: %s\n", strerror(fInitStatus));

		return;
	}

	// Now get the port from the socket.
	socklen_t server_address_length = sizeof(server_address);
	::getsockname(
		socket_fd,
		reinterpret_cast<struct sockaddr*>(&server_address),
		&server_address_length);
	fServerPort = ntohs(server_address.sin_port);

	fInitStatus = B_OK;
}


RandomTCPServerPort::~RandomTCPServerPort()
{
	if (fSocketFd != -1) {
		::close(fSocketFd);
		fSocketFd = -1;
		fInitStatus = B_NOT_INITIALIZED;
	}
}


status_t RandomTCPServerPort::InitCheck() const
{
	return fInitStatus;
}


int RandomTCPServerPort::FileDescriptor() const
{
	return fSocketFd;
}


uint16_t RandomTCPServerPort::Port() const
{
	return fServerPort;
}


ChildProcess::ChildProcess()
	:
	fChildPid(-1)
{
}


ChildProcess::~ChildProcess()
{
	if (fChildPid != -1) {
		::kill(fChildPid, SIGTERM);

		pid_t result = -1;
		while (result != fChildPid) {
			result = ::waitpid(fChildPid, NULL, 0);
		}
	}
}


// The job of this method is to spawn a child process that will later be killed
// by the destructor.
status_t ChildProcess::Start(const std::vector<std::string>& args)
{
	if (fChildPid != -1) {
		return B_ALREADY_RUNNING;
	}

	pid_t child = ::fork();
	if (child < 0)
		return B_ERROR;

	if (child > 0) {
		fChildPid = child;
		return B_OK;
	}

	// This is the child process. We can exec image provided in args.
	exec(args);

	// If we reach this point we failed to load the Python image.
	std::ostringstream ostr;

	for (std::vector<std::string>::const_iterator iter = args.begin();
		 iter != args.end();
		 ++iter) {
		ostr << " " << *iter;
	}

	fprintf(
		stderr,
		"Unable to spawn `%s': %s\n",
		ostr.str().c_str(),
		strerror(errno));
	exit(1);
}


TestServer::TestServer(TestServerMode mode)
	:
	fMode(mode)
{
}


// Start a child testserver.py process with the random TCP port chosen by
// fPort.
status_t TestServer::Start()
{
	if (fPort.InitCheck() != B_OK) {
		return fPort.InitCheck();
	}

	// This is the child process. We can exec the server process.
	std::vector<std::string> child_process_args;
	child_process_args.push_back("/bin/python3");
	child_process_args.push_back(TestFilePath("testserver.py"));
	child_process_args.push_back("--port");
	child_process_args.push_back(to_string(fPort.Port()));
	child_process_args.push_back("--fd");
	child_process_args.push_back(to_string(fPort.FileDescriptor()));

	if (fMode == TEST_SERVER_MODE_HTTPS) {
		child_process_args.push_back("--use-tls");
	}

	// After this the child process has started. It may take a short amount of
	// time before the child process is ready to call accept(), but that's OK.
	//
	// Since the socket has already been created above, the tests will not
	// get ECONNREFUSED and will block until the child process calls
	// accept(). So we don't have to busy loop here waiting for a
	// connection to the child.
	return fChildProcess.Start(child_process_args);
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

	std::string port_string = to_string(fPort.Port());

	std::string baseUrl = scheme + "127.0.0.1:" + port_string + "/";
	return BUrl(baseUrl.c_str());
}


// Start a child proxy.py process using the random TCP port chosen by fPort.
status_t TestProxyServer::Start()
{
	if (fPort.InitCheck() != B_OK) {
		return fPort.InitCheck();
	}

	std::vector<std::string> child_process_args;
	child_process_args.push_back("/bin/python3");
	child_process_args.push_back(TestFilePath("proxy.py"));
	child_process_args.push_back("--port");
	child_process_args.push_back(to_string(fPort.Port()));
	child_process_args.push_back("--fd");
	child_process_args.push_back(to_string(fPort.FileDescriptor()));

	// After this the child process has started. It may take a short amount of
	// time before the child process is ready to call accept(), but that's OK.
	//
	// Since the socket has already been created above, the tests will not
	// get ECONNREFUSED and will block until the child process calls
	// accept(). So we don't have to busy loop here waiting for a
	// connection to the child.
	return fChildProcess.Start(child_process_args);
}


uint16_t TestProxyServer::Port() const
{
	return fPort.Port();
}
