/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_MANAGER_UTILS_H
#define PACKAGE_MANAGER_UTILS_H


#include <package/manager/Exceptions.h>


#define DIE(...)											\
do {														\
	throw BFatalErrorException(__VA_ARGS__);				\
} while(0)


#define DIE_DETAILS(details, ...)									\
do {																\
	throw BFatalErrorException(__VA_ARGS__).SetDetails(details);	\
} while(0)


#endif	// PACKAGE_MANAGER_UTILS_H
