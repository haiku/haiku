/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UNIT_TEST_UTILS_H
#define _UNIT_TEST_UTILS_H

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
	// allow 5% deviation
	bigtime_t diff = actual > expected ? actual - expected : expected - actual;
	if (diff <= expected / 20)
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

#endif // _UNIT_TEST_UTILS_H
