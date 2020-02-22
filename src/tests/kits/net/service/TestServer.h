/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Kyle Ambroff-Kao, kyle@ambroffkao.com
 */
#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include <string>
#include <vector>

#include <os/support/SupportDefs.h>
#include <os/support/Url.h>


// Binds to a random unused TCP port.
class RandomTCPServerPort {
public:
						RandomTCPServerPort();
						~RandomTCPServerPort();

	status_t			InitCheck()							const;
	int					FileDescriptor()					const;
	uint16_t			Port()								const;

private:
	status_t			fInitStatus;
	int					fSocketFd;
	uint16_t			fServerPort;
};


class ChildProcess {
public:
						ChildProcess();
						~ChildProcess();

	status_t			Start(const std::vector<std::string>& args);
private:
	pid_t				fChildPid;
};


enum TestServerMode {
	TEST_SERVER_MODE_HTTP,
	TEST_SERVER_MODE_HTTPS,
};


class TestServer {
public:
						TestServer(TestServerMode mode);

	status_t			Start();
	BUrl				BaseUrl()							const;

private:
	TestServerMode		fMode;
	ChildProcess		fChildProcess;
	RandomTCPServerPort fPort;
};


class TestProxyServer {
public:
	status_t			Start();
	uint16_t			Port()								const;

private:
	ChildProcess		fChildProcess;
	RandomTCPServerPort	fPort;
};


#endif // TEST_SERVER_H
