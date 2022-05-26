/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_INFO_H_
#define _PACKAGE__PACKAGE_INFO_H_


#include <Archivable.h>
#include <ObjectList.h>
#include <String.h>
#include <StringList.h>

#include <package/GlobalWritableFileInfo.h>
#include <package/PackageArchitecture.h>
#include <package/PackageFlags.h>
#include <package/PackageInfoAttributes.h>
#include <package/PackageResolvable.h>
#include <package/PackageResolvableExpression.h>
#include <package/PackageVersion.h>
#include <package/User.h>
#include <package/UserSettingsFileInfo.h>


class BEntry;
class BFile;


namespace BPackageKit {


/*
 * Keeps a list of package info elements (e.g. name, version, vendor, ...) which
 * will be converted to package attributes when creating a package. Usually,
 * these elements have been parsed from a ".PackageInfo" file.
 * Alternatively, they can be read from an existing package file.
 */
class BPackageInfo : public BArchivable {
public:
			struct ParseErrorListener {
				virtual	~ParseErrorListener();
				virtual void OnError(const BString& msg, int line, int col) = 0;
			};

public:
								BPackageInfo();
								BPackageInfo(BMessage* archive,
									status_t* _error = NULL);
	virtual						~BPackageInfo();

			status_t			ReadFromConfigFile(
									const BEntry& packageInfoEntry,
									ParseErrorListener* listener = NULL);
			status_t			ReadFromConfigFile(
									BFile& packageInfoFile,
									ParseErrorListener* listener = NULL);
			status_t			ReadFromConfigString(
									const BString& packageInfoString,
									ParseErrorListener* listener = NULL);
			status_t			ReadFromPackageFile(const char* path);
			status_t			ReadFromPackageFile(int fd);

			status_t			InitCheck() const;

			const BString&		Name() const;
			const BString&		Summary() const;
			const BString&		Description() const;
			const BString&		Vendor() const;
			const BString&		Packager() const;
			const BString&		BasePackage() const;
			const BString&		Checksum() const;
			const BString&		InstallPath() const;
			BString				FileName() const;
									// the explicitly set file name, if any, or
									// CanonicalFileName() otherwise

			uint32				Flags() const;

			BPackageArchitecture	Architecture() const;
			const char*			ArchitectureName() const;
									// NULL, if invalid

			const BPackageVersion&	Version() const;

			const BStringList&	CopyrightList() const;
			const BStringList&	LicenseList() const;
			const BStringList&	URLList() const;
			const BStringList&	SourceURLList() const;

			const BObjectList<BGlobalWritableFileInfo>&
									GlobalWritableFileInfos() const;
			const BObjectList<BUserSettingsFileInfo>&
									UserSettingsFileInfos() const;

			const BObjectList<BUser>& Users() const;
			const BStringList&	Groups() const;

			const BStringList&	PostInstallScripts() const;
			const BStringList&	PreUninstallScripts() const;

			const BObjectList<BPackageResolvable>&	ProvidesList() const;
			const BObjectList<BPackageResolvableExpression>&
								RequiresList() const;
			const BObjectList<BPackageResolvableExpression>&
								SupplementsList() const;
			const BObjectList<BPackageResolvableExpression>&
								ConflictsList() const;
			const BObjectList<BPackageResolvableExpression>&
								FreshensList() const;
			const BStringList&	ReplacesList() const;

			BString				CanonicalFileName() const;

			bool				Matches(const BPackageResolvableExpression&
									expression) const;

			void				SetName(const BString& name);
			void				SetSummary(const BString& summary);
			void				SetDescription(const BString& description);
			void				SetVendor(const BString& vendor);
			void				SetPackager(const BString& packager);
			void				SetBasePackage(const BString& basePackage);
			void				SetChecksum(const BString& checksum);
			void				SetInstallPath(const BString& installPath);
			void				SetFileName(const BString& fileName);

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

			void				ClearGlobalWritableFileInfos();
			status_t			AddGlobalWritableFileInfo(
									const BGlobalWritableFileInfo& info);

			void				ClearUserSettingsFileInfos();
			status_t			AddUserSettingsFileInfo(
									const BUserSettingsFileInfo& info);

			void				ClearUsers();
			status_t			AddUser(const BUser& user);

			void				ClearGroups();
			status_t			AddGroup(const BString& group);

			void				ClearPostInstallScripts();
			status_t			AddPostInstallScript(const BString& path);

			void				ClearPreUninstallScripts();
			status_t			AddPreUninstallScript(const BString& path);

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

	virtual	status_t 			Archive(BMessage* archive,
									bool deep = true) const;
	static 	BArchivable*		Instantiate(BMessage* archive);

			status_t			GetConfigString(BString& _string) const;
			BString				ToString() const;

public:
	static	status_t			GetArchitectureByName(const BString& name,
									BPackageArchitecture& _architecture);

	static	status_t			ParseVersionString(const BString& string,
									bool revisionIsOptional,
									BPackageVersion& _version,
									ParseErrorListener* listener = NULL);
	static	status_t			ParseResolvableExpressionString(
									const BString& string,
									BPackageResolvableExpression& _expression,
									ParseErrorListener* listener = NULL);

public:
	static	const char*	const	kElementNames[];
	static	const char*	const	kArchitectureNames[];
	static	const char* const	kWritableFileUpdateTypes[];

private:
			class Parser;
			friend class Parser;
			struct StringBuilder;
			struct FieldName;
			struct PackageFileLocation;

			typedef BObjectList<BPackageResolvable> ResolvableList;
			typedef BObjectList<BPackageResolvableExpression>
				ResolvableExpressionList;

			typedef BObjectList<BGlobalWritableFileInfo>
				GlobalWritableFileInfoList;
			typedef BObjectList<BUserSettingsFileInfo>
				UserSettingsFileInfoList;

			typedef BObjectList<BUser> UserList;

private:
			status_t			_ReadFromPackageFile(
									const PackageFileLocation& fileLocation);

	static	status_t			_AddVersion(BMessage* archive,
									const char* field,
									const BPackageVersion& version);
	static	status_t			_AddResolvables(BMessage* archive,
									const char* field,
									const ResolvableList& resolvables);
	static	status_t			_AddResolvableExpressions(BMessage* archive,
									const char* field,
									const ResolvableExpressionList&
										expressions);
	static	status_t			_AddGlobalWritableFileInfos(BMessage* archive,
									const char* field,
									const GlobalWritableFileInfoList&
										infos);
	static	status_t			_AddUserSettingsFileInfos(BMessage* archive,
									const char* field,
									const UserSettingsFileInfoList&
										infos);
	static	status_t			_AddUsers(BMessage* archive, const char* field,
									const UserList& users);

	static	status_t			_ExtractVersion(BMessage* archive,
									const char* field, int32 index,
									BPackageVersion& _version);
	static	status_t			_ExtractStringList(BMessage* archive,
									const char* field, BStringList& _list);
	static	status_t			_ExtractResolvables(BMessage* archive,
									const char* field,
									ResolvableList& _resolvables);
	static	status_t			_ExtractResolvableExpressions(BMessage* archive,
									const char* field,
									ResolvableExpressionList& _expressions);
	static	status_t			_ExtractGlobalWritableFileInfos(
									BMessage* archive, const char* field,
									GlobalWritableFileInfoList& _infos);
	static	status_t			_ExtractUserSettingsFileInfos(
									BMessage* archive, const char* field,
									UserSettingsFileInfoList& _infos);
	static	status_t			_ExtractUsers(BMessage* archive,
									const char* field, UserList& _users);

private:
			BString				fName;
			BString				fSummary;
			BString				fDescription;
			BString				fVendor;
			BString				fPackager;
			BString				fBasePackage;

			uint32				fFlags;

			BPackageArchitecture fArchitecture;

			BPackageVersion		fVersion;

			BStringList			fCopyrightList;
			BStringList			fLicenseList;
			BStringList			fURLList;
			BStringList			fSourceURLList;

			BObjectList<BGlobalWritableFileInfo> fGlobalWritableFileInfos;
			BObjectList<BUserSettingsFileInfo> fUserSettingsFileInfos;

			UserList			fUsers;
			BStringList			fGroups;

			BStringList			fPostInstallScripts;
			BStringList			fPreUninstallScripts;

			ResolvableList		fProvidesList;

			ResolvableExpressionList fRequiresList;
			ResolvableExpressionList fSupplementsList;
			ResolvableExpressionList fConflictsList;
			ResolvableExpressionList fFreshensList;

			BStringList			fReplacesList;

			BString				fChecksum;
			BString				fInstallPath;
			BString				fFileName;
};


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_INFO_H_
