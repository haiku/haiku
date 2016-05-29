/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CFA_RULE_SET_H
#define CFA_RULE_SET_H


#include "CfaRule.h"


class CfaRuleSet {
public:
								CfaRuleSet();

			status_t			Init(uint32 registerCount);
			CfaRuleSet*			Clone() const;

			CfaCfaRule*			GetCfaCfaRule()			{ return &fCfaCfaRule; }
			const CfaCfaRule*	GetCfaCfaRule() const	{ return &fCfaCfaRule; }

			CfaRule*			RegisterRule(uint32 index) const;

private:
			CfaCfaRule			fCfaCfaRule;
			CfaRule*			fRegisterRules;
			uint32				fRegisterCount;
};


#endif	// CFA_RULE_SET_H
