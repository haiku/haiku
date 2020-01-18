/*
 * Copyright 2013-2014, Rene Gollent, rene@gollent.com.
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "CLanguageFamily.h"

#include <new>

#include <stdlib.h>

#include "CLanguageExpressionEvaluator.h"
#include "CLanguageFamilySyntaxHighlighter.h"
#include "CLanguageTokenizer.h"
#include "ExpressionInfo.h"
#include "TeamTypeInformation.h"
#include "StringValue.h"
#include "Type.h"
#include "TypeLookupConstraints.h"


using CLanguage::ParseException;


CLanguageFamily::CLanguageFamily()
{
}


CLanguageFamily::~CLanguageFamily()
{
}


SyntaxHighlighter*
CLanguageFamily::GetSyntaxHighlighter() const
{
	return new(std::nothrow) CLanguageFamilySyntaxHighlighter();
}


status_t
CLanguageFamily::EvaluateExpression(const BString& expression,
	ValueNodeManager* manager, TeamTypeInformation* info,
	ExpressionResult*& _output, ValueNode*& _neededNode)
{
	_output = NULL;
	_neededNode = NULL;
	CLanguageExpressionEvaluator evaluator;
	try {
		_output = evaluator.Evaluate(expression, manager, info);
		return B_OK;
	} catch (ParseException& ex) {
		BString error;
		error.SetToFormat("Parse error at position %" B_PRId32 ": %s",
			ex.position, ex.message.String());
		StringValue* value = new(std::nothrow) StringValue(error.String());
		if (value == NULL)
			return B_NO_MEMORY;
		BReference<Value> valueReference(value, true);
		_output = new(std::nothrow) ExpressionResult();
		if (_output == NULL)
			return B_NO_MEMORY;
		_output->SetToPrimitive(value);
		return B_BAD_DATA;
	} catch (ValueNeededException& ex) {
		_neededNode = ex.value;
	}

	return B_OK;
}
