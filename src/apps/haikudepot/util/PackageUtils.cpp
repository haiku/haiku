/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageUtils.h"


/*!	This method will obtain the title from the package if this is possible or
	otherwise it will return the name of the package.
*/

/*static*/ void
PackageUtils::TitleOrName(const PackageInfoRef package, BString& title)
{
	PackageUtils::Title(package, title);
	if (title.IsEmpty() && package.IsSet())
		title.SetTo(package->Name());
}


/*static*/ void
PackageUtils::Title(const PackageInfoRef package, BString& title)
{
	if (package.IsSet()) {
		PackageLocalizedTextRef localizedText = package->LocalizedText();

		if (localizedText.IsSet())
			title.SetTo(localizedText->Title());
		else
			title.SetTo("");
	} else {
		title.SetTo("");
	}
}


/*static*/ void
PackageUtils::Summary(const PackageInfoRef package, BString& summary)
{
	if (package.IsSet()) {
		PackageLocalizedTextRef localizedText = package->LocalizedText();

		if (localizedText.IsSet())
			summary.SetTo(localizedText->Summary());
		else
			summary.SetTo("");
	} else {
		summary.SetTo("");
	}
}
