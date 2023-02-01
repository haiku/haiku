/*
 * Copyright 2022, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCOPE_EXIT_H
#define _SCOPE_EXIT_H


#if __cplusplus < 201103L
#error This file requires compiler support for the C++11 standard.
#endif


#include <utility>


template<typename F>
class ScopeExit
{
public:
	explicit ScopeExit(F&& fn) : fFn(fn)
	{
	}

	~ScopeExit()
	{
		fFn();
	}

	ScopeExit(ScopeExit&& other) : fFn(std::move(other.fFn))
	{
	}

private:
	ScopeExit(const ScopeExit&);
	ScopeExit& operator=(const ScopeExit&);

private:
	F fFn;
};


#endif // _SCOPE_EXIT_H
