/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>


#ifdef __HAIKU__
#	include <OS.h>
#else

typedef int64_t bigtime_t;

static bigtime_t
system_time()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return (bigtime_t)tv.tv_sec * 1000000 + tv.tv_usec;
}


#endif	// !__HAIKU__


static timespec*
absolute_timeout(timespec& timeout, bigtime_t relativeTimeout)
{
	timeval tv;
	gettimeofday(&tv, NULL);
	timeout.tv_sec = tv.tv_sec + relativeTimeout / 1000000;
	timeout.tv_nsec = (tv.tv_usec + relativeTimeout % 1000000) * 1000;
	if (timeout.tv_nsec > 1000000000) {
		timeout.tv_sec++;
		timeout.tv_nsec -= 1000000000;
	}

	return &timeout;
}


template<typename Type>
static void
_assert_equals(const char* test, const Type& expected, const Type& actual,
	int lineNumber)
{
	if (actual == expected)
		return;

	fprintf(stderr, "%s FAILED in line %d\n", test, lineNumber);
	exit(1);
}


template<typename Type>
static void
_assert_equals_not(const char* test, const Type& unexpected, const Type& actual,
	int lineNumber)
{
	if (actual != unexpected)
		return;

	fprintf(stderr, "%s FAILED in line %d\n", test, lineNumber);
	exit(1);
}


static void
_assert_time_equals(const char* test, bigtime_t expected,
	bigtime_t actual, int lineNumber)
{
	// allow 2% deviation
	bigtime_t diff = actual > expected ? actual - expected : expected - actual;
	if (diff <= expected / 50)
		return;

	fprintf(stderr, "%s FAILED in line %d: expected time: %lld, actual: %lld\n",
		test, lineNumber, (long long)expected, (long long)actual);
	exit(1);
}


static void
_assert_posix_bool_success(const char* test, bool success, int lineNumber)
{
	if (success)
		return;

	fprintf(stderr, "%s FAILED in line %d: %s\n", test, lineNumber,
		strerror(errno));
	exit(1);
}


static void
_assert_posix_bool_error(const char* test, int expectedError, bool success,
	int lineNumber)
{
	if (success) {
		fprintf(stderr, "%s FAILED in line %d: call succeeded unexpectedly\n",
			test, lineNumber);
		exit(1);
	}

	if (errno != expectedError) {
		fprintf(stderr, "%s FAILED in line %d: call set unexpected error "
			"code \"%s\" (0x%x), expected: \"%s\" (0x%x)\n", test, lineNumber,
			strerror(errno), errno, strerror(expectedError), expectedError);
		exit(1);
	}
}


static void
test_set(const char* testSet)
{
	printf("\nTEST SET: %s\n", testSet);
}


static void
test_ok(const char* test)
{
	if (test != NULL)
		printf("%s OK\n", test);
}


static void
_wait_for_child(const char* test, pid_t child, int lineNumber)
{
	int status;
	pid_t result = wait(&status);
	_assert_posix_bool_success(test, result >= 0, lineNumber);
	_assert_equals(test, child, result, lineNumber);
	_assert_equals(test, 0, status, lineNumber);
}


#if 0
static void
dump_sem(const char* name, sem_t* sem)
{
	printf("%s, %p: ", name, sem);
	for (size_t i = 0; i < sizeof(sem_t); i++)
		printf("%02x", ((char*)sem)[i]);
	printf("\n");
}
#endif


#define TEST_SET(testSet)	test_set(testSet)
#define TEST(test)	test_ok(currentTest); currentTest = (test)

#define assert_equals(expected, actual) \
	_assert_equals(currentTest, (expected), (actual), __LINE__)

#define assert_equals_not(expected, actual) \
	_assert_equals_not(currentTest, (expected), (actual), __LINE__)

#define assert_time_equals(expected, actual) \
	_assert_time_equals(currentTest, (expected), (actual), __LINE__)

#define assert_posix_bool_success(success) \
	_assert_posix_bool_success(currentTest, (success), __LINE__)

#define assert_posix_success(result) \
	_assert_posix_bool_success(currentTest, (result) == 0, __LINE__)

#define assert_posix_bool_error(expectedError, success) \
	_assert_posix_bool_error(currentTest, (expectedError), (success), __LINE__)

#define assert_posix_error(expectedError, result) \
	_assert_posix_bool_error(currentTest, (expectedError), (result) == 0, \
		__LINE__)

#define wait_for_child(child) \
	_wait_for_child(currentTest, (child), __LINE__)


static const char* const kSemName1 = "/test_sem1";


static void
test_open_close_unlink()
{
	TEST_SET("sem_{open,close,unlink}()");

	const char* currentTest = NULL;

	// open non-existing with O_CREAT
	TEST("sem_open(O_CREAT) non-existing");
	sem_t* sem = sem_open(kSemName1, O_CREAT, S_IRUSR | S_IWUSR, 1);
	assert_posix_bool_success(sem != SEM_FAILED);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	// open existing with O_CREAT
	TEST("sem_open(O_CREAT) existing");
	sem = sem_open(kSemName1, O_CREAT, S_IRUSR | S_IWUSR, 1);
	assert_posix_bool_success(sem != SEM_FAILED);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	// open existing without O_CREAT
	TEST("sem_open() existing");
	sem = sem_open(kSemName1, 0);
	assert_posix_bool_success(sem != SEM_FAILED);

	// re-open existing without O_CREAT
	TEST("sem_open() existing");
	sem_t* sem2 = sem_open(kSemName1, 0);
	assert_posix_bool_success(sem2 != SEM_FAILED);
	assert_equals(sem, sem2);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	// open existing with O_CREAT | O_EXCL
	TEST("sem_open(O_CREAT | O_EXCL) existing");
	sem = sem_open(kSemName1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1);
	assert_posix_bool_error(EEXIST, sem != SEM_FAILED);

	// open existing without O_CREAT
	TEST("sem_open() existing");
	sem = sem_open(kSemName1, 0);
	assert_posix_bool_success(sem != SEM_FAILED);

	// unlink
	TEST("unlink() existing");
	assert_posix_success(sem_unlink(kSemName1));

	// open non-existing with O_CREAT | O_EXCL
	TEST("sem_open(O_CREAT | O_EXCL) non-existing");
	sem2 = sem_open(kSemName1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 2);
	assert_posix_bool_success(sem2 != SEM_FAILED);
	assert_equals_not(sem, sem2);

	// unlink
	TEST("unlink() existing");
	assert_posix_success(sem_unlink(kSemName1));

	// unlink
	TEST("unlink() non-existing");
	assert_posix_error(ENOENT, sem_unlink(kSemName1));

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem2));

	TEST("done");
}


static void
test_init_destroy()
{
	TEST_SET("sem_{init,destroy}()");

	const char* currentTest = NULL;

	// init
	TEST("sem_init()");
	sem_t sem;
	assert_posix_success(sem_init(&sem, 0, 1));

	// destroy
	TEST("sem_destroy()");
	assert_posix_success(sem_destroy(&sem));

	// init
	TEST("sem_init()");
	assert_posix_success(sem_init(&sem, 0, 1));

	// init
	TEST("sem_init()");
	sem_t sem2;
	assert_posix_success(sem_init(&sem2, 0, 2));

	// destroy
	TEST("sem_destroy()");
	assert_posix_success(sem_destroy(&sem));

	// destroy
	TEST("sem_destroy()");
	assert_posix_success(sem_destroy(&sem2));

	TEST("done");
}


static void
test_open_close_fork()
{
	TEST_SET("sem_{open,close}() with fork()");

	const char* currentTest = NULL;

	// open non-existing with O_CREAT
	TEST("sem_open(O_CREAT) non-existing");
	sem_t* sem = sem_open(kSemName1, O_CREAT, S_IRUSR | S_IWUSR, 1);
	assert_posix_bool_success(sem != SEM_FAILED);

	TEST("close_sem() forked");
	pid_t child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		assert_posix_success(sem_close(sem));
		exit(0);
	} else {
		// parent
		assert_posix_success(sem_close(sem));
		wait_for_child(child);
	}

	TEST("sem_open() existing forked");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		sem = sem_open(kSemName1, O_CREAT);
		assert_posix_bool_success(sem != SEM_FAILED);
		exit(0);
	} else {
		// parent
		sem = sem_open(kSemName1, O_CREAT);
		wait_for_child(child);
		assert_posix_success(sem_close(sem));
	}

	TEST("done");
}


static void
test_init_destroy_fork()
{
	TEST_SET("sem_{init,destroy}() with fork()");

	const char* currentTest = NULL;

	// init
	TEST("sem_init()");
	sem_t sem;
	assert_posix_success(sem_init(&sem, 0, 1));

	// destroy
	TEST("sem_destroy() forked");
	pid_t child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		assert_posix_success(sem_destroy(&sem));
		exit(0);
	} else {
		// parent
		assert_posix_success(sem_destroy(&sem));
		wait_for_child(child);
	}

	TEST("done");
}


static void
test_post_wait_named()
{
	TEST_SET("sem_{post,wait,trywait,timedwait}() named semaphore");

	const char* currentTest = NULL;

	// make sure the sem doesn't exist yet
	sem_unlink(kSemName1);

	// open non-existing with O_CREAT
	TEST("sem_open(O_CREAT) non-existing");
	sem_t* sem = sem_open(kSemName1, O_CREAT, S_IRUSR | S_IWUSR, 1);
	assert_posix_bool_success(sem != SEM_FAILED);

	TEST("sem_getvalue()");
	int value;
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(2, value);

	// wait
	TEST("sem_wait() non-blocking");
	assert_posix_success(sem_wait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// wait
	TEST("sem_wait() non-blocking");
	assert_posix_success(sem_wait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	// re-open existing
	TEST("sem_open() existing");
	sem = sem_open(kSemName1, 0);
	assert_posix_bool_success(sem != SEM_FAILED);

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(2, value);

	// trywait
	TEST("sem_trywait() success");
	assert_posix_success(sem_trywait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// trywait
	TEST("sem_trywait() success");
	assert_posix_success(sem_trywait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// trywait failure
	TEST("sem_trywait() failure");
	assert_posix_error(EAGAIN, sem_trywait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(2, value);

	// timedwait
	TEST("sem_timedwait() success");
	timespec timeout;
	assert_posix_success(sem_timedwait(sem,
		absolute_timeout(timeout, 1000000)));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	TEST("sem_timedwait() success");
	assert_posix_success(sem_timedwait(sem,
		absolute_timeout(timeout, 1000000)));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("sem_timedwait() timeout");
	bigtime_t startTime = system_time();
	assert_posix_error(ETIMEDOUT, sem_timedwait(sem,
		absolute_timeout(timeout, 1000000)));
	bigtime_t diffTime = system_time() - startTime;
	assert_time_equals(1000000, diffTime);

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	TEST("done");
}


static void
test_post_wait_unnamed()
{
	TEST_SET("sem_{post,wait,trywait,timedwait}() unnamed semaphore");

	const char* currentTest = NULL;

	// init
	TEST("sem_init()");
	sem_t _sem;
	assert_posix_success(sem_init(&_sem, 0, 1));
	sem_t* sem = &_sem;

	TEST("sem_getvalue()");
	int value;
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(2, value);

	// wait
	TEST("sem_wait() non-blocking");
	assert_posix_success(sem_wait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// wait
	TEST("sem_wait() non-blocking");
	assert_posix_success(sem_wait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(2, value);

	// trywait
	TEST("sem_trywait() success");
	assert_posix_success(sem_trywait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// trywait
	TEST("sem_trywait() success");
	assert_posix_success(sem_trywait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// trywait failure
	TEST("sem_trywait() failure");
	assert_posix_error(EAGAIN, sem_trywait(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// post
	TEST("sem_post() no waiting");
	assert_posix_success(sem_post(sem));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(2, value);

	// timedwait
	TEST("sem_timedwait() success");
	timespec timeout;
	assert_posix_success(sem_timedwait(sem,
		absolute_timeout(timeout, 1000000)));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	TEST("sem_timedwait() success");
	assert_posix_success(sem_timedwait(sem,
		absolute_timeout(timeout, 1000000)));

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("sem_timedwait() timeout");
	bigtime_t startTime = system_time();
	assert_posix_error(ETIMEDOUT, sem_timedwait(sem,
		absolute_timeout(timeout, 1000000)));
	bigtime_t diffTime = system_time() - startTime;
	assert_time_equals(1000000, diffTime);

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// destroy
	TEST("sem_destroy()");
	assert_posix_success(sem_destroy(sem));

	TEST("done");
}


static void
test_post_wait_named_fork()
{
	TEST_SET("sem_{post,wait,trywait,timedwait}() named semaphore with fork()");

	const char* currentTest = NULL;

	// make sure the sem doesn't exist yet
	sem_unlink(kSemName1);

	// open non-existing with O_CREAT
	TEST("sem_open(O_CREAT) non-existing");
	sem_t* sem = sem_open(kSemName1, O_CREAT, S_IRUSR | S_IWUSR, 0);
	assert_posix_bool_success(sem != SEM_FAILED);

	TEST("sem_getvalue()");
	int value;
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("unblock child after wait");
	pid_t child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		bigtime_t startTime = system_time();
		assert_posix_success(sem_wait(sem));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sleep(1);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("unblock parent after wait");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		sleep(1);
		assert_posix_success(sem_post(sem));

		exit(0);
	} else {
		// parent
		bigtime_t startTime = system_time();
		assert_posix_success(sem_wait(sem));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("unblock child after wait before timeout");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		timespec timeout;
		bigtime_t startTime = system_time();
		assert_posix_success(sem_timedwait(sem,
			absolute_timeout(timeout, 2000000)));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sleep(1);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("unblock child after wait after timeout");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		timespec timeout;
		bigtime_t startTime = system_time();
		assert_posix_error(ETIMEDOUT, sem_timedwait(sem,
			absolute_timeout(timeout, 1000000)));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sleep(2);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	TEST("done");
}


static void
test_post_wait_named_fork2()
{
	TEST_SET("sem_{post,wait,trywait,timedwait}() named semaphore open after "
		"fork");

	const char* currentTest = NULL;

	// make sure the sem doesn't exist yet
	sem_unlink(kSemName1);

	// open non-existing with O_CREAT
	TEST("sem_open(O_CREAT) non-existing");
	sem_t* sem = sem_open(kSemName1, O_CREAT, S_IRUSR | S_IWUSR, 0);
	assert_posix_bool_success(sem != SEM_FAILED);

	TEST("sem_getvalue()");
	int value;
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));
	sem = NULL;

	TEST("unblock child after wait");
	pid_t child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		sem = sem_open(kSemName1, 0);
		assert_posix_bool_success(sem != SEM_FAILED);

		bigtime_t startTime = system_time();
		assert_posix_success(sem_wait(sem));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sem = sem_open(kSemName1, 0);
		assert_posix_bool_success(sem != SEM_FAILED);

		sleep(1);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));
	sem = NULL;

	TEST("unblock child after wait before timeout");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		sem = sem_open(kSemName1, 0);
		assert_posix_bool_success(sem != SEM_FAILED);

		timespec timeout;
		bigtime_t startTime = system_time();
		assert_posix_success(sem_timedwait(sem,
			absolute_timeout(timeout, 2000000)));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sem = sem_open(kSemName1, 0);
		assert_posix_bool_success(sem != SEM_FAILED);

		sleep(1);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));
	sem = NULL;

	TEST("unblock child after wait after timeout");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		sem = sem_open(kSemName1, 0);
		assert_posix_bool_success(sem != SEM_FAILED);

		timespec timeout;
		bigtime_t startTime = system_time();
		assert_posix_error(ETIMEDOUT, sem_timedwait(sem,
			absolute_timeout(timeout, 1000000)));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sem = sem_open(kSemName1, 0);
		assert_posix_bool_success(sem != SEM_FAILED);

		sleep(2);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// close
	TEST("sem_close()");
	assert_posix_success(sem_close(sem));

	TEST("done");
}


static void
test_post_wait_unnamed_fork()
{
	TEST_SET("sem_{post,wait,trywait,timedwait}() unnamed semaphore with "
		"fork()");

	const char* currentTest = NULL;

	// init
	TEST("sem_init()");
	sem_t _sem;
	assert_posix_success(sem_init(&_sem, 1, 1));
	sem_t* sem = &_sem;

	TEST("sem_getvalue()");
	int value;
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	TEST("sem_wait() on fork()ed unnamed sem in parent and child");
	pid_t child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		sleep(1);
		assert_posix_success(sem_wait(sem));

		assert_posix_success(sem_getvalue(sem, &value));
		assert_equals(0, value);

		exit(0);
	} else {
		// parent
		assert_posix_success(sem_wait(sem));
		assert_posix_success(sem_getvalue(sem, &value));
		assert_equals(0, value);

		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("sem_post() on fork()ed unnamed sem in parent and child");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		assert_posix_success(sem_post(sem));

		assert_posix_success(sem_getvalue(sem, &value));
		assert_equals(1, value);

		exit(0);
	} else {
		// parent
		assert_posix_success(sem_post(sem));
		assert_posix_success(sem_getvalue(sem, &value));
		assert_equals(1, value);

		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// destroy
	TEST("sem_destroy()");
	assert_posix_success(sem_destroy(sem));

	TEST("done");
}


static void
test_post_wait_unnamed_fork_shared()
{
	TEST_SET("sem_{post,wait,trywait,timedwait}() unnamed semaphore with "
		"fork() in shared memory");

	const char* currentTest = NULL;

	// create shared memory area
	void* address = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANON, -1, 0);
	assert_posix_bool_success(address != MAP_FAILED);

	// init
	TEST("sem_init()");
	sem_t* sem = (sem_t*)address;
	assert_posix_success(sem_init(sem, 1, 0));

	TEST("sem_getvalue()");
	int value;
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("unblock child after wait");
	pid_t child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		bigtime_t startTime = system_time();
		assert_posix_success(sem_wait(sem));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sleep(1);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("unblock parent after wait");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		sleep(1);
		assert_posix_success(sem_post(sem));

		exit(0);
	} else {
		// parent
		bigtime_t startTime = system_time();
		assert_posix_success(sem_wait(sem));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("unblock child after wait before timeout");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		timespec timeout;
		bigtime_t startTime = system_time();
		assert_posix_success(sem_timedwait(sem,
			absolute_timeout(timeout, 2000000)));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sleep(1);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(0, value);

	TEST("unblock child after wait after timeout");
	child = fork();
	assert_posix_bool_success(child >= 0);

	if (child == 0) {
		// child
		timespec timeout;
		bigtime_t startTime = system_time();
		assert_posix_error(ETIMEDOUT, sem_timedwait(sem,
			absolute_timeout(timeout, 1000000)));
		bigtime_t diffTime = system_time() - startTime;
		assert_time_equals(1000000, diffTime);

		exit(0);
	} else {
		// parent
		sleep(2);
		assert_posix_success(sem_post(sem));
		wait_for_child(child);
	}

	TEST("sem_getvalue()");
	assert_posix_success(sem_getvalue(sem, &value));
	assert_equals(1, value);

	// destroy
	TEST("sem_destroy()");
	assert_posix_success(sem_destroy(sem));

	// unmap memory
	assert_posix_success(munmap(address, 4096));

	TEST("done");
}


int
main()
{
	test_open_close_unlink();
	test_init_destroy();
	test_open_close_fork();
	test_init_destroy_fork();
	test_post_wait_named();
	test_post_wait_unnamed();
	test_post_wait_named_fork();
	test_post_wait_named_fork2();
	test_post_wait_unnamed_fork();
	test_post_wait_unnamed_fork_shared();

	printf("\nall tests OK\n");
}
