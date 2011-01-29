/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_INFO_H_
#define _PACKAGE__PACKAGE_INFO_H_


#include <ObjectList.h>
#include <String.h>

#include <package/PackageResolvable.h>
#include <package/PackageResolvableExpression.h>
#include <package/PackageVersion.h>


class BEntry;


namespace BPackageKit {


enum BPackageInfoIndex {
	B_PACKAGE_INFO_NAME = 0,
	B_PACKAGE_INFO_SUMMARY,		// single line
	B_PACKAGE_INFO_DESCRIPTION,	// multiple lines possible
	B_PACKAGE_INFO_VENDOR,		// e.g. "Haiku Project"
	B_PACKAGE_INFO_PACKAGER,	// e-mail address preferred
	B_PACKAGE_INFO_ARCHITECTURE,
	B_PACKAGE_INFO_VERSION,		// <major>[.<minor>[.<micro>]]
	B_PACKAGE_INFO_COPYRIGHTS,	// list
	B_PACKAGE_INFO_LICENSES,	// list
	B_PACKAGE_INFO_PROVIDES,	// list of resolvables this package provides,
								// each optionally giving a version
	B_PACKAGE_INFO_REQUIRES,	// list of resolvables required by this package,
								// each optionally specifying a version relation
								// (e.g. libssl.so >= 0.9.8)
	B_PACKAGE_INFO_SUPPLEMENTS,	// list of resolvables that are supplemented
								// by this package, i.e. this package marks
								// itself as an extension to other packages
	B_PACKAGE_INFO_CONFLICTS,	// list of resolvables that inhibit installation
								// of this package
	B_PACKAGE_INFO_REPLACES,	// list of resolvables that this package
								// will replace (upon update)
	B_PACKAGE_INFO_FRESHENS,	// list of resolvables that this package
								// contains a patch for
	//
	B_PACKAGE_INFO_ENUM_COUNT,
};


enum BPackageArchitecture {
	B_PACKAGE_ARCHITECTURE_ANY = 0,
	B_PACKAGE_ARCHITECTURE_X86,
	B_PACKAGE_ARCHITECTURE_X86_GCC2,
	//
	B_PACKAGE_ARCHITECTURE_ENUM_COUNT,
};


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

			BPackageArchitecture	Architecture() const;

			const BPackageVersion&	Version() const;

			const BObjectList<BString>&	CopyrightList() const;
			const BObjectList<BString>&	LicenseList() const;

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

			void				SetArchitecture(
									BPackageArchitecture architecture);

			void				SetVersion(const BPackageVersion& version);

			void				ClearCopyrightList();
			status_t			AddCopyright(const BString& copyright);

			void				ClearLicenseList();
			status_t			AddLicense(const BString& license);

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

			BPackageArchitecture	fArchitecture;

			BPackageVersion		fVersion;

			BObjectList<BString>	fCopyrightList;
			BObjectList<BString>	fLicenseList;

			BObjectList<BPackageResolvable>	fProvidesList;

			BObjectList<BPackageResolvableExpression>	fRequiresList;
			BObjectList<BPackageResolvableExpression>	fSupplementsList;

			BObjectList<BPackageResolvableExpression>	fConflictsList;

			BObjectList<BPackageResolvableExpression>	fFreshensList;

			BObjectList<BString>	fReplacesList;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_INFO_H_
