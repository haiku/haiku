#ifndef SOCKET_TESTS_H
#define SOCKET_TESTS_H

#include <TestCase.h>
#include <TestSuite.h>

class SocketTests : public BTestCase {
 public:
			void	ClientSocketReuseTest();
			void	TcpRecvmsgTest();

	static	void	AddTests(BTestSuite& suite);
};

#endif // SOCKET_TESTS_H
