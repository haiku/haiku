/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_LOCALIZED_TEXT_H
#define PACKAGE_LOCALIZED_TEXT_H


#include <Referenceable.h>
#include <String.h>


class PackageLocalizedTextBuilder;


/*!	Instances of this class should not be created directly; instead use the
	PackageLocalizedTextBuilder class as a builder-constructor.
*/
class PackageLocalizedText : public BReferenceable
{
friend class PackageLocalizedTextBuilder;

public:
								PackageLocalizedText();
								PackageLocalizedText(const PackageLocalizedText& other);

			bool				operator==(const PackageLocalizedText& other) const;
			bool				operator!=(const PackageLocalizedText& other) const;

			const BString&		Title() const;
			const BString&		Summary() const;
			const BString&		Description() const;
			const bool			HasChangelog() const;
			const BString&		Changelog() const;

private:

			void				SetTitle(const BString& value);
			void				SetSummary(const BString& value);
			void				SetDescription(const BString& value);
			void				SetHasChangelog(bool value);
			void				SetChangelog(const BString& value);

private:
			BString				fTitle;
			BString				fSummary;
			BString				fDescription;
			bool				fHasChangelog;
			BString				fChangelog;
				// ^ Note the changelog is not localized yet but will be at some point.
};


typedef BReference<PackageLocalizedText> PackageLocalizedTextRef;


class PackageLocalizedTextBuilder
{
public:
								PackageLocalizedTextBuilder();
								PackageLocalizedTextBuilder(const PackageLocalizedTextRef& value);
	virtual						~PackageLocalizedTextBuilder();

			PackageLocalizedTextRef
								BuildRef();

			PackageLocalizedTextBuilder&
								WithTitle(const BString& value);
			PackageLocalizedTextBuilder&
								WithSummary(const BString& value);
			PackageLocalizedTextBuilder&
								WithDescription(const BString& value);
			PackageLocalizedTextBuilder&
								WithHasChangelog(bool value);
			PackageLocalizedTextBuilder&
								WithChangelog(const BString& value);

private:
			void				_InitFromSource();
			void				_Init(const PackageLocalizedText* value);

private:
			PackageLocalizedTextRef
								fSource;
			BString				fTitle;
			BString				fSummary;
			BString				fDescription;
			bool				fHasChangelog;
			BString				fChangelog;
};


#endif // PACKAGE_LOCALIZED_TEXT_H
