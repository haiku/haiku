/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "BaseJob.h"

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
