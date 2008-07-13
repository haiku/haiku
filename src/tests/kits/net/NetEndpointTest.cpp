/*
 * Copyright 2008, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT license.
 */


#include <Message.h>
#include <NetEndpoint.h>

#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>


static BNetAddress serverAddr("127.0.0.1", 1234);
static BNetAddress clientAddr("127.0.0.1", 51234);


static int problemCount = 0;


void
checkAddrsAreEqual(const BNetAddress& na1, const BNetAddress& na2,
	const char* fmt)
{
	in_addr addr1, addr2;
	unsigned short port1, port2;
	na1.GetAddr(addr1, &port1);
	na2.GetAddr(addr2, &port2);
	if (addr1.s_addr == addr2.s_addr && port1 == port2)
		return;
	fprintf(stderr, fmt, addr1.s_addr, port1, addr2.s_addr, port2);
	exit(1);
}


void
checkArchive(const BNetEndpoint ne, int32 protocol,
	const BNetAddress& localNetAddress, const BNetAddress& remoteNetAddress)
{
	in_addr localAddr, remoteAddr;
	unsigned short localPort, remotePort;
	localNetAddress.GetAddr(localAddr, &localPort);
	remoteNetAddress.GetAddr(remoteAddr, &remotePort);

	BMessage archive(0UL);
	status_t status = ne.Archive(&archive);
	if (status != B_OK) {
		fprintf(stderr, "Archive() failed - %lx:%s\n", status, 
			strerror(status));
		problemCount++;
		exit(1);
	}
	const char* arcClass;
	if (archive.FindString("class", &arcClass) != B_OK) {
		fprintf(stderr, "'class' not found in archive\n");
		problemCount++;
		exit(1);
	}
	if (strcmp(arcClass, "BNetEndpoint") != 0) {
		fprintf(stderr, "expected 'class' to be 'BNetEndpoint' - is '%s'\n",
			arcClass);
		problemCount++;
		exit(1);
	}

	if (ne.LocalAddr().InitCheck() == B_OK) {
		int32 arcAddr;
		if (archive.FindInt32("_BNetEndpoint_addr_addr", &arcAddr) != B_OK) {
			fprintf(stderr, "'_BNetEndpoint_addr_addr' not found in archive\n");
			problemCount++;
			exit(1);
		}
		if ((uint32)localAddr.s_addr != (uint32)arcAddr) {
			fprintf(stderr, 
				"expected '_BNetEndpoint_addr_addr' to be %x - is %x\n",
				localAddr.s_addr, (unsigned int)arcAddr);
			problemCount++;
			exit(1);
		}
		int16 arcPort;
		if (archive.FindInt16("_BNetEndpoint_addr_port", &arcPort) != B_OK) {
			fprintf(stderr, "'_BNetEndpoint_addr_port' not found in archive\n");
			problemCount++;
			exit(1);
		}
		if ((uint16)localPort != (uint16)arcPort) {
			fprintf(stderr, 
				"expected '_BNetEndpoint_addr_port' to be %d - is %d\n",
				localPort, (int)arcPort);
			problemCount++;
			exit(1);
		}
	}

	if (ne.RemoteAddr().InitCheck() == B_OK) {
		int32 arcAddr;
		if (archive.FindInt32("_BNetEndpoint_peer_addr", &arcAddr) != B_OK) {
			fprintf(stderr, "'_BNetEndpoint_peer_addr' not found in archive\n");
			problemCount++;
			exit(1);
		}
		if ((uint32)remoteAddr.s_addr != (uint32)arcAddr) {
			fprintf(stderr, 
				"expected '_BNetEndpoint_peer_addr' to be %x - is %x\n",
				remoteAddr.s_addr, (unsigned int)arcAddr);
			problemCount++;
			exit(1);
		}
		int16 arcPort;
		if (archive.FindInt16("_BNetEndpoint_peer_port", &arcPort) != B_OK) {
			fprintf(stderr, "'_BNetEndpoint_peer_port' not found in archive\n");
			problemCount++;
			exit(1);
		}
		if ((uint16)remotePort != (uint16)arcPort) {
			fprintf(stderr, 
				"expected '_BNetEndpoint_peer_port' to be %u - is %u\n",
				remotePort, (unsigned short)arcPort);
			problemCount++;
			exit(1);
		}
	}

	int64 arcTimeout;
	if (archive.FindInt64("_BNetEndpoint_timeout", &arcTimeout) != B_OK) {
		fprintf(stderr, "'_BNetEndpoint_timeout' not found in archive\n");
		problemCount++;
		exit(1);
	}
	if (arcTimeout != B_INFINITE_TIMEOUT) {
		fprintf(stderr, 
			"expected '_BNetEndpoint_timeout' to be %llu - is %llu\n",
			B_INFINITE_TIMEOUT, (uint64)arcTimeout);
		problemCount++;
		exit(1);
	}

	int32 arcProtocol;
	if (archive.FindInt32("_BNetEndpoint_proto", &arcProtocol) != B_OK) {
		fprintf(stderr, "'_BNetEndpoint_proto' not found in archive\n");
		problemCount++;
		exit(1);
	}
	if (arcProtocol != protocol) {
		fprintf(stderr, "expected '_BNetEndpoint_proto' to be %d - is %d\n",
			(int)protocol, (int)arcProtocol);
		problemCount++;
		exit(1);
	}
	
	BNetEndpoint* clone 
		= dynamic_cast<BNetEndpoint *>(BNetEndpoint::Instantiate(&archive));
	if (!clone) {
		fprintf(stderr, "unable to instantiate endpoint from archive\n");
		problemCount++;
		exit(1);
	}
	delete clone;
}

void testServer(thread_id clientThread)
{
	char buf[1];
	
	// check simple UDP "connection"
	BNetEndpoint server(SOCK_DGRAM);
	for(int i=0; i < 2; ++i) {
		status_t status = server.Bind(serverAddr);
		if (status != B_OK) {
			fprintf(stderr, "Bind() failed in testServer - %s\n",
				strerror(status));
			problemCount++;
			exit(1);
		}
		
		checkAddrsAreEqual(server.LocalAddr(), serverAddr,
			"LocalAddr() doesn't match serverAddr\n");
	
		if (i == 0)
			resume_thread(clientThread);

		BNetAddress remoteAddr;
		status = server.ReceiveFrom(buf, 1, remoteAddr, 0);
		if (status < B_OK) {
			fprintf(stderr, "ReceiveFrom() failed in testServer - %s\n",
				strerror(status));
			problemCount++;
			exit(1);
		}
	
		if (buf[0] != 'U') {
			fprintf(stderr, "expected to receive %c but got %c\n", 'U', buf[0]);
			problemCount++;
			exit(1);
		}

		checkAddrsAreEqual(remoteAddr, clientAddr, 
			"remoteAddr(%x:%d) doesn't match clientAddr(%x:%d)\n");

		checkArchive(server, SOCK_DGRAM, serverAddr, clientAddr);

		server.Close();
	}
	
	// now switch to TCP and try again
	server.SetProtocol(SOCK_STREAM);
	status_t status = server.Bind(serverAddr);
	if (status != B_OK) {
		fprintf(stderr, "Bind() failed in testServer - %s\n",
			strerror(status));
		problemCount++;
		exit(1);
	}
	
	checkAddrsAreEqual(server.LocalAddr(), serverAddr,
		"LocalAddr() doesn't match serverAddr\n");

	status = server.Listen();
	BNetEndpoint* acceptedConn = server.Accept();
	if (acceptedConn == NULL) {
		fprintf(stderr, "Accept() failed in testServer\n");
		problemCount++;
		exit(1);
	}

	const BNetAddress& remoteAddr = acceptedConn->RemoteAddr();
	checkAddrsAreEqual(remoteAddr, clientAddr, 
		"remoteAddr(%x:%d) doesn't match clientAddr(%x:%d)\n");
		
	status = acceptedConn->Receive(buf, 1);
	if (status < B_OK) {
		fprintf(stderr, "Receive() failed in testServer - %s\n",
			strerror(status));
		problemCount++;
		exit(1);
	}
	delete acceptedConn;

	if (buf[0] != 'T') {
		fprintf(stderr, "expected to receive %c but got %c\n", 'T', buf[0]);
		problemCount++;
		exit(1);
	}

	checkArchive(server, SOCK_STREAM, serverAddr, clientAddr);

	server.Close();
}


int32 testClient(void *)
{
	BNetEndpoint client(SOCK_DGRAM);
	printf("testing udp...\n");
	for(int i=0; i < 2; ++i) {
		status_t status = client.Bind(clientAddr);
		if (status != B_OK) {
			fprintf(stderr, "Bind() failed in testClient - %s\n",
				strerror(status));
			problemCount++;
			exit(1);
		}
		
		checkAddrsAreEqual(client.LocalAddr(), clientAddr,
			"LocalAddr(%x:%d) doesn't match clientAddr(%x:%d)\n");
	
		status = client.SendTo("U", 1, serverAddr, 0);
		if (status < B_OK) {
			fprintf(stderr, "SendTo() failed in testClient - %s\n",
				strerror(status));
			problemCount++;
			exit(1);
		}

		checkArchive(client, SOCK_DGRAM, clientAddr, serverAddr);

		sleep(1);

		client.Close();
	}

	sleep(1);

	printf("testing tcp...\n");
	// now switch to TCP and try again
	client.SetProtocol(SOCK_STREAM);
	status_t status = client.Bind(clientAddr);
	if (status != B_OK) {
		fprintf(stderr, "Bind() failed in testClient - %s\n",
			strerror(status));
		problemCount++;
		exit(1);
	}
	
	checkAddrsAreEqual(client.LocalAddr(), clientAddr,
		"LocalAddr(%x:%d) doesn't match clientAddr(%x:%d)\n");

	status = client.Connect(serverAddr);
	if (status < B_OK) {
		fprintf(stderr, "Connect() failed in testClient - %s\n",
			strerror(status));
		problemCount++;
		exit(1);
	}
	status = client.Send("T", 1);
	if (status < B_OK) {
		fprintf(stderr, "Send() failed in testClient - %s\n",
			strerror(status));
		problemCount++;
		exit(1);
	}

	checkArchive(client, SOCK_STREAM, clientAddr, serverAddr);

	client.Close();

	return B_OK;
}


int
main(int argc, const char* const* argv)
{
	BNetEndpoint dummy(SOCK_DGRAM);
	if (sizeof(dummy) != 208) {
		fprintf(stderr, "expected sizeof(netEndpoint) to be 208 - is %ld\n",
			sizeof(dummy));
		exit(1);
	}
	dummy.Close();

	// start thread for client
	thread_id tid = spawn_thread(testClient, "client", B_NORMAL_PRIORITY, NULL);
	if (tid < 0) {
		fprintf(stderr, "spawn_thread() failed: %s\n", strerror(tid));
		exit(1);
	}

	testServer(tid);

	status_t clientStatus;
	wait_for_thread(tid, &clientStatus);

	if (!problemCount)
		printf("Everything went fine.\n");
	
	return 0;
}
