/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*/
#ifndef FILTERIO_H
#define FILTERIO_H


#include <DataIO.h>
#include <OS.h>


class BString;


class FilterIO : public BDataIO
{
public:
						FilterIO(int argc, const char** argv,
							const char** envp = NULL);
						FilterIO(const BString& cmdline);
						~FilterIO();

			status_t	InitCheck() const
						{
							return fInitErr;
						}

			ssize_t		Read(void* buffer, size_t size);
			ssize_t		Write(const void* buffer, size_t size);
private:
			int			fStdIn, fStdOut, fStdErr;
			thread_id	fThreadId;
			status_t	fInitErr;

			status_t	InitData(int argc, const char** argv,
							const char** envp = NULL);
			thread_id	PipeCommand(int argc, const char** argv, int& in,
							int& out, int& err, const char** envp = NULL);
};

#endif // FILTERIO_H
