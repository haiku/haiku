/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__CONTEXT_H_
#define _HAIKU__PACKAGE__CONTEXT_H_


#include <package/TempfileManager.h>


namespace Haiku {

namespace Package {


class JobStateListener;
using Private::TempfileManager;


struct DecisionProvider {
	virtual						~DecisionProvider();

	virtual	bool				YesNoDecisionNeeded(const BString& description,
									const BString& question,
									const BString& yes, const BString& no,
									const BString& defaultChoice) = 0;
//	virtual	bool				ActionsAcceptanceDecisionNeeded(
//									const BString& description,
//									const BString& question) = 0;
//	virtual	int32				ChoiceDecisionNeeded(
//									const BString& question) = 0;
};


class Context {
public:
								Context(DecisionProvider& decisionProvider);
								~Context();

			TempfileManager&	GetTempfileManager() const;

			JobStateListener*	GetJobStateListener() const;
			void				SetJobStateListener(JobStateListener* listener);

			DecisionProvider&	GetDecisionProvider() const;

private:
	mutable	TempfileManager		fTempfileManager;
			DecisionProvider&	fDecisionProvider;
			JobStateListener*	fJobStateListener;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__CONTEXT_H_
