/*
 * Copyright 2007 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ramshankar, v.ramshankar@gmail.com
 */
#ifndef _COMMAND_PIPE_H
#define _COMMAND_PIPE_H


#include <stdio.h>

#include <List.h>
#include <OS.h>


class BMessage;
class BMessenger;
class BString;

namespace BPrivate {

class BCommandPipe {
public:
								BCommandPipe();
	virtual						~BCommandPipe();

			status_t			AddArg(const char* argv);
			void				PrintToStream() const;

	// FlushArgs() deletes the commands while Close() explicity closes all
	// pending pipe-ends
	// Note: Internally FlushArgs() calls Close()
			void				FlushArgs();
			void				Close();

	// You MUST NOT free/delete the strings in the array, but you MUST free
	// just the array itself.
			const char**		Argv(int32& _argc) const;

	// If you use these, you must explicitly call "close" for the parameters
	// (stdOut/stdErr) when you are done with them!
			thread_id			Pipe(int* stdOut, int* stdErr) const;
			thread_id			Pipe(int* stdOut) const;
			thread_id			PipeAll(int* stdOutAndErr) const;

	// If you use these, you need NOT call "fclose" for the parameters
	// (out/err) when you are done with them, also you need not do any
	// allocation for these FILE* pointers, just use FILE* out = NULL
	// and pass &out and so on...
			thread_id			PipeInto(FILE** _out, FILE** _err);
			thread_id			PipeInto(FILE** _outAndErr);

	// Run() is a synchronous call, and waits till the command has finished
	// executing RunAsync() is an asynchronous call that returns immediately
	// after launching the command Neither of these bother about redirecting
	// pipes for you to use
			void				Run();
			void				RunAsync();

	// Reading the Pipe output

			class LineReader {
			public:
				virtual				~LineReader() {}
				virtual	bool		IsCanceled() = 0;
				virtual	status_t	ReadLine(const BString& line) = 0;
				// TODO: Add a Timeout() method.
			};

	// This function reads line-by-line from "file". It calls IsCanceled()
	// on the passed LineReader instance for each byte read from file
	// (currently). And it calls ReadLine() for each complete line.
			status_t			ReadLines(FILE* file, LineReader* lineReader);
	// This method can be used to read the entire file into a BString.
			BString				ReadLines(FILE* file);

	// Stardard append operators, if you use pointers to a BCommandPipe,
	// you must use *pipe << "command"; and not pipe << "command" (as it
	// will not compile that way)
			BCommandPipe&		operator<<(const char* arg);
			BCommandPipe&		operator<<(const BString& arg);
			BCommandPipe&		operator<<(const BCommandPipe& arg);

protected:
			BList				fArgList;
			int					fStdOut[2];
			int					fStdErr[2];
			bool				fStdOutOpen;
			bool				fStdErrOpen;
};

}	// namespace BPrivate

using BPrivate::BCommandPipe;

#endif // _COMMAND_PIPE_H
