/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <new>

#include "CfaRuleSet.h"


CfaRuleSet::CfaRuleSet()
	:
	fRegisterRules(NULL),
	fRegisterCount(0)
{
}


status_t
CfaRuleSet::Init(uint32 registerCount)
{
	fRegisterRules = new(std::nothrow) CfaRule[registerCount];
	if (fRegisterRules == NULL)
		return B_NO_MEMORY;

	fRegisterCount = registerCount;

	return B_OK;
}


CfaRuleSet*
CfaRuleSet::Clone() const
{
	CfaRuleSet* other = new(std::nothrow) CfaRuleSet;
	if (other == NULL)
		return NULL;

	if (other->Init(fRegisterCount) != B_OK) {
		delete other;
		return NULL;
	}

	other->fCfaCfaRule = fCfaCfaRule;
	memcpy(other->fRegisterRules, fRegisterRules,
		sizeof(CfaRule) * fRegisterCount);

	return other;
}


CfaRule*
CfaRuleSet::RegisterRule(uint32 index) const
{
	return index < fRegisterCount ? fRegisterRules + index : NULL;
}
