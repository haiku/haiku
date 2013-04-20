/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PACKAGE__PRIVATE__ACTIVATION_TRANSACTION_H_
#define _PACKAGE__PRIVATE__ACTIVATION_TRANSACTION_H_


#include <package/PackageDefs.h>
#include <StringList.h>


namespace BPackageKit {
namespace BPrivate {


class BActivationTransaction {
public:
								BActivationTransaction();
								~BActivationTransaction();

			status_t			SetTo(BPackageInstallationLocation location,
									int64 changeCount,
									const BString& directoryName);

			status_t			InitCheck() const;

			BPackageInstallationLocation Location() const;
			void				SetLocation(
									BPackageInstallationLocation location);

			int64				ChangeCount() const;
			void				SetChangeCount(int64 changeCount);

			const BString&		TransactionDirectoryName() const;
			void				SetTransactionDirectoryName(
									const BString& directoryName);

			const BStringList&	PackagesToActivate() const;
			bool				SetPackagesToActivate(
									const BStringList& packages);
			bool				AddPackageToActivate(const BString& package);

			const BStringList&	PackagesToDeactivate() const;
			bool				SetPackagesToDeactivate(
									const BStringList& packages);
			bool				AddPackageToDeactivate(const BString& package);

private:
			BPackageInstallationLocation fLocation;
			int64				fChangeCount;
			BString				fTransactionDirectoryName;
			BStringList			fPackagesToActivate;
			BStringList			fPackagesToDeactivate;
};


}	// namespace BPrivate
}	// namespace BPackageKit


#endif	// _PACKAGE__PRIVATE__ACTIVATION_TRANSACTION_H_
