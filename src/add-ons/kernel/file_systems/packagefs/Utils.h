/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILS_H
#define UTILS_H


inline bool operator<(const timespec& a, const timespec& b)
{
	return a.tv_sec < b.tv_sec
		|| (a.tv_sec == b.tv_sec && a.tv_nsec < b.tv_nsec);
}


inline bool operator>(const timespec& a, const timespec& b)
{
	return b < a;
}


#endif	// UTILS_H
