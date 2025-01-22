/*
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file included code earlier from `Model.cpp` and
 * copyrights have been latterly been carried across in 2024.
 */


#include "PopulatePkgUserRatingsFromServerProcess.h"

#include <Autolock.h>
#include <Catalog.h>

#include "Logger.h"
#include "PackageUtils.h"
#include "ServerHelper.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PopulatePkgUserRatingsFromServerProcess"


PopulatePkgUserRatingsFromServerProcess::PopulatePkgUserRatingsFromServerProcess(
	const BString& packageName, Model* model)
	:
	fModel(model),
	fPackageName(packageName)
{
}


PopulatePkgUserRatingsFromServerProcess::~PopulatePkgUserRatingsFromServerProcess()
{
}


const char*
PopulatePkgUserRatingsFromServerProcess::Name() const
{
	return "PopulatePkgUserRatingsFromServerProcess";
}


const char*
PopulatePkgUserRatingsFromServerProcess::Description() const
{
	return B_TRANSLATE("Fetching user ratings for package");
}


status_t
PopulatePkgUserRatingsFromServerProcess::RunInternal()
{
	// TODO; use API spec to code generation techniques instead of this manually written client.

	status_t status = B_OK;

	// Retrieve info from web-app
	BMessage info;
	BString webAppRepositoryCode = _WebAppRepositoryCode();

	if (webAppRepositoryCode.IsEmpty()) {
		HDERROR("unable to get the web app repository code for pkg [%s]", fPackageName.String());
		status = B_ERROR;
	}

	if (status == B_OK) {
		status = fModel->WebApp()->RetrieveUserRatingsForPackageForDisplay(fPackageName,
			webAppRepositoryCode, BString(), 0, PACKAGE_INFO_MAX_USER_RATINGS, info);
			// ^ note intentionally not using the repository source code as this would then show
			// too few results as it would be architecture specific.
	}

	PackageUserRatingInfoBuilder userRatingInfoBuilder;

	if (status == B_OK) {
		// Parse message
		BMessage result;
		BMessage items;

		status = info.FindMessage("result", &result);

		if (status == B_OK) {

			if (result.FindMessage("items", &items) == B_OK) {

				int32 index = 0;
				while (true) {
					BString name;
					name << index++;

					BMessage item;
					if (items.FindMessage(name, &item) != B_OK)
						break;

					BString code;
					if (item.FindString("code", &code) != B_OK) {
						HDERROR("corrupt user rating at index %" B_PRIi32, index);
						continue;
					}

					BString user;
					BMessage userInfo;
					if (item.FindMessage("user", &userInfo) != B_OK
						|| userInfo.FindString("nickname", &user) != B_OK) {
						HDERROR("ignored user rating [%s] without a user nickname", code.String());
						continue;
					}

					// Extract basic info, all items are optional
					BString languageCode;
					BString comment;
					double rating;
					item.FindString("naturalLanguageCode", &languageCode);
					item.FindString("comment", &comment);
					if (item.FindDouble("rating", &rating) != B_OK)
						rating = -1;
					if (comment.Length() == 0 && rating == -1) {
						HDERROR("rating [%s] has no comment or rating so will"
								" be ignored",
							code.String());
						continue;
					}

					// For which version of the package was the rating?
					BString major = "?";
					BString minor = "?";
					BString micro = "";
					double revision = -1;
					BString architectureCode = "";
					BMessage version;
					if (item.FindMessage("pkgVersion", &version) == B_OK) {
						version.FindString("major", &major);
						version.FindString("minor", &minor);
						version.FindString("micro", &micro);
						version.FindDouble("revision", &revision);
						version.FindString("architectureCode", &architectureCode);
					}
					BString versionString = major;
					versionString << ".";
					versionString << minor;
					if (!micro.IsEmpty()) {
						versionString << ".";
						versionString << micro;
					}
					if (revision > 0) {
						versionString << "-";
						versionString << (int)revision;
					}

					if (!architectureCode.IsEmpty()) {
						versionString << " " << STR_MDASH << " ";
						versionString << architectureCode;
					}

					double createTimestamp;
					item.FindDouble("createTimestamp", &createTimestamp);

					// Add the rating to the PackageInfo
					UserRatingRef userRating(
						new UserRating(UserInfo(user), rating, comment, languageCode,
							// note that language identifiers are "code" in HDS and "id" in Haiku
							versionString, static_cast<uint64>(createTimestamp)),
						true);
					userRatingInfoBuilder.AddUserRating(userRating);
					HDDEBUG("rating [%s] retrieved from server", code.String());
				}

				userRatingInfoBuilder.WithUserRatingsPopulated();
				HDDEBUG("did retrieve %" B_PRIi32 " user ratings for [%s]", index - 1,
					fPackageName.String());
			}
		} else {
			int32 errorCode = WebAppInterface::ErrorCodeFromResponse(info);

			if (errorCode != ERROR_CODE_NONE)
				ServerHelper::NotifyServerJsonRpcError(info);
		}
	} else {
		ServerHelper::NotifyTransportError(status);
	}

	// Now fetch the user rating summary which is derived separately as it is
	// not just based on the user-ratings downloaded; it is calculated according
	// to an algorithm. This is best executed server-side to avoid discrepancy.

	BMessage summaryResponse;

	if (status == B_OK) {
		status = fModel->WebApp()->RetrieveUserRatingSummaryForPackage(fPackageName,
			webAppRepositoryCode, summaryResponse);
	}

	if (status == B_OK) {
		// Parse message
		UserRatingSummaryBuilder userRatingSummaryBuilder;

		BMessage result;

		// TODO; this entire BMessage handling is historical and needs to be swapped out with
		//	generated code from the API spec; it just takes time unfortunately.

		status = summaryResponse.FindMessage("result", &result);

		double sampleSizeF;
		int sampleSize = 0;
		bool hasData;

		if (status == B_OK)
			status = result.FindDouble("sampleSize", &sampleSizeF);

		if (status == B_OK) {
			sampleSize = static_cast<int>(sampleSizeF);
			userRatingSummaryBuilder.WithRatingCount(sampleSize);
		}

		hasData = status == B_OK && sampleSize > 0;

		if (hasData) {
			double ratingF;

			if (status == B_OK)
				status = result.FindDouble("rating", &ratingF);

			if (status == B_OK)
				userRatingSummaryBuilder.WithAverageRating(ratingF);
		}

		if (hasData) {
			BMessage ratingDistributionItems;
			BMessage item;

			status = result.FindMessage("ratingDistribution", &ratingDistributionItems);

			int32 index = 0;
			while (status == B_OK) {
				BString name;
				name << index++;

				BMessage ratingDistributionItem;
				if (ratingDistributionItems.FindMessage(name, &ratingDistributionItem) != B_OK)
					break;

				double ratingDistributionRatingF;

				if (status == B_OK) {
					status
						= ratingDistributionItem.FindDouble("rating", &ratingDistributionRatingF);
				}

				double ratingDistributionTotalF;

				if (status == B_OK)
					status = ratingDistributionItem.FindDouble("total", &ratingDistributionTotalF);

				userRatingSummaryBuilder.AddRatingByStar(
					static_cast<int>(ratingDistributionRatingF),
					static_cast<int>(ratingDistributionTotalF));
			}

			userRatingInfoBuilder.WithSummary(userRatingSummaryBuilder.BuildRef());
		} else {
			int32 errorCode = WebAppInterface::ErrorCodeFromResponse(summaryResponse);

			if (errorCode != ERROR_CODE_NONE)
				ServerHelper::NotifyServerJsonRpcError(summaryResponse);
		}
	} else {
		ServerHelper::NotifyTransportError(status);
	}

	if (status == B_OK) {
		PackageInfoRef package = fModel->PackageForName(fPackageName);

		PackageInfoRef updatedPackage = PackageInfoBuilder(package)
											.WithUserRatingInfo(userRatingInfoBuilder.BuildRef())
											.BuildRef();

		fModel->AddPackage(updatedPackage);
	}

	return status;
}


const BString
PopulatePkgUserRatingsFromServerProcess::_WebAppRepositoryCode() const
{
	const PackageInfoRef package = fModel->PackageForName(fPackageName);
	const BString depotName = PackageUtils::DepotName(package);
	const DepotInfoRef depot = fModel->DepotForName(depotName);

	if (depot.IsSet())
		return depot->WebAppRepositoryCode();

	return BString();
}
