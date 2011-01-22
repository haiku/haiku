/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef MY_DECISION_PROVIDER_H
#define MY_DECISION_PROVIDER_H


#include <package/Context.h>


struct MyDecisionProvider : public Haiku::Package::DecisionProvider {
	virtual bool YesNoDecisionNeeded(const BString& description,
		const BString& question, const BString& yes, const BString& no,
		const BString& defaultChoice);
};


#endif	// MY_DECISION_PROVIDER_H
