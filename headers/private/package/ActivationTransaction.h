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
								BActivationTransaction(
									BPackageInstallationLocation location,
									int64 changeCount,
									const BString& directoryName,
									const BStringList& packagesToActivate,
									const BStringList& packagesToDeactivate);
								~BActivationTransaction();

			status_t			InitCheck() const;
			status_t			SetTo(BPackageInstallationLocation location,
									int64 changeCount,
									const BString& directoryName,
									const BStringList& packagesToActivate,
									const BStringList& packagesToDeactivate);

			BPackageInstallationLocation Location() const;
			void				SetLocation(
									BPackageInstallationLocation location);

			int64				ChangeCount() const;
			void				SetChangeCount(int64 changeCount);

			const BString&		TransactionDirectoryName() const;
			void				SetTransactionDirectoryName(
									const BString& directoryName);

			const BStringList&	PackagesToActivate() const;
			void				SetPackagesToActivate(
									const BStringList& packages);

			const BStringList&	PackagesToDeactivate() const;
			void				SetPackagesToDeactivate(
									const BStringList& packages);

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
