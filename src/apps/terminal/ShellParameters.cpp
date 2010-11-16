/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "ShellParameters.h"


ShellParameters::ShellParameters(int argc, const char* const* argv,
	const BString& currentDirectory)
	:
	fArguments(argv),
	fArgumentCount(argc),
	fCurrentDirectory(currentDirectory),
	fEncoding("UTF8")
{
}


void
ShellParameters::SetArguments(int argc, const char* const* argv)
{
	fArguments = argv;
	fArgumentCount = argc;
}


void
ShellParameters::SetCurrentDirectory(const BString& currentDirectory)
{
	fCurrentDirectory = currentDirectory;
}


void
ShellParameters::SetEncoding(const BString& encoding)
{
	fEncoding = encoding;
}
