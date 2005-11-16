#include <stdio.h>

#include <string>

#include "exceptions.h"

using std::string;

static const char *kCaughtNothing		= "nothing";
static const char *kCaughtGeneric		= "generic";
static const char *kCaughtBase			= "ExceptionBase";
static const char *kCaughtA				= "ExceptionA";
static const char *kCaughtB				= "ExceptionB";
static const char *kCaughtVirtualBase	= "VirtualExceptionBase";
static const char *kCaughtVirtualA		= "VirtualExceptionA";
static const char *kCaughtVirtualB		= "VirtualExceptionB";
static const char *kCaughtInt			= "VirtualInt";


static string
catchBase(void (*function)())
{
	try {
		(*function)();
	} catch (ExceptionBase exception) {
		return kCaughtBase;
	} catch (...) {
		return kCaughtGeneric;
	}
	return kCaughtNothing;
}


static string
catchA(void (*function)())
{
	try {
		(*function)();
	} catch (ExceptionA exception) {
		return kCaughtA;
	} catch (...) {
		return kCaughtGeneric;
	}
	return kCaughtNothing;
}


static string
catchB(void (*function)())
{
	try {
		(*function)();
	} catch (ExceptionB exception) {
		return kCaughtB;
	} catch (...) {
		return kCaughtGeneric;
	}
	return kCaughtNothing;
}


static string
catchVirtualBase(void (*function)())
{
	try {
		(*function)();
	} catch (VirtualExceptionBase exception) {
		return kCaughtVirtualBase;
	} catch (...) {
		return kCaughtGeneric;
	}
	return kCaughtNothing;
}


static string
catchVirtualA(void (*function)())
{
	try {
		(*function)();
	} catch (VirtualExceptionA exception) {
		return kCaughtVirtualA;
	} catch (...) {
		return kCaughtGeneric;
	}
	return kCaughtNothing;
}


static string
catchVirtualB(void (*function)())
{
	try {
		(*function)();
	} catch (VirtualExceptionB exception) {
		return kCaughtVirtualB;
	} catch (...) {
		return kCaughtGeneric;
	}
	return kCaughtNothing;
}


static string
catchInt(void (*function)())
{
	try {
		(*function)();
	} catch (int exception) {
		return kCaughtInt;
	} catch (...) {
		return kCaughtGeneric;
	}
	return kCaughtNothing;
}

static string
catchAny(void (*function)())
{
	try {
		(*function)();
	} catch (int exception) {
		return kCaughtInt;
	} catch (VirtualExceptionA exception) {
		return kCaughtVirtualA;
	} catch (VirtualExceptionB exception) {
		return kCaughtVirtualB;
	} catch (VirtualExceptionBase exception) {
		return kCaughtVirtualBase;
	} catch (ExceptionA exception) {
		return kCaughtA;
	} catch (ExceptionB exception) {
		return kCaughtB;
	} catch (ExceptionBase exception) {
		return kCaughtBase;
	} catch (...) {
		return kCaughtGeneric;
	}
	return kCaughtNothing;
}

static void
test(string (*catcher)(void (*)()), void (*thrower)(), const char *expected)
{
	string caught((*catcher)(thrower));
	if (caught != expected) {
		printf("ERROR: expected exception: %s, but caught: %s\n", expected,
			caught.c_str());
	}
}

int
main()
{
	test(catchBase, throwBase, kCaughtBase);
	test(catchBase, throwA, kCaughtBase);
	test(catchBase, throwB, kCaughtBase);
	test(catchBase, throwVirtualBase, kCaughtBase);
	test(catchBase, throwVirtualA, kCaughtBase);
	test(catchBase, throwVirtualB, kCaughtBase);
	test(catchBase, throwInt, kCaughtGeneric);

	test(catchA, throwBase, kCaughtGeneric);
	test(catchA, throwA, kCaughtA);
	test(catchA, throwB, kCaughtGeneric);
	test(catchA, throwVirtualBase, kCaughtGeneric);
	test(catchA, throwVirtualA, kCaughtGeneric);
	test(catchA, throwVirtualB, kCaughtGeneric);
	test(catchA, throwInt, kCaughtGeneric);

	test(catchVirtualBase, throwBase, kCaughtGeneric);
	test(catchVirtualBase, throwA, kCaughtGeneric);
	test(catchVirtualBase, throwB, kCaughtGeneric);
	test(catchVirtualBase, throwVirtualBase, kCaughtVirtualBase);
	test(catchVirtualBase, throwVirtualA, kCaughtVirtualBase);
	test(catchVirtualBase, throwVirtualB, kCaughtVirtualBase);
	test(catchVirtualBase, throwInt, kCaughtGeneric);

	test(catchVirtualA, throwBase, kCaughtGeneric);
	test(catchVirtualA, throwA, kCaughtGeneric);
	test(catchVirtualA, throwB, kCaughtGeneric);
	test(catchVirtualA, throwVirtualBase, kCaughtGeneric);
	test(catchVirtualA, throwVirtualA, kCaughtVirtualA);
	test(catchVirtualA, throwVirtualB, kCaughtGeneric);
	test(catchVirtualA, throwInt, kCaughtGeneric);

	test(catchInt, throwBase, kCaughtGeneric);
	test(catchInt, throwA, kCaughtGeneric);
	test(catchInt, throwB, kCaughtGeneric);
	test(catchInt, throwVirtualBase, kCaughtGeneric);
	test(catchInt, throwVirtualA, kCaughtGeneric);
	test(catchInt, throwVirtualB, kCaughtGeneric);
	test(catchInt, throwInt, kCaughtInt);

	test(catchAny, throwBase, kCaughtBase);
	test(catchAny, throwA, kCaughtA);
	test(catchAny, throwB, kCaughtB);
	test(catchAny, throwVirtualBase, kCaughtVirtualBase);
	test(catchAny, throwVirtualA, kCaughtVirtualA);
	test(catchAny, throwVirtualB, kCaughtVirtualB);
	test(catchAny, throwInt, kCaughtInt);

	return 0;
}
