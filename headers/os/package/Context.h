/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__CONTEXT_H_
#define _HAIKU__PACKAGE__CONTEXT_H_


#include <package/TempEntryManager.h>


namespace Haiku {

namespace Package {


class JobStateListener;


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

			TempEntryManager&	GetTempEntryManager() const;

			JobStateListener*	GetJobStateListener() const;
			void				SetJobStateListener(JobStateListener* listener);

			DecisionProvider&	GetDecisionProvider() const;

private:
	mutable	TempEntryManager	fTempEntryManager;
			DecisionProvider&	fDecisionProvider;
			JobStateListener*	fJobStateListener;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__CONTEXT_H_
