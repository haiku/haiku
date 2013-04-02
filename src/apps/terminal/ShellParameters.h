/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SHELL_PARAMETERS_H
#define SHELL_PARAMETERS_H


#include <String.h>


class ShellParameters {
public:
								ShellParameters(int argc,
									const char* const* argv,
									const BString& currentDirectory
										= BString());

			void				SetArguments(int argc, const char* const* argv);
			const char* const*	Arguments() const
									{ return fArguments; }
			int					ArgumentCount() const
									{ return fArgumentCount; }

			void				SetCurrentDirectory(
									const BString& currentDirectory);
			const BString&		CurrentDirectory() const
									{ return fCurrentDirectory; }

			void				SetEncoding(int encoding);
			int					Encoding() const
									{ return fEncoding; }

private:
			const char* const*	fArguments;
			int					fArgumentCount;
			BString				fCurrentDirectory;
			int					fEncoding;
};



#endif	// SHELL_PARAMETERS_H
