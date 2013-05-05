/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef CFA_CONTEXT_H
#define CFA_CONTEXT_H


#include <ObjectList.h>

#include "CfaRuleSet.h"
#include "Types.h"


class CfaContext {
public:
								CfaContext();
								~CfaContext();

			void				SetLocation(target_addr_t targetLocation,
									target_addr_t initialLocation);

			status_t			Init(uint32 registerCount);
			status_t			SaveInitialRuleSet();

			status_t			PushRuleSet();
			status_t			PopRuleSet();

			target_addr_t		TargetLocation() const
									{ return fTargetLocation; }

			target_addr_t		Location() const
									{ return fLocation; }
			void				SetLocation(target_addr_t location);

			uint32				CodeAlignment() const
									{ return fCodeAlignment; }
			void				SetCodeAlignment(uint32 alignment);

			int32				DataAlignment() const
									{ return fDataAlignment; }
			void				SetDataAlignment(int32 alignment);

			uint32				ReturnAddressRegister() const
									{ return fReturnAddressRegister; }
			void				SetReturnAddressRegister(uint32 reg);

			CfaCfaRule*			GetCfaCfaRule() const
									{ return fRuleSet->GetCfaCfaRule(); }
			CfaRule*			RegisterRule(uint32 index) const
									{ return fRuleSet->RegisterRule(index); }

			void				RestoreRegisterRule(uint32 reg);

private:
			typedef BObjectList<CfaRuleSet> RuleSetList;

private:
			target_addr_t		fTargetLocation;
			target_addr_t		fLocation;
			uint32				fCodeAlignment;
			int32				fDataAlignment;
			uint32				fReturnAddressRegister;
			CfaRuleSet*			fRuleSet;
			CfaRuleSet*			fInitialRuleSet;
			RuleSetList			fRuleSetStack;
};



#endif	// CFA_CONTEXT_H
