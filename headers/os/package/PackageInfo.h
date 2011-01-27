/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_INFO_H_
#define _PACKAGE__PACKAGE_INFO_H_


#include <ObjectList.h>
#include <String.h>


class BEntry;


namespace BPackageKit {


enum BPackageInfoIndex {
	B_PACKAGE_INFO_NAME = 0,
	B_PACKAGE_INFO_SUMMARY,			// single line, 72 chars max
	B_PACKAGE_INFO_DESCRIPTION,		// multiple lines possible
	B_PACKAGE_INFO_VENDOR,			// e.g. "Haiku Project"
	B_PACKAGE_INFO_PACKAGER,		// e-mail address preferred
	B_PACKAGE_INFO_ARCHITECTURE,
	B_PACKAGE_INFO_VERSION,
	B_PACKAGE_INFO_COPYRIGHTS,		// list
	B_PACKAGE_INFO_LICENSES,		// list
	B_PACKAGE_INFO_PROVIDES,		// list
	B_PACKAGE_INFO_REQUIRES,		// list
	//
	B_PACKAGE_INFO_ENUM_COUNT,
};


enum BPackageArchitecture {
	B_PACKAGE_ARCHITECTURE_NONE = 0,
	B_PACKAGE_ARCHITECTURE_X86,
	B_PACKAGE_ARCHITECTURE_X86_GCC2,
	//
	B_PACKAGE_ARCHITECTURE_ENUM_COUNT,
};


class BPackageVersion {
public:
			void				MakeEmpty() {}
};


class BPackageProvision {
public:
			int					Compare(const BPackageProvision& other) const
			{
				return fX.Compare(other.fX);
			}
private:
			BString fX;
};


class BPackageRequirement {
public:
			int					Compare(const BPackageRequirement& other) const
			{
				return fX.Compare(other.fX);
			}
private:
			BString fX;
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

			const BObjectList<BString>&	Copyrights() const;
			const BObjectList<BString>&	Licenses() const;

			const BObjectList<BPackageRequirement>&	Requirements() const;
			const BObjectList<BPackageProvision>&	Provisions() const;

			void				SetName(const BString& name);
			void				SetSummary(const BString& summary);
			void				SetDescription(const BString& description);
			void				SetVendor(const BString& vendor);
			void				SetPackager(const BString& packager);

			void				SetArchitecture(
									BPackageArchitecture architecture);

			void				SetVersion(const BPackageVersion& version);

			void				ClearCopyrights();
			status_t			AddCopyright(const BString& copyright);
			status_t			RemoveCopyright(const BString& copyright);

			void				ClearLicenses();
			status_t			AddLicense(const BString& license);
			status_t			RemoveLicense(const BString& license);

			void				ClearProvisions();
			status_t			AddProvision(
									const BPackageProvision& provision);
			status_t			RemoveProvision(
									const BPackageProvision& provision);

			void				ClearRequirements();
			status_t			AddRequirement(
									const BPackageRequirement& requirement);
			status_t			RemoveRequirement(
									const BPackageRequirement& requirement);

			void				MakeEmpty();

public:
	static	status_t			GetElementName(
									BPackageInfoIndex index,
									const char** name);

private:
			class Parser;
	friend	class Parser;

	static	const char*			kElementNames[];
	static	const char*			kArchitectureNames[];

private:
			BString				fName;
			BString				fSummary;
			BString				fDescription;
			BString				fVendor;
			BString				fPackager;

			BPackageArchitecture	fArchitecture;

			BPackageVersion		fVersion;

			BObjectList<BString>	fCopyrights;
			BObjectList<BString>	fLicenses;

			BObjectList<BPackageProvision>	fProvisions;
			BObjectList<BPackageRequirement>	fRequirements;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_INFO_H_
