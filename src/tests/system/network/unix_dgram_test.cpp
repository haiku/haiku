/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>


#define REPORT_ERROR(msg, ...) \
	fprintf(stderr, "%s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)


int
connect_test()
{
	unlink("test.sock");
	unlink("test1.sock");
	unlink("test2.sock");

	int status;

	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, "test.sock");
	status = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	int sock1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock1 == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr1;
	addr1.sun_family = AF_UNIX;
	strcpy(addr1.sun_path, "test1.sock");
	status = bind(sock1, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	// Set non-blocking on both sockets
	int flags1 = fcntl(sock, F_GETFL, 0);
	if (flags1 == -1) {
		REPORT_ERROR("fcntl() failed: %s\n", strerror(errno));
		return 1;
	}
	status = fcntl(sock, F_SETFL, flags1 | O_NONBLOCK);
	if (status == -1) {
		REPORT_ERROR("fcntl() failed: %s\n", strerror(errno));
		return 1;
	}
	status = fcntl(sock1, F_SETFL, flags1 | O_NONBLOCK);
	if (status == -1) {
		REPORT_ERROR("fcntl() failed: %s\n", strerror(errno));
		return 1;
	}

	status = connect(sock, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("connect() failed: %s\n", strerror(errno));
		return 1;
	}

	// Connect in the opposite way
	status = connect(sock1, (struct sockaddr*)&addr, sizeof(addr));
	if (status == -1) {
		REPORT_ERROR("connect() failed: %s\n", strerror(errno));
		return 1;
	}

	// Reconnect a connected DGRAM socket
	status = connect(sock, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("connect() failed: %s\n", strerror(errno));
		return 1;
	}

	int sock2 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock2 == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr2;
	addr2.sun_family = AF_UNIX;
	strcpy(addr2.sun_path, "test2.sock");
	status = bind(sock2, (struct sockaddr*)&addr2, sizeof(addr2));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	// Connect to a socket that are already connected
	status = connect(sock2, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status != -1) {
		REPORT_ERROR("connect() succeeded unexpectedly\n");
		return 1;
	}
	if (errno != EPERM) {
		REPORT_ERROR("connect() failed with unexpected error: %s\n", strerror(errno));
		return 1;
	}

	status = close(sock2);
	if (status == -1) {
		REPORT_ERROR("close() failed: %s\n", strerror(errno));
		return 1;
	}

	// Connect to a closed socket
	status = connect(sock, (struct sockaddr*)&addr2, sizeof(addr2));
	if (status != -1) {
		REPORT_ERROR("connect() succeeded unexpectedly\n");
		return 1;
	}
	if (errno != ECONNREFUSED) {
		REPORT_ERROR("connect() failed with unexpected error: %s\n", strerror(errno));
		return 1;
	}

	close(sock);
	close(sock1);

	unlink("test.sock");
	unlink("test1.sock");
	unlink("test2.sock");

	return 0;
}


int
send_test()
{
	unlink("test.sock");
	unlink("test1.sock");
	unlink("test2.sock");

	int status;

	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, "test.sock");
	status = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	status = send(sock, "test", 4, 0);
	if (status != -1) {
		REPORT_ERROR("send() succeeded unexpectedly\n");
		return 1;
	}
	// if (errno != ENOTCONN) {
	// 	REPORT_ERROR("send() failed with unexpected error: %s\n", strerror(errno));
	// 	return 1;
	// }

	int sock1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock1 == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr1;
	addr1.sun_family = AF_UNIX;
	strcpy(addr1.sun_path, "test1.sock");
	status = bind(sock1, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	// Set non-blocking on both sockets
	status = fcntl(sock, F_SETFL, O_NONBLOCK);
	if (status == -1) {
		REPORT_ERROR("fcntl() failed: %s\n", strerror(errno));
		return 1;
	}
	status = fcntl(sock1, F_SETFL, O_NONBLOCK);
	if (status == -1) {
		REPORT_ERROR("fcntl() failed: %s\n", strerror(errno));
		return 1;
	}

	status = sendto(sock, "test1", 5, 0, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("sendto() failed: %s\n", strerror(errno));
		return 1;
	}

	status = connect(sock, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("connect() failed: %s\n", strerror(errno));
		return 1;
	}
	status = connect(sock1, (struct sockaddr*)&addr, sizeof(addr));
	if (status == -1) {
		REPORT_ERROR("connect() failed: %s\n", strerror(errno));
		return 1;
	}

	status = send(sock, "test2", 5, 0);
	if (status == -1) {
		REPORT_ERROR("send() failed: %s\n", strerror(errno));
		return 1;
	}

	int sock2 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock2 == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr2;
	addr2.sun_family = AF_UNIX;
	strcpy(addr2.sun_path, "test2.sock");
	status = bind(sock2, (struct sockaddr*)&addr2, sizeof(addr2));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	status = sendto(sock2, "test3", 5, 0, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status != -1) {
		REPORT_ERROR("sendto() succeeded unexpectedly\n");
		return 1;
	}
	if (errno != EPERM) {
		REPORT_ERROR("sendto() failed with unexpected error: %s\n", strerror(errno));
		return 1;
	}

	char buf[16];
	memset(buf, 0, sizeof(buf));
	status = recv(sock1, buf, sizeof(buf), 0);
	if (status == -1) {
		REPORT_ERROR("recv() failed: %s\n", strerror(errno));
		return 1;
	}
	if (strcmp(buf, "test1") != 0) {
		REPORT_ERROR("recv() received unexpected data: %s\n", buf);
		return 1;
	}

	memset(buf, 0, sizeof(buf));
	struct sockaddr_un addr3;
	memset(&addr3, 0, sizeof(addr3));
	socklen_t addrlen = sizeof(addr3);
	status = recvfrom(sock1, buf, sizeof(buf), 0, (struct sockaddr*)&addr3, &addrlen);
	if (status == -1) {
		REPORT_ERROR("recv() failed: %s\n", strerror(errno));
		return 1;
	}
	if (strcmp(buf, "test2") != 0) {
		REPORT_ERROR("recv() received unexpected data: %s\n", buf);
		return 1;
	}
	if (strcmp(addr.sun_path, addr3.sun_path) != 0) {
		REPORT_ERROR("recv() received unexpected address: %s\n", addr3.sun_path);
		return 1;
	}

	status = send(sock, "test4", 4, 0);
	if (status == -1) {
		REPORT_ERROR("send() failed: %s\n", strerror(errno));
		return 1;
	}

	status = send(sock, "test5", 5, 0);
	if (status == -1) {
		REPORT_ERROR("send() failed: %s\n", strerror(errno));
		return 1;
	}

	memset(buf, 0, sizeof(buf));
	status = recv(sock1, buf, 4, 0);
	if (status == -1) {
		REPORT_ERROR("recv() failed: %s\n", strerror(errno));
		return 1;
	}
	if (strcmp(buf, "test") != 0) {
		REPORT_ERROR("recv() received unexpected data: %s\n", buf);
		return 1;
	}

	// The last byte of the previous datagram should be discarded.
	memset(buf, 0, sizeof(buf));
	status = recv(sock1, buf, sizeof(buf), 0);
	if (status == -1) {
		REPORT_ERROR("recv() failed: %s\n", strerror(errno));
		return 1;
	}
	if (strcmp(buf, "test5") != 0) {
		REPORT_ERROR("recv() received unexpected data: %s\n", buf);
		return 1;
	}

	close(sock1);
	status = send(sock, "test6", 5, 0);
	if (status != -1) {
		REPORT_ERROR("send() succeeded unexpectedly\n");
		return 1;
	}
	if (errno != ECONNREFUSED) {
		REPORT_ERROR("send() failed with unexpected error: %s\n", strerror(errno));
		return 1;
	}

	close(sock);
	close(sock2);

	unlink("test.sock");
	unlink("test1.sock");
	unlink("test2.sock");

	return 0;
}


int
shutdown_test()
{
	unlink("test.sock");
	unlink("test1.sock");

	int status;

	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, "test.sock");
	status = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	int sock1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock1 == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr1;
	addr1.sun_family = AF_UNIX;
	strcpy(addr1.sun_path, "test1.sock");
	status = bind(sock1, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	status = shutdown(sock, SHUT_WR);
	if (status == -1) {
		REPORT_ERROR("shutdown() failed: %s\n", strerror(errno));
		return 1;
	}

	status = sendto(sock, "test", 4, 0, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status != -1) {
		REPORT_ERROR("send() succeeded unexpectedly\n");
		return 1;
	}
	if (errno != EPIPE) {
		REPORT_ERROR("send() failed with unexpected error: %s\n", strerror(errno));
		return 1;
	}

	status = sendto(sock1, "test", 4, 0, (struct sockaddr*)&addr, sizeof(addr));
	if (status == -1) {
		REPORT_ERROR("send() failed: %s\n", strerror(errno));
		return 1;
	}

	status = shutdown(sock, SHUT_RD);
	if (status == -1) {
		REPORT_ERROR("shutdown() failed: %s\n", strerror(errno));
		return 1;
	}

	status = sendto(sock1, "test", 4, 0, (struct sockaddr*)&addr, sizeof(addr));
	if (status != -1) {
		REPORT_ERROR("send() succeeded unexpectedly\n");
		return 1;
	}
	if (errno != EPIPE) {
		REPORT_ERROR("send() failed with unexpected error: %s\n", strerror(errno));
		return 1;
	}

	char buf[16];
	memset(buf, 0, sizeof(buf));
	status = recv(sock, buf, sizeof(buf), 0);
	if (status == -1) {
		REPORT_ERROR("recv() failed: %s\n", strerror(errno));
		return 1;
	}
	if (status != 0) {
		REPORT_ERROR("recv() received unexpected data\n");
		return 1;
	}

	close(sock);
	close(sock1);

	unlink("test.sock");
	unlink("test1.sock");

	return 0;
}


int
send_fd_test()
{
	unlink("test.sock");
	unlink("test1.sock");

	int status;

	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr;
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, "test.sock");
	status = bind(sock, (struct sockaddr*)&addr, sizeof(addr));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	int sock1 = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock1 == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	struct sockaddr_un addr1;
	addr1.sun_family = AF_UNIX;
	strcpy(addr1.sun_path, "test1.sock");
	status = bind(sock1, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("bind() failed: %s\n", strerror(errno));
		return 1;
	}

	status = connect(sock, (struct sockaddr*)&addr1, sizeof(addr1));
	if (status == -1) {
		REPORT_ERROR("connect() failed: %s\n", strerror(errno));
		return 1;
	}

	int fd = shm_open("test_shm", O_CREAT | O_RDWR, 0666);
	if (fd == -1) {
		REPORT_ERROR("shm_open() failed: %s\n", strerror(errno));
		return 1;
	}
	shm_unlink("test_shm");

	// Send FD
	char iobuf[] = "test";
	struct iovec iov {
		.iov_base = iobuf,
		.iov_len = sizeof(iobuf),
	};

	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));

	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(fd))];
	memset(buf, 0, sizeof(buf));
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));
	msg.msg_controllen = cmsg->cmsg_len;

	status = sendmsg(sock, &msg, 0);
	if (status == -1) {
		REPORT_ERROR("sendmsg() failed: %s\n", strerror(errno));
		return 1;
	}

	// Receive FD
	memset(buf, 0, sizeof(buf));
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	status = recvmsg(sock1, &msg, 0);
	if (status == -1) {
		REPORT_ERROR("recvmsg() failed: %s\n", strerror(errno));
		return 1;
	}

	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL) {
		REPORT_ERROR("recvmsg() failed: no control message\n");
		return 1;
	}
	if (cmsg->cmsg_level != SOL_SOCKET) {
		REPORT_ERROR("recvmsg() failed: unexpected level %d\n", cmsg->cmsg_level);
		return 1;
	}
	if (cmsg->cmsg_type != SCM_RIGHTS) {
		REPORT_ERROR("recvmsg() failed: unexpected type %d\n", cmsg->cmsg_type);
		return 1;
	}
	if (cmsg->cmsg_len != CMSG_LEN(sizeof(fd))) {
		REPORT_ERROR("recvmsg() failed: unexpected length %ld\n", cmsg->cmsg_len);
		return 1;
	}

	int fd1;
	memcpy(&fd1, CMSG_DATA(cmsg), sizeof(fd1));
	if (fd1 == -1) {
		REPORT_ERROR("recvmsg() failed: unexpected fd %d\n", fd1);
		return 1;
	}

	// Check that the FD refers to the same file
	struct stat statbuf;
	status = fstat(fd, &statbuf);
	if (status == -1) {
		REPORT_ERROR("fstat() failed: %s\n", strerror(errno));
		return 1;
	}

	struct stat statbuf1;
	status = fstat(fd1, &statbuf1);
	if (status == -1) {
		REPORT_ERROR("fstat() failed: %s\n", strerror(errno));
		return 1;
	}

	if (statbuf.st_dev != statbuf1.st_dev) {
		REPORT_ERROR("recvmsg() failed: unexpected device %ld\n", (long)statbuf1.st_dev);
		return 1;
	}
	if (statbuf.st_ino != statbuf1.st_ino) {
		REPORT_ERROR("recvmsg() failed: unexpected inode %ld\n", (long)statbuf1.st_ino);
		return 1;
	}

	close(sock);
	close(sock1);
	close(fd);
	close(fd1);

	unlink("test.sock");
	unlink("test1.sock");

	return 0;
}


int
main()
{
	if (connect_test() != 0)
		return 1;

	if (send_test() != 0)
		return 1;

	if (shutdown_test() != 0)
		return 1;

	if (send_fd_test() != 0)
		return 1;

	return 0;
}
