/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "BaseJob.h"

#include <Message.h>

#include "Conditions.h"


BaseJob::BaseJob(const char* name)
	:
	BJob(name),
	fCondition(NULL)
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


const BStringList&
BaseJob::Environment() const
{
	return fEnvironment;
}


const BStringList&
BaseJob::EnvironmentSourceFiles() const
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
		if (strcmp(name, "from_file") == 0) {
			const char* fromFile;
			for (int32 fileIndex = 0; message.FindString(name, fileIndex,
					&fromFile) == B_OK; fileIndex++) {
				fSourceFiles.Add(fromFile);
			}
			continue;
		}

		BString variable = name;
		variable << "='";

		const char* argument;
		for (int32 argumentIndex = 0; message.FindString(name, argumentIndex,
				&argument) == B_OK; argumentIndex++) {
			if (argumentIndex > 0)
				variable << " ";
			variable += argument;
		}
		variable << "'";

		fEnvironment.Add(variable);
	}
}
