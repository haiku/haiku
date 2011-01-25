/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <string.h>

#include "DecisionProvider.h"


bool
DecisionProvider::YesNoDecisionNeeded(const BString& description,
	const BString& question, const BString& yes, const BString& no,
	const BString& defaultChoice)
{
	if (description.Length() > 0)
		printf("%s\n", description.String());

	bool haveDefault = defaultChoice.Length() > 0;

	while (true) {
		printf("%s [%s/%s]%s: ", question.String(), yes.String(), no.String(),
			haveDefault
				? (BString(" (") << defaultChoice << ") ").String() : "");

		char buffer[32];
		if (fgets(buffer, 32, stdin)) {
			if (haveDefault &&  (buffer[0] == '\n' || buffer[0] == '\0'))
				return defaultChoice == yes;
			int length = strlen(buffer);
			for (int i = 1; i <= length; ++i) {
				if (yes.ICompare(buffer, i) == 0) {
					if (no.ICompare(buffer, i) != 0)
						return true;
				} else if (no.Compare(buffer, i) == 0) {
					if (yes.ICompare(buffer, i) != 0)
						return false;
				} else
					break;
			}
			fprintf(stderr, "*** please enter '%s' or '%s'\n", yes.String(),
				no.String());
		}
	}
}
