/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_INFO_H_
#define _PACKAGE__PACKAGE_INFO_H_


#include <ObjectList.h>
#include <String.h>

#include <package/PackageArchitecture.h>
#include <package/PackageFlags.h>
#include <package/PackageInfoAttributes.h>
#include <package/PackageResolvable.h>
#include <package/PackageResolvableExpression.h>
#include <package/PackageVersion.h>


class BEntry;


namespace BPackageKit {


/*
 * Keeps a list of package info elements (e.g. name, version, vendor, ...) which
 * will be converted to package attributes when creating a package. Usually,
 * these elements have been parsed from a ".PackageInfo"-file.
 * Alternatively, the package reader populates a BPackageInfo object by
 * collecting package attributes from an existing package.
 */
class BPackageInfo {
public:
			struct ParseErrorListener {
				virtual	~ParseErrorListener();
				virtual void OnError(const BString& msg, int line, int col) = 0;
			};

public:
								BPackageInfo();
								~BPackageInfo();

			status_t			ReadFromConfigFile(
									const BEntry& packageInfoEntry,
									ParseErrorListener* listener = NULL);
			status_t			ReadFromConfigString(
									const BString& packageInfoString,
									ParseErrorListener* listener = NULL);

			status_t			InitCheck() const;

			const BString&		Name() const;
			const BString&		Summary() const;
			const BString&		Description() const;
			const BString&		Vendor() const;
			const BString&		Packager() const;
			const BString&		Checksum() const;

			uint32				Flags() const;

			BPackageArchitecture	Architecture() const;

			const BPackageVersion&	Version() const;

			const BObjectList<BString>&	CopyrightList() const;
			const BObjectList<BString>&	LicenseList() const;
			const BObjectList<BString>&	URLList() const;
			const BObjectList<BString>&	SourceURLList() const;

			const BObjectList<BPackageResolvable>&	ProvidesList() const;
			const BObjectList<BPackageResolvableExpression>&
								RequiresList() const;
			const BObjectList<BPackageResolvableExpression>&
								SupplementsList() const;
			const BObjectList<BPackageResolvableExpression>&
								ConflictsList() const;
			const BObjectList<BPackageResolvableExpression>&
								FreshensList() const;
			const BObjectList<BString>&	ReplacesList() const;

			void				SetName(const BString& name);
			void				SetSummary(const BString& summary);
			void				SetDescription(const BString& description);
			void				SetVendor(const BString& vendor);
			void				SetPackager(const BString& packager);
			void				SetChecksum(const BString& checksum);

			void				SetFlags(uint32 flags);

			void				SetArchitecture(
									BPackageArchitecture architecture);

			void				SetVersion(const BPackageVersion& version);

			void				ClearCopyrightList();
			status_t			AddCopyright(const BString& copyright);

			void				ClearLicenseList();
			status_t			AddLicense(const BString& license);

			void				ClearURLList();
			status_t			AddURL(const BString& url);

			void				ClearSourceURLList();
			status_t			AddSourceURL(const BString& url);

			void				ClearProvidesList();
			status_t			AddProvides(const BPackageResolvable& provides);

			void				ClearRequiresList();
			status_t			AddRequires(
									const BPackageResolvableExpression& expr);

			void				ClearSupplementsList();
			status_t			AddSupplements(
									const BPackageResolvableExpression& expr);

			void				ClearConflictsList();
			status_t			AddConflicts(
									const BPackageResolvableExpression& expr);

			void				ClearFreshensList();
			status_t			AddFreshens(
									const BPackageResolvableExpression& expr);

			void				ClearReplacesList();
			status_t			AddReplaces(const BString& replaces);

			void				Clear();

public:
	static	status_t			GetArchitectureByName(const BString& name,
									BPackageArchitecture& _architecture);

	static	const char*			kElementNames[];
	static	const char*			kArchitectureNames[];

private:
			class Parser;

private:
			BString				fName;
			BString				fSummary;
			BString				fDescription;
			BString				fVendor;
			BString				fPackager;

			uint32				fFlags;

			BPackageArchitecture	fArchitecture;

			BPackageVersion		fVersion;

			BObjectList<BString>	fCopyrightList;
			BObjectList<BString>	fLicenseList;
			BObjectList<BString>	fURLList;
			BObjectList<BString>	fSourceURLList;

			BObjectList<BPackageResolvable>	fProvidesList;

			BObjectList<BPackageResolvableExpression>	fRequiresList;
			BObjectList<BPackageResolvableExpression>	fSupplementsList;

			BObjectList<BPackageResolvableExpression>	fConflictsList;

			BObjectList<BPackageResolvableExpression>	fFreshensList;

			BObjectList<BString>	fReplacesList;

			BString				fChecksum;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_INFO_H_
