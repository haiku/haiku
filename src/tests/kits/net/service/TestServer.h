/*
 * Copyright 2020 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Kyle Ambroff-Kao, kyle@ambroffkao.com
 */
#ifndef TEST_SERVER_H
#define TEST_SERVER_H

#include <os/support/SupportDefs.h>
#include <os/support/Url.h>


enum TestServerMode {
	TEST_SERVER_MODE_HTTP,
	TEST_SERVER_MODE_HTTPS,
};


class TestServer {
public:
	TestServer(TestServerMode mode);
	~TestServer();

	status_t	StartIfNotRunning();
	BUrl		BaseUrl()	const;

private:
	TestServerMode	fMode;
	bool			fRunning;
	pid_t			fChildPid;
	int				fSocketFd;
	uint16_t		fServerPort;
};


#endif // TEST_SERVER_H
