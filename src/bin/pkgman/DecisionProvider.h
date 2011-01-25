/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef DECISION_PROVIDER_H
#define DECISION_PROVIDER_H


#include <package/Context.h>


struct DecisionProvider : public BPackageKit::BDecisionProvider {
	virtual bool YesNoDecisionNeeded(const BString& description,
		const BString& question, const BString& yes, const BString& no,
		const BString& defaultChoice);
};


#endif	// DECISION_PROVIDER_H
