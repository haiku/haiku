/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILS_H
#define UTILS_H


#include <dirent.h>
#include <string.h>


inline bool operator<(const timespec& a, const timespec& b)
{
	return a.tv_sec < b.tv_sec
		|| (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec);
}


inline bool operator>(const timespec& a, const timespec& b)
{
	return b < a;
}


static inline bool
set_dirent_name(struct dirent* buffer, size_t bufferSize, const char* name,
	size_t nameLen)
{
	size_t length = (buffer->d_name + nameLen + 1) - (char*)buffer;
	if (length > bufferSize)
		return false;

	memcpy(buffer->d_name, name, nameLen);
	buffer->d_name[nameLen] = '\0';
	buffer->d_reclen = length;
	return true;
}


#endif	// UTILS_H
