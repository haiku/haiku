/*
 * Copyright 2014, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */

#include "Jobs.h"

#include <String.h>

#include <AutoLocker.h>

#include "SourceLanguage.h"
#include "StackFrame.h"
#include "Team.h"
#include "Value.h"


ExpressionEvaluationJob::ExpressionEvaluationJob(Team* team,
	SourceLanguage* language, const char* expression, type_code resultType,
	StackFrame* frame)
	:
	fKey(expression, JOB_TYPE_EVALUATE_EXPRESSION),
	fTeam(team),
	fLanguage(language),
	fExpression(expression),
	fResultType(resultType),
	fFrame(frame)
{
	fLanguage->AcquireReference();
	if (fFrame != NULL)
		fFrame->AcquireReference();
}


ExpressionEvaluationJob::~ExpressionEvaluationJob()
{
	fLanguage->ReleaseReference();
	if (fFrame != NULL)
		fFrame->ReleaseReference();
}


const JobKey&
ExpressionEvaluationJob::Key() const
{
	return fKey;
}


status_t
ExpressionEvaluationJob::Do()
{
	Value* value = NULL;
	status_t result = fLanguage->EvaluateExpression(fExpression, fResultType,
		value);
	BReference<Value> reference;
	if (value != NULL)
		reference.SetTo(value, true);

	AutoLocker<Team> teamLocker(fTeam);
	fTeam->NotifyExpressionEvaluated(fExpression.String(), result, value);

	return B_OK;
}
