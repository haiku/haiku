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
		
	// If you use these, you must explicitly call "close" for the parameters
	// (outdes/errdes) when you are done with them!
			thread_id			Pipe(int* outdes, int* errdes) const;
			thread_id			Pipe(int* outdes) const;
			thread_id			PipeAll(int* outAndErrDes) const;
		
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
		
	// This function reads line-by-line from "file", cancels its reading
	// when "*cancel" is true. It reports each line it has read to "target"
	// using the supplied "_message" and string field name. "cancel" can be
	// NULL
			BString				ReadLines(FILE* file, bool* cancel,
									BMessenger& target, const BMessage& message,
									const BString& stringFieldName);
		
	// You need NOT free/delete	the return array of strings
			const char**		Argv(int32& _argc) const;
		
	// Stardard append operators, if you use pointers to a BCommandPipe,
	// you must use *pipe << "command"; and not pipe << "command" (as it
	// will not compile that way)
			BCommandPipe&		operator<<(const char *arg);
			BCommandPipe&		operator<<(const BString& arg);
			BCommandPipe&		operator<<(const BCommandPipe& arg);
		
	protected:
			BList				fArgList;
			int					fOutDes[2];
			int					fErrDes[2];
			bool				fOutDesOpen;
			bool				fErrDesOpen;
};

}	// namespace BPrivate

using BPrivate::BCommandPipe;

#endif // _COMMAND_PIPE_H
