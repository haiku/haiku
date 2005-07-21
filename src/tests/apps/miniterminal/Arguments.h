/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <Rect.h>

class Arguments {
public:
	Arguments();
	~Arguments();

	void Parse(int argc, const char *const *argv);

	BRect Bounds() const		{ return fBounds; }
	const char *Title() const	{ return fTitle; }
	bool StandardShell() const	{ return fStandardShell; }
	void GetShellArguments(int &argc, const char *const *&argv) const;

private:
	void _SetShellArguments(int argc, const char *const *argv);

	BRect		fBounds;
	bool		fStandardShell;
	int			fShellArgumentCount;
	const char	**fShellArguments;
	const char	*fTitle;
};


#endif	// ARGUMENTS_H
