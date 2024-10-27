/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LOCALIZED_TEXT_H
#define PACKAGE_LOCALIZED_TEXT_H


#include <Referenceable.h>
#include <String.h>


class PackageLocalizedText : public BReferenceable
{
public:
								PackageLocalizedText();
								PackageLocalizedText(const PackageLocalizedText& other);

			const BString&		Title() const;
			void				SetTitle(const BString& value);

			const BString&		Summary() const;
			void				SetSummary(const BString& value);

			const BString&		Description() const;
			void				SetDescription(const BString& value);

			const bool			HasChangelog() const;
			void				SetHasChangelog(bool value);

			void				SetChangelog(const BString& value);
			const BString&		Changelog() const;

			bool				operator==(const PackageLocalizedText& other) const;
			bool				operator!=(const PackageLocalizedText& other) const;

private:
			BString				fTitle;
			BString				fSummary;
			BString				fDescription;
			bool				fHasChangelog;
			BString				fChangelog;
				// ^ Note the changelog is not localized yet but will be at some point.
};


typedef BReference<PackageLocalizedText> PackageLocalizedTextRef;


#endif // PACKAGE_LOCALIZED_TEXT_H
