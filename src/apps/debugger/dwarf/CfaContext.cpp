/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include "CfaContext.h"


CfaContext::CfaContext()
	:
	fTargetLocation(0),
	fLocation(0),
	fCodeAlignment(0),
	fDataAlignment(0),
	fReturnAddressRegister(0),
	fRuleSet(NULL),
	fInitialRuleSet(NULL),
	fRuleSetStack(10, true)
{
}


CfaContext::~CfaContext()
{
	delete fRuleSet;
	delete fInitialRuleSet;
}


void
CfaContext::SetLocation(target_addr_t targetLocation,
	target_addr_t initialLocation)
{
	fTargetLocation = targetLocation;
	fLocation = initialLocation;
}


status_t
CfaContext::Init(uint32 registerCount)
{
	fRuleSet = new(std::nothrow) CfaRuleSet;
	if (fRuleSet == NULL)
		return B_NO_MEMORY;

	return fRuleSet->Init(registerCount);
}


status_t
CfaContext::SaveInitialRuleSet()
{
	fInitialRuleSet = fRuleSet->Clone();
	if (fInitialRuleSet == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


status_t
CfaContext::PushRuleSet()
{
	CfaRuleSet* ruleSet = fRuleSet->Clone();
	if (ruleSet == NULL || !fRuleSetStack.AddItem(ruleSet)) {
		delete ruleSet;
		return B_NO_MEMORY;
	}

	return B_OK;
}


status_t
CfaContext::PopRuleSet()
{
	if (fRuleSetStack.IsEmpty())
		return B_BAD_DATA;

	delete fRuleSet;
	fRuleSet = fRuleSetStack.RemoveItemAt(
		fRuleSetStack.CountItems() - 1);

	return B_OK;
}


void
CfaContext::SetLocation(target_addr_t location)
{
	fLocation = location;
}


void
CfaContext::SetCodeAlignment(uint32 alignment)
{
	fCodeAlignment = alignment;
}


void
CfaContext::SetDataAlignment(int32 alignment)
{
	fDataAlignment = alignment;
}


void
CfaContext::SetReturnAddressRegister(uint32 reg)
{
	fReturnAddressRegister = reg;
}


void
CfaContext::RestoreRegisterRule(uint32 reg)
{
	if (CfaRule* rule = RegisterRule(reg)) {
		if (fInitialRuleSet != NULL)
			*rule = *fInitialRuleSet->RegisterRule(reg);
	}
}
