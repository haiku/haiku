/*
 * Copyright 2008, Salvatore Benedetto, salvatore.benedetto@gmail.com.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <OS.h>

#include "TestUnitUtils.h"

#define KEY			((key_t)12345)

static status_t
remove_msg_queue(int msgID)
{
	return msgctl(msgID, IPC_RMID, 0);
}

struct message {
	long type;
	char text[20];
};


static void
test_msgget()
{
	TEST_SET("msgget({IPC_PRIVATE, key})");

	const char* currentTest = NULL;

	// Open private set with IPC_PRIVATE
	TEST("msgget(IPC_PRIVATE) - private");
	int msgID = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR);
	assert_posix_bool_success(msgID != -1);

	// Destroy private msg_queue
	TEST("msgctl(IPC_RMID) - private");
	status_t status = remove_msg_queue(msgID);
	assert_posix_bool_success(status != -1);

	// Open non-private non-existing set with IPC_CREAT
	TEST("msgget(KEY, IPC_CREAT) non-existing");
	msgID = msgget(KEY, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(status != -1);

	// Re-open non-private existing without IPC_CREAT
	TEST("msgget(KEY) re-open existing without IPC_CREAT");
	int returnID = msgget(KEY, 0);
	assert_equals(msgID, returnID);

	// Re-open non-private existing with IPC_CREAT
	TEST("msgget(IPC_CREATE) re-open existing with IPC_CREAT");
	returnID = msgget(KEY, IPC_CREAT | IPC_EXCL);
	assert_posix_bool_success(errno == EEXIST);

	// Destroy non-private msg_queue
	TEST("msgctl(IPC_RMID)");
	status = remove_msg_queue(msgID);
	assert_posix_bool_success(status != -1);

	// Open non-private non-existing without IPC_CREAT
	TEST("msgget(IPC_CREATE) non-existing without IPC_CREAT");
	msgID = msgget(KEY, IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(errno == ENOENT);

	// Destroy non-existing msg_queue
	TEST("msgctl()");
	status = remove_msg_queue(msgID);
	assert_posix_bool_success(errno == EINVAL);

	TEST("done");
}


static void
test_msgctl()
{
	TEST_SET("msgctl({IPC_STAT, IPC_SET, IPC_RMID})");

	const char* currentTest = NULL;

	// Open non-private non-existing set with IPC_CREAT
	TEST("msgget(IPC_CREATE) non-existing");
	int msgID = msgget(KEY, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(msgID != -1);

	// IPC_SET
	TEST("msgctl(IPC_SET)");
	struct msqid_ds msg_queue;
	memset(&msg_queue, 0, sizeof(struct msqid_ds));
	msg_queue.msg_perm.uid = getuid() + 3;
	msg_queue.msg_perm.gid = getgid() + 3;
	msg_queue.msg_perm.mode = 0666;
	msg_queue.msg_qbytes = 512;
	status_t status = msgctl(msgID, IPC_SET, &msg_queue);
	assert_posix_bool_success(status != 1);

	// IPC_STAT set
	TEST("msgctl(IPC_STAT)");
	memset(&msg_queue, 0, sizeof(struct msqid_ds));
	status = msgctl(msgID, IPC_STAT, &msg_queue);
	assert_posix_bool_success(status != 1);
	TEST("msgctl(IPC_STAT): number of bytes");
	assert_equals((msglen_t)msg_queue.msg_qbytes, (msglen_t)512);
	TEST("msgctl(IPC_STAT): uid");
	assert_equals(msg_queue.msg_perm.uid, getuid() + 3);
	TEST("msgctl(IPC_STAT): gid");
	assert_equals(msg_queue.msg_perm.gid, getgid() + 3);

	// Destroy non-private msg_queue
	TEST("msgctl(IPC_RMID)");
	status = remove_msg_queue(msgID);
	assert_posix_bool_success(status != 1);

	TEST("done");
}


static void
test_msgsnd()
{
	TEST_SET("msgsnd({EAGAIN, send})");

	const char* currentTest = NULL;

	// Open non-private non-existing set with IPC_CREAT
	TEST("msgget(IPC_CREATE) non-existing");
	int msgID = msgget(KEY, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(msgID != -1);

	// Send simple message
	TEST("msgsnd(simple message)");
	struct message msg;
	msg.type = 0;
	strcpy(msg.text, "Message to send\n");
	status_t status = msgsnd((key_t)msgID, (void *)&msg, 20, 0);
	assert_posix_bool_success(status != 1);

	// IPC_SET
	TEST("msgctl(IPC_SET) - set limit to 512");
	struct msqid_ds msg_queue;
	memset(&msg_queue, 0, sizeof(struct msqid_ds));
	msg_queue.msg_perm.uid = getuid() + 3;
	msg_queue.msg_perm.gid = getgid() + 3;
	msg_queue.msg_perm.mode = 0666;
	msg_queue.msg_qbytes = 512;
	status = msgctl(msgID, IPC_SET, &msg_queue);
	assert_posix_bool_success(status != 1);

	// Send big message IPC_NOWAIT
	TEST("msgsnd(IPC_NOWAIT)");
	msgsnd((key_t)msgID, (void *)&msg, 500, IPC_NOWAIT);
	assert_posix_bool_success(errno == EAGAIN);

	// Destroy non-private msg_queue
	TEST("msgctl(IPC_RMID)");
	status = remove_msg_queue(msgID);
	assert_posix_bool_success(status != 1);

	TEST("done");
}


static void
test_msgrcv()
{
	TEST_SET("msgrcv({IPC_STAT, IPC_SET, IPC_RMID})");

	const char* currentTest = NULL;

	// Open non-private non-existing set with IPC_CREAT
	TEST("msgget(IPC_CREATE) non-existing");
	int msgID = msgget(KEY, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(msgID != -1);

	// Receive simple message
	TEST("msgrcv(IPC_NOWAIT)");
	struct message msg;
	memset(&msg, 0, sizeof(struct message));
	msgrcv((key_t)msgID, (void *)&msg, 20, 0, IPC_NOWAIT);
	assert_posix_bool_success(errno == ENOMSG);

	pid_t child = fork();
	if (child == 0) {
		// Send a simple message
		TEST("msgsnd(simple message)");
		struct message smsg;
		msg.type = 0;
		strcpy(msg.text, "Message to send\n");
		status_t status = msgsnd((key_t)msgID, (void *)&smsg, 20, 0);
		assert_posix_bool_success(status != 1);
		exit(0);
	}

	wait_for_child(child);
	TEST("msgrcv(E2BIG)");
	msgrcv((key_t)msgID, (void *)&msg, 10, 0, IPC_NOWAIT);
	assert_posix_bool_success(errno == E2BIG);

	TEST("msgrcv(MSG_NOERROR)");
	status_t status
		= msgrcv((key_t)msgID, (void *)&msg, 10, 0, IPC_NOWAIT | MSG_NOERROR);
	assert_posix_bool_success(status != -1);

	// Destroy non-private msg_queue
	TEST("msgctl(IPC_RMID)");
	status = remove_msg_queue(msgID);
	assert_posix_bool_success(status != 1);

	TEST("done");
}


int
main()
{
	test_msgget();
	test_msgctl();
	test_msgsnd();
	test_msgrcv();

	printf("\nAll tests OK\n");
}
