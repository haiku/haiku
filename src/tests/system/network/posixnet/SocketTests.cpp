#include "SocketTests.h"

#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


// This test reproduces a KDL from issue #13927 where a socket is created
// and an attempt is made to connect to an address, which fails. The socket
// is closed and then reused to connect to a *different* address.
void SocketTests::ClientSocketReuseTest()
{
	// TODO: Try to find unused ports instead of using these hard-coded ones.
	const uint16_t kFirstPort = 14025;
	const uint16_t kSecondPort = 14026;

	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	CPPUNIT_ASSERT(fd > 0);

	// Connect to 127.0.0.1:kFirstPort
	sockaddr_in address;
	memset(&address, 0, sizeof(sockaddr_in));
	address.sin_family = AF_INET;
	address.sin_port = htons(kFirstPort);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	int connect_result = ::connect(
		fd,
		reinterpret_cast<const sockaddr *>(&address),
		sizeof(struct sockaddr));
	CPPUNIT_ASSERT_EQUAL(connect_result, -1);

	// Connection to 127.0.0.1:kFirstPort failed as expected.
	// Now try connecting to 127.0.0.1:kSecondPort.
	address.sin_port = htons(kSecondPort);
	connect_result = ::connect(
		fd,
		reinterpret_cast<const sockaddr *>(&address),
		sizeof(struct sockaddr));
	CPPUNIT_ASSERT_EQUAL(connect_result, -1);

	close(fd);
}


void SocketTests::AddTests(BTestSuite &parent) {
	CppUnit::TestSuite &suite = *new CppUnit::TestSuite("SocketTests");

	suite.addTest(new CppUnit::TestCaller<SocketTests>(
		"SocketTests::ClientSocketReuseTest",
		&SocketTests::ClientSocketReuseTest));

	parent.addTest("SocketTests", &suite);
}
