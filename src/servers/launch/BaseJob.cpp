/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "BaseJob.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Message.h>

#include "Conditions.h"
#include "Events.h"


BaseJob::BaseJob(const char* name)
	:
	BJob(name),
	fCondition(NULL),
	fEvent(NULL)
{
}


BaseJob::~BaseJob()
{
	delete fCondition;
}


const char*
BaseJob::Name() const
{
	return Title().String();
}


const ::Condition*
BaseJob::Condition() const
{
	return fCondition;
}


::Condition*
BaseJob::Condition()
{
	return fCondition;
}


void
BaseJob::SetCondition(::Condition* condition)
{
	fCondition = condition;
}


bool
BaseJob::CheckCondition(ConditionContext& context) const
{
	if (fCondition != NULL)
		return fCondition->Test(context);

	return true;
}


const ::Event*
BaseJob::Event() const
{
	return fEvent;
}


::Event*
BaseJob::Event()
{
	return fEvent;
}


void
BaseJob::SetEvent(::Event* event)
{
	fEvent = event;
	if (event != NULL)
		event->SetOwner(this);
}


/*!	Determines whether the events of this job has been triggered
	already or not.
	Note, if this job does not have any events, this method returns
	\c true.
*/
bool
BaseJob::EventHasTriggered() const
{
	return Event() == NULL || Event()->Triggered();
}


const BStringList&
BaseJob::Environment() const
{
	return fEnvironment;
}


BStringList&
BaseJob::Environment()
{
	return fEnvironment;
}


const BStringList&
BaseJob::EnvironmentSourceFiles() const
{
	return fSourceFiles;
}


BStringList&
BaseJob::EnvironmentSourceFiles()
{
	return fSourceFiles;
}


void
BaseJob::SetEnvironment(const BMessage& message)
{
	char* name;
	type_code type;
	int32 count;
	for (int32 index = 0; message.GetInfo(B_STRING_TYPE, index, &name, &type,
			&count) == B_OK; index++) {
		if (strcmp(name, "from_script") == 0) {
			const char* fromScript;
			for (int32 scriptIndex = 0; message.FindString(name, scriptIndex,
					&fromScript) == B_OK; scriptIndex++) {
				fSourceFiles.Add(fromScript);
			}
			continue;
		}

		BString variable = name;
		variable << "=";

		const char* argument;
		for (int32 argumentIndex = 0; message.FindString(name, argumentIndex,
				&argument) == B_OK; argumentIndex++) {
			if (argumentIndex > 0)
				variable << " ";
			variable += argument;
		}

		fEnvironment.Add(variable);
	}
}


void
BaseJob::GetSourceFilesEnvironment(BStringList& environment)
{
	int32 count = fSourceFiles.CountStrings();
	for (int32 index = 0; index < count; index++) {
		_GetSourceFileEnvironment(fSourceFiles.StringAt(index), environment);
	}
}


/*!	Gets the environment by evaluating the source files, and move that
	environment to the static environment.

	When this method returns, the source files list will be empty.
*/
void
BaseJob::ResolveSourceFiles()
{
	if (fSourceFiles.IsEmpty())
		return;

	GetSourceFilesEnvironment(fEnvironment);
	fSourceFiles.MakeEmpty();
}


void
BaseJob::_GetSourceFileEnvironment(const char* script, BStringList& environment)
{
	int pipes[2];
	if (pipe(&pipes[0]) != 0) {
		// TODO: log error
		return;
	}

	pid_t child = fork();
	if (child < 0) {
		// TODO: log error
		debug_printf("could not fork: %s\n", strerror(errno));
	} else if (child == 0) {
		// We're the child, redirect stdout
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		dup2(pipes[1], STDOUT_FILENO);
		dup2(pipes[1], STDERR_FILENO);

		for (int32 i = 0; i < 2; i++)
			close(pipes[i]);

		BString command;
		command.SetToFormat(". \"%s\"; export -p", script);
		execl("/bin/sh", "/bin/sh", "-c", command.String(), NULL);
		exit(1);
	} else {
		// Retrieve environment from child

		close(pipes[1]);

		BString line;
		char buffer[4096];
		while (true) {
			ssize_t bytesRead = read(pipes[0], buffer, sizeof(buffer) - 1);
			if (bytesRead <= 0)
				break;

			// Make sure the buffer is null terminated
			buffer[bytesRead] = 0;

			const char* chunk = buffer;
			while (true) {
				const char* separator = strchr(chunk, '\n');
				if (separator == NULL) {
					line.Append(chunk, bytesRead);
					break;
				}
				line.Append(chunk, separator - chunk);
				chunk = separator + 1;

				_ParseExportVariable(environment, line);
				line.Truncate(0);
			}
		}
		if (!line.IsEmpty())
			_ParseExportVariable(environment, line);

		close(pipes[0]);
	}
}


void
BaseJob::_ParseExportVariable(BStringList& environment, const BString& line)
{
	if (!line.StartsWith("export "))
		return;

	int separator = line.FindFirst("=\"");
	if (separator < 0)
		return;

	BString variable;
	line.CopyInto(variable, 7, separator - 7);

	BString value;
	line.CopyInto(value, separator + 2, line.Length() - separator - 3);

	variable << "=" << value;
	environment.Add(variable);
}
