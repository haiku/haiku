/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef DECISION_PROVIDER_H
#define DECISION_PROVIDER_H


#include <package/Context.h>


class DecisionProvider : public BPackageKit::BDecisionProvider {
public:
								DecisionProvider(bool interactive = true);

			void				SetInteractive(bool interactive)
									{ fInteractive = interactive; }

	virtual	bool				YesNoDecisionNeeded(const BString& description,
									const BString& question, const BString& yes,
									const BString& no,
									 const BString& defaultChoice);

private:
			bool				fInteractive;
};


#endif	// DECISION_PROVIDER_H
