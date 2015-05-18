/*
 * Copyright 2011-2015, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__CONTEXT_H_
#define _PACKAGE__CONTEXT_H_


#include <Entry.h>
#include <String.h>


namespace BSupportKit {
	class BJobStateListener;
}


namespace BPackageKit {


namespace BPrivate {
	class TempfileManager;
}


struct BDecisionProvider {
	virtual						~BDecisionProvider();

	virtual	bool				YesNoDecisionNeeded(const BString& description,
									const BString& question,
									const BString& yes, const BString& no,
									const BString& defaultChoice);
//	virtual	bool				ActionsAcceptanceDecisionNeeded(
//									const BString& description,
//									const BString& question) = 0;
//	virtual	int32				ChoiceDecisionNeeded(
//									const BString& question) = 0;
};


class BContext {
public:
								BContext(BDecisionProvider& decisionProvider,
									BSupportKit::BJobStateListener&
										jobStateListener);
								~BContext();

			status_t			InitCheck() const;

			status_t			GetNewTempfile(const BString& baseName,
									BEntry* entry) const;

			BDecisionProvider&	DecisionProvider() const;
			BSupportKit::BJobStateListener&	JobStateListener() const;

private:
			status_t			_Initialize();

			BDecisionProvider&	fDecisionProvider;
			BSupportKit::BJobStateListener&	fJobStateListener;
			status_t			fInitStatus;

	mutable	BPrivate::TempfileManager*	fTempfileManager;
};


}	// namespace BPackageKit


#endif // _PACKAGE__CONTEXT_H_
