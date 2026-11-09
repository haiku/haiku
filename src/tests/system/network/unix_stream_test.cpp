/*
 * Copyright 2026, Jerome Duval, jerome.duval0@gmail.com.
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
send_fd_test(bool withControlBuffer, bool fullTable = false)
{
	int sock[2];
	int status = socketpair(AF_UNIX, SOCK_STREAM, 0, sock);
	if (status == -1) {
		REPORT_ERROR("socket() failed: %s\n", strerror(errno));
		return 1;
	}

	int fds[8];
	for (int i = 0; i < 8; i++) {
		fds[i] = shm_open("test_shm", O_CREAT | O_RDWR, 0666);
		if (fds[i] == -1) {
			REPORT_ERROR("shm_open() failed: %s\n", strerror(errno));
			return 1;
		}
		shm_unlink("test_shm");
	}

	// Send FDs
	char iobuf[] = "test";
	struct iovec iov {
		.iov_base = iobuf,
		.iov_len = sizeof(iobuf),
	};

	struct msghdr msg;
	memset(&msg, 0, sizeof(msg));

	struct cmsghdr *cmsg;
	char buf[CMSG_SPACE(sizeof(fds))];
	memset(buf, 0, sizeof(buf));
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fds));
	memcpy(CMSG_DATA(cmsg), &fds, sizeof(fds));
	msg.msg_controllen = cmsg->cmsg_len;

	status = sendmsg(sock[0], &msg, 0);
	if (status == -1) {
		REPORT_ERROR("sendmsg() failed: %s\n", strerror(errno));
		return 1;
	}

	if (fullTable) {
		int fd = -1, last;
		do {
			last = fd;
			fd = dup(0);
		} while (fd != -1);
		if (errno != EMFILE) {
			REPORT_ERROR("dup() failed: %s\n", strerror(errno));
			return 1;
		}
		close(last);
	}

	// Receive FD
	char buf2[CMSG_SPACE(sizeof(int))];
	memset(buf2, 0, sizeof(buf2));
	msg.msg_control = withControlBuffer ? buf2 : NULL;
	msg.msg_controllen = sizeof(buf2);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	status = recvmsg(sock[1], &msg, 0);
	if (status == -1) {
		REPORT_ERROR("recvmsg() failed: %s\n", strerror(errno));
		return 1;
	}

	if ((msg.msg_flags & MSG_CTRUNC) == 0) {
		REPORT_ERROR("recvmsg() failed: no MSG_CTRUNC flag\n");
		return 1;
	}
	cmsg = CMSG_FIRSTHDR(&msg);
	if (cmsg == NULL) {
		if (withControlBuffer && !fullTable) {
			REPORT_ERROR("recvmsg() failed: no control message\n");
			return 1;
		} else
			return 0;
	}
	if (cmsg->cmsg_level != SOL_SOCKET) {
		REPORT_ERROR("recvmsg() failed: unexpected level %d\n", cmsg->cmsg_level);
		return 1;
	}
	if (cmsg->cmsg_type != SCM_RIGHTS) {
		REPORT_ERROR("recvmsg() failed: unexpected type %d\n", cmsg->cmsg_type);
		return 1;
	}
	if (fullTable) {
		if (cmsg->cmsg_len != CMSG_LEN(sizeof(int))) {
			REPORT_ERROR("recvmsg() failed: unexpected length %ld (expected %ld)\n",
				cmsg->cmsg_len, CMSG_LEN(sizeof(int)));
			return 1;
		}
	} else {
		if (cmsg->cmsg_len != CMSG_LEN(2 * sizeof(int))) {
			REPORT_ERROR("recvmsg() failed: unexpected length %ld (expected %ld)\n",
				cmsg->cmsg_len, CMSG_LEN(2 * sizeof(int)));
			return 1;
		}
	}
	int fd1;
	memcpy(&fd1, CMSG_DATA(cmsg), sizeof(fd1));
	if (fd1 == -1) {
		REPORT_ERROR("recvmsg() failed: unexpected fd %d\n", fd1);
		return 1;
	}

	// Check that the FD refers to the same file
	struct stat statbuf;
	status = fstat(fds[0], &statbuf);
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

	close(sock[0]);
	close(sock[1]);
	for (int i = 0; i < 8; i++)
		close(fds[i]);
	close(fd1);

	unlink("test.sock");
	unlink("test1.sock");

	return 0;
}


int
main()
{
	if (send_fd_test(true, false) != 0)
		return 1;

	if (send_fd_test(false, false) != 0)
		return 1;

	if (send_fd_test(true, true) != 0)
		return 1;

	return 0;
}
