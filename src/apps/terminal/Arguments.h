/*
 * Copyright 2005-2018, Haiku, Inc. All rights reserved.
 * Copyright 2005, Ingo Weinhold, <bonefish@users.sf.net>
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Jeremiah Bailey, <jjbailey@gmail.com>
 *	Ingo Weinhold, <bonefish@users.sf.net>
 */


#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <Rect.h>

class Arguments {
public:
	Arguments(int defaultArgcNum, const char* const* defaultArgv);
	~Arguments();

	void Parse(int argc, const char* const* argv);

	BRect Bounds() const			{ return fBounds; }
	const char* Title() const		{ return fTitle; }
	const char* WorkingDir() const	{ return fWorkingDirectory; }
	bool StandardShell() const		{ return fStandardShell; }
	bool FullScreen() const			{ return fFullScreen; }
	bool UsageRequested() const		{ return fUsageRequested; }
	void GetShellArguments(int& argc, const char* const*& argv) const;

private:
	void _SetShellArguments(int argc, const char* const* argv);

	bool			fUsageRequested;
	BRect			fBounds;
	bool			fStandardShell;
	bool			fFullScreen;
	int				fShellArgumentCount;
	const char**	fShellArguments;
	const char*		fTitle;
	const char*		fWorkingDirectory;
};


#endif	// ARGUMENTS_H
