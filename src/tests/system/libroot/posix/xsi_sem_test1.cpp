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
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>

#include <OS.h>

#include "TestUnitUtils.h"

#define KEY			((key_t)12345)
#define NUM_OF_SEMS	10

union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};


static status_t
remove_semaphore(int semID)
{
	return semctl(semID, 0, IPC_RMID, 0);
}


static void
test_semget()
{
	TEST_SET("semget({IPC_PRIVATE, key})");

	const char* currentTest = NULL;

	// Open private set with IPC_PRIVATE
	TEST("semget(IPC_PRIVATE) - private");
	int semID = semget(IPC_PRIVATE, NUM_OF_SEMS, S_IRUSR | S_IWUSR);
	assert_posix_bool_success(semID != -1);

	// Destroy private semaphore
	TEST("semctl(IPC_RMID) - private");
	status_t status = remove_semaphore(semID);
	assert_posix_bool_success(status != -1);

	// Open non-private non-existing set with IPC_CREAT
	TEST("semget(KEY, IPC_CREAT) non-existing");
	semID = semget(KEY, NUM_OF_SEMS, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(status != -1);

	// Re-open non-private existing without IPC_CREAT
	TEST("semget(KEY) re-open existing without IPC_CREAT");
	int returnID = semget(KEY, 0, 0);
	assert_equals(semID, returnID);

	// Re-open non-private existing with IPC_CREAT
	TEST("semget(IPC_CREATE) re-open existing with IPC_CREAT");
	returnID = semget(KEY, 0, IPC_CREAT | IPC_EXCL);
	assert_posix_bool_success(errno == EEXIST);

	// Destroy non-private semaphore
	TEST("semctl(IPC_RMID)");
	status = remove_semaphore(semID);
	assert_posix_bool_success(status != -1);

	// Open non-private non-existing without IPC_CREAT
	TEST("semget(IPC_CREATE) non-existing without IPC_CREAT");
	semID = semget(KEY, NUM_OF_SEMS, IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(errno == ENOENT);

	// Destroy non-existing semaphore
	TEST("semctl()");
	status = remove_semaphore(semID);
	assert_posix_bool_success(errno == EINVAL);

	TEST("done");
}


static void
test_semop2()
{
	TEST_SET("semop2()");

	const char* currentTest = NULL;

	// Re-open non-private existing without IPC_CREAT
	TEST("semget(KEY) re-open existing without IPC_CREAT");
	int returnedID = semget(KEY, 0, 0);
	assert_posix_bool_success(returnedID != -1);

	TEST("semop(IPC_NOWAIT) - wait for zero");
	// Set up array of semaphores
	struct sembuf array[NUM_OF_SEMS];
	for (int i = 0; i < NUM_OF_SEMS; i++) {
		array[i].sem_num = i;
		array[i].sem_op = 0;
		array[i].sem_flg = IPC_NOWAIT;
	}
	semop(returnedID, array, NUM_OF_SEMS);
	assert_posix_bool_success(errno == EAGAIN);

	TEST("semop(IPC_NOWAIT) - wait to increase");
	for (int i = 0; i < NUM_OF_SEMS; i++) {
		array[i].sem_num = i;
		array[i].sem_op = -9;
		array[i].sem_flg = IPC_NOWAIT;
	}
	semop(returnedID, array, NUM_OF_SEMS);
	assert_posix_bool_success(errno == EAGAIN);

	TEST("semop(IPC_NOWAIT) - acquire resource sem #0");
	struct sembuf ops;
	ops.sem_num = 0;
	ops.sem_op = -8;
	ops.sem_flg = 0;
	status_t status = semop(returnedID, &ops, 1);
	assert_posix_bool_success(status != -1);

	TEST("semop(IPC_NOWAIT) - acquire zero sem #0");
	ops.sem_num = 0;
	ops.sem_op = 0;
	ops.sem_flg = 0;
	status = semop(returnedID, &ops, 1);

	TEST("semop(IPC_NOWAIT) - revert semop sem #0");
	ops.sem_num = 0;
	ops.sem_op = 8;
	ops.sem_flg = 0;
	status = semop(returnedID, &ops, 1);

	// Decrease to zero even semaphores and 
	// use SEM_UNDO flag on odd semaphores in order
	// to wake up the father on exit
	// Set up array of semaphores
	for (int i = 0; i < NUM_OF_SEMS; i++) {
		array[i].sem_num = i;
		array[i].sem_op = -8;
		if (i % 2)
			array[i].sem_flg = 0;
		else
			array[i].sem_flg = SEM_UNDO;
	}
	TEST("semop() - father");
	status = semop(returnedID, array, NUM_OF_SEMS);
	assert_posix_bool_success(status != -1);

	TEST("done");
}


static void
test_semop()
{	
	TEST_SET("semop()");
	const char* currentTest = NULL;

	// Open non-private non-existing set with IPC_CREAT
	TEST("semget(IPC_CREATE) non-existing");
	int semID = semget(KEY, NUM_OF_SEMS, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(semID != -1);

	// SETALL
	TEST("semctl(SETALL)");
	union semun args;
	args.array = (unsigned short *)malloc(sizeof(unsigned short) * NUM_OF_SEMS);
	for (int i = 0; i < NUM_OF_SEMS; i++)
		args.array[i] = 8;
	status_t status = semctl(semID, 0, SETALL, args);
	assert_posix_bool_success(status != -1);
	free(args.array);

	pid_t child = fork();
	if (child == 0) {
		// The child first will test the IPC_NOWAIT
		// feature, while the father waits for him,
		// by waiting for zero on even semaphores, 
		// and to increase for odd semaphores, which
		// will happen on process exit due to SEM_UNDO
		// feature.
		test_semop2();
		exit(0);
	}

	wait_for_child(child);

	// Set up array of semaphores
	struct sembuf array[NUM_OF_SEMS];
	for (int i = 0; i < NUM_OF_SEMS; i++) {
		array[i].sem_num = i;
		if (i % 2)
			array[i].sem_op = 0; // wait for zero
		else 
			array[i].sem_op = -8; // wait to increase
		array[i].sem_flg = 0;
	}
	TEST("semop() - father acquired set");
	status = semop(semID, array, NUM_OF_SEMS);
	assert_posix_bool_success(status != -1);

	// Destroy non-private semaphore
	TEST("semctl(IPC_RMID)");
	status = remove_semaphore(semID);
	assert_posix_bool_success(status != 1);

	TEST("done");
}


static void
test_semctl()
{
	TEST_SET("semctl({GETVAL, SETVAL, GETPID, GETNCNT, GETZCNT, GETALL, SETALL, IPC_STAT, IPC_SET, IPC_RMID})");

	const char* currentTest = NULL;

	// Open non-private non-existing set with IPC_CREAT
	TEST("semget(IPC_CREATE) non-existing");
	int semID = semget(KEY, NUM_OF_SEMS, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR
		| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	assert_posix_bool_success(semID != -1);

	// GETVAL
	TEST("semctl(GETVAL)");
	union semun args;
	status_t status = semctl(semID, NUM_OF_SEMS - 1, GETVAL, args);
	// Semaphore is not initialized. Value is unknown.
	// We care about not crashing into KDL.
	assert_posix_bool_success(status != -1);

	// SETALL
	TEST("semctl(SETALL)");
	args.array = (unsigned short *)malloc(sizeof(unsigned short) * NUM_OF_SEMS);
	for (int i = 0; i < NUM_OF_SEMS; i++)
		args.array[i] = 5;
	status = semctl(semID, 0, SETALL, args);
	assert_posix_bool_success(status != -1);
	free(args.array);

	// GETVAL semaphore 4
	int returnedValue = semctl(semID, 4, GETVAL, 0);
	assert_equals((unsigned short)returnedValue, (unsigned short)5);

	// GETALL
	TEST("semctl(GETALL)");
	args.array = (unsigned short *)malloc(sizeof(unsigned short) * NUM_OF_SEMS);
	semctl(semID, 0, GETALL, args);
	// Check only last semaphore value
	assert_equals(args.array[NUM_OF_SEMS - 1], (unsigned short)5);
	free(args.array);

	// SETVAL semaphore 2
	TEST("semctl(SETVAL) - semaphore #2");
	args.val = 7;
	status = semctl(semID, 2, SETVAL, args);
	assert_posix_bool_success(status != 1);

	// GETALL
	TEST("semctl(GETALL)");
	args.array = (unsigned short *)malloc(sizeof(unsigned short) * NUM_OF_SEMS);
	status = semctl(semID, 0, GETALL, args);
	assert_posix_bool_success(status != -1);
	TEST("semctl(GETALL) - semaphore #10");
	assert_equals(args.array[NUM_OF_SEMS - 1], (unsigned short)5);
	TEST("semctl(GETALL) - semaphore #2");
	assert_equals(args.array[NUM_OF_SEMS - 1], (unsigned short)5);
	free(args.array);

	// IPC_SET
	TEST("semctl(IPC_SET)");
	struct semid_ds semaphore;
	memset(&semaphore, 0, sizeof(struct semid_ds));
	semaphore.sem_perm.uid = getuid() + 3;
	semaphore.sem_perm.gid = getgid() + 3;
	semaphore.sem_perm.mode = 0666;
	args.buf = &semaphore;
	status = semctl(semID, 0, IPC_SET, args);
	assert_posix_bool_success(status != 1);

	// IPC_STAT set
	TEST("semctl(IPC_STAT)");
	memset(&semaphore, 0, sizeof(struct semid_ds));
	args.buf = &semaphore;
	status = semctl(semID, 0, IPC_STAT, args);
	assert_posix_bool_success(status != 1);
	TEST("semctl(IPC_STAT): number of sems");
	assert_equals((unsigned short)args.buf->sem_nsems, (unsigned short)NUM_OF_SEMS);
	TEST("semctl(IPC_STAT): uid");
	assert_equals(args.buf->sem_perm.uid, getuid() + 3);
	TEST("semctl(IPC_STAT): gid");
	assert_equals(args.buf->sem_perm.gid, getgid() + 3);
	
	// Destroy non-private semaphore
	TEST("semctl(IPC_RMID)");
	status = remove_semaphore(semID);
	assert_posix_bool_success(status != 1);

	TEST("done");
}


int
main()
{
	test_semget();
	test_semctl();
	test_semop();

	printf("\nAll tests OK\n");
}
