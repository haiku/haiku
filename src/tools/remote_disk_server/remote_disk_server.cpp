/*
 * Copyright 2005-2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <boot/net/RemoteDiskDefs.h>


#if defined(__BEOS__) && !defined(__HAIKU__)
typedef int socklen_t;
#endif


#if __BYTE_ORDER == __LITTLE_ENDIAN

static inline
uint64_t swap_uint64(uint64_t data)
{
	return ((data & 0xff) << 56)
		| ((data & 0xff00) << 40)
		| ((data & 0xff0000) << 24)
		| ((data & 0xff000000) << 8)
		| ((data >> 8) & 0xff000000)
		| ((data >> 24) & 0xff0000)
		| ((data >> 40) & 0xff00)
		| ((data >> 56) & 0xff);
}

#define host_to_net64(data)	swap_uint64(data)
#define net_to_host64(data)	swap_uint64(data)

#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define host_to_net64(data)	(data)
#define net_to_host64(data)	(data)
#endif

#undef htonll
#undef ntohll
#define htonll(data)	host_to_net64(data)
#define ntohll(data)	net_to_host64(data)


class Server {
public:
	Server(const char *fileName)
		: fImagePath(fileName),
		  fImageFD(-1),
		  fImageSize(0),
		  fSocket(-1)
	{
	}

	int Run()
	{
		_CreateSocket();

		// main server loop
		for (;;) {
			// receive
			fClientAddress.sin_family = AF_INET;
			fClientAddress.sin_port = 0;
			fClientAddress.sin_addr.s_addr = htonl(INADDR_ANY);
			socklen_t addrSize = sizeof(fClientAddress);
			char buffer[2048];
			ssize_t bytesRead = recvfrom(fSocket, buffer, sizeof(buffer), 0,
								(sockaddr*)&fClientAddress, &addrSize);
			// handle error
			if (bytesRead < 0) {
				if (errno == EINTR)
					continue;
				fprintf(stderr, "Error: Failed to read from socket: %s.\n",
					strerror(errno));
				exit(1);
			}

			// short package?
			if (bytesRead < (ssize_t)sizeof(remote_disk_header)) {
				fprintf(stderr, "Dropping short request package (%d bytes).\n",
					(int)bytesRead);
				continue;
			}

			fRequest = (remote_disk_header*)buffer;
			fRequestSize = bytesRead;

			switch (fRequest->command) {
				case REMOTE_DISK_HELLO_REQUEST:
					_HandleHelloRequest();
					break;

				case REMOTE_DISK_READ_REQUEST:
					_HandleReadRequest();
					break;

				case REMOTE_DISK_WRITE_REQUEST:
					_HandleWriteRequest();
					break;

				default:
					fprintf(stderr, "Ignoring invalid request %d.\n",
						(int)fRequest->command);
					break;
			}
		}
	
		return 0;
	}

private:
	void _OpenImage(bool reopen)
	{
		// already open?
		if (fImageFD >= 0) {
			if (!reopen)
				return;

			close(fImageFD);
			fImageFD = -1;
			fImageSize = 0;
		}

		// open the image
		fImageFD = open(fImagePath, O_RDWR);
		if (fImageFD < 0) {
			fprintf(stderr, "Error: Failed to open \"%s\": %s.\n", fImagePath,
				strerror(errno));
			exit(1);
		}

		// get its size
		struct stat st;
		if (fstat(fImageFD, &st) < 0) {
			fprintf(stderr, "Error: Failed to stat \"%s\": %s.\n", fImagePath,
				strerror(errno));
			exit(1);
		}
		fImageSize = st.st_size;
	}

	void _CreateSocket()
	{
		// create a socket
		fSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (fSocket < 0) {
			fprintf(stderr, "Error: Failed to create a socket: %s.",
				strerror(errno));
			exit(1);
		}

		// bind it to the port
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(REMOTE_DISK_SERVER_PORT);
		addr.sin_addr.s_addr = INADDR_ANY;
		if (bind(fSocket, (sockaddr*)&addr, sizeof(addr)) < 0) {
			fprintf(stderr, "Error: Failed to bind socket to port %hu: %s\n",
				REMOTE_DISK_SERVER_PORT, strerror(errno));
			exit(1);
		}
	}

	void _HandleHelloRequest()
	{
		printf("HELLO request\n");

		_OpenImage(true);

		remote_disk_header reply;
		reply.offset = htonll(fImageSize);

		reply.command = REMOTE_DISK_HELLO_REPLY;	
		_SendReply(&reply, sizeof(remote_disk_header));
	}

	void _HandleReadRequest()
	{
		_OpenImage(false);

		char buffer[2048];
		remote_disk_header *reply = (remote_disk_header*)buffer;
		uint64_t offset = ntohll(fRequest->offset);
		int16_t size = ntohs(fRequest->size);
		int16_t result = 0;

		printf("READ request: offset: %llu, %hd bytes\n", offset, size);

		if (offset < (uint64_t)fImageSize && size > 0) {
			// always read 1024 bytes
			size = REMOTE_DISK_BLOCK_SIZE;
			if (offset + size > (uint64_t)fImageSize)
				size = fImageSize - offset;

			// seek to the offset
			off_t oldOffset = lseek(fImageFD, offset, SEEK_SET);
			if (oldOffset >= 0) {
				// read
				ssize_t bytesRead = read(fImageFD, reply->data, size);
				if (bytesRead >= 0) {
					result = bytesRead;
				} else {
					fprintf(stderr, "Error: Failed to read at position %llu: "
						"%s.", offset, strerror(errno));
					result = REMOTE_DISK_IO_ERROR;
				}
			} else {
				fprintf(stderr, "Error: Failed to seek to position %llu: %s.",
					offset, strerror(errno));
				result = REMOTE_DISK_IO_ERROR;
			}
		}

		// send reply
		reply->command = REMOTE_DISK_READ_REPLY;
		reply->offset = htonll(offset);
		reply->size = htons(result);
		_SendReply(reply, sizeof(*reply) + (result >= 0 ? result : 0));
	}

	void _HandleWriteRequest()
	{
		_OpenImage(false);

		remote_disk_header reply;
		uint64_t offset = ntohll(fRequest->offset);
		int16_t size = ntohs(fRequest->size);
		int16_t result = 0;

		printf("WRITE request: offset: %llu, %hd bytes\n", offset, size);

		if (size < 0
			|| (uint32_t)size > fRequestSize - sizeof(remote_disk_header)
			|| offset > (uint64_t)fImageSize) {
			result = REMOTE_DISK_BAD_REQUEST;
		} else if (offset < (uint64_t)fImageSize && size > 0) {
			if (offset + size > (uint64_t)fImageSize)
				size = fImageSize - offset;

			// seek to the offset
			off_t oldOffset = lseek(fImageFD, offset, SEEK_SET);
			if (oldOffset >= 0) {
				// write
				ssize_t bytesWritten = write(fImageFD, fRequest->data, size);
				if (bytesWritten >= 0) {
					result = bytesWritten;
				} else {
					fprintf(stderr, "Error: Failed to write at position %llu: "
						"%s.", offset, strerror(errno));
					result = REMOTE_DISK_IO_ERROR;
				}
			} else {
				fprintf(stderr, "Error: Failed to seek to position %llu: %s.",
					offset, strerror(errno));
				result = REMOTE_DISK_IO_ERROR;
			}
		}

		// send reply
		reply.command = REMOTE_DISK_WRITE_REPLY;
		reply.offset = htonll(offset);
		reply.size = htons(result);
		_SendReply(&reply, sizeof(reply));
	}

	void _SendReply(remote_disk_header *reply, int size)
	{
		reply->request_id = fRequest->request_id;
		reply->port = htons(REMOTE_DISK_SERVER_PORT);

		for (;;) {
			ssize_t bytesSent = sendto(fSocket, reply, size, 0,
				(const sockaddr*)&fClientAddress, sizeof(fClientAddress));

			if (bytesSent < 0) {
				if (errno == EINTR)
					continue;
				fprintf(stderr, "Error: Failed to send reply to client: %s.\n",
					strerror(errno));
			}
			break;
		}
	}

private:
	const char			*fImagePath;
	int					fImageFD;
	off_t				fImageSize;
	int					fSocket;
	remote_disk_header	*fRequest;
	ssize_t				fRequestSize;
	sockaddr_in			fClientAddress;
};


// main
int
main(int argc, const char *const *argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <image path>\n", argv[0]);
		exit(1);
	}
	const char *fileName = argv[1];

	Server server(fileName);
	return server.Run();
}
