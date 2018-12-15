/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LocalRepositoryUpdateProcess.h"

#include <Catalog.h>
#include <Roster.h>
#include <String.h>
#include <StringList.h>

#include <package/Context.h>
#include <package/manager/Exceptions.h>
#include <package/PackageRoster.h>
#include <package/RefreshRepositoryRequest.h>

#include "AppUtils.h"
#include "DecisionProvider.h"
#include "JobStateListener.h"
#include "Logger.h"
#include "HaikuDepotConstants.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LocalRepositoryUpdateProcess"


using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;


LocalRepositoryUpdateProcess::LocalRepositoryUpdateProcess(
	Model *model, bool force)
	:
	AbstractProcess(),
	fModel(model),
	fForce(force)
{
}


LocalRepositoryUpdateProcess::~LocalRepositoryUpdateProcess()
{
}


const char*
LocalRepositoryUpdateProcess::Name() const
{
	return "LocalRepositoryUpdateProcess";
}


const char*
LocalRepositoryUpdateProcess::Description() const
{
	return B_TRANSLATE("Fetching remote repository data");
}


status_t
LocalRepositoryUpdateProcess::RunInternal()
{
	BPackageRoster roster;
	BStringList repoNames;

	if (Logger::IsInfoEnabled()) {
		printf("[%s] will update local repositories\n", Name());
	}

	status_t result = roster.GetRepositoryNames(repoNames);

	if (result == B_OK) {
		DecisionProvider decisionProvider;
		JobStateListener listener;
		BContext context(decisionProvider, listener);
		BRepositoryCache cache;

		for (
			int32 i = 0;
			result == B_OK && i < repoNames.CountStrings() && !WasStopped();
			++i) {
			result = _RunForRepositoryName(repoNames.StringAt(i), context,
				roster, &cache);
		}
	} else {
		_NotifyError(strerror(result));
		result = B_ERROR;
	}

	if (result == B_OK && Logger::IsInfoEnabled()) {
		printf("[%s] did update %" B_PRIi32 " local repositories\n",
			Name(), repoNames.CountStrings());
	}

	return result;
}

status_t
LocalRepositoryUpdateProcess::_RunForRepositoryName(const BString& repoName,
	BPackageKit::BContext& context, BPackageKit::BPackageRoster& roster,
	BPackageKit::BRepositoryCache* cache)
{
	status_t result = B_ERROR;
	BRepositoryConfig repoConfig;
	result = roster.GetRepositoryConfig(repoName, &repoConfig);
	if (result == B_OK) {
		if (roster.GetRepositoryCache(repoName, cache) != B_OK || fForce) {
			try {
				BRefreshRepositoryRequest refreshRequest(context, repoConfig);
				result = refreshRequest.Process();
				result = B_OK;
			} catch (BFatalErrorException ex) {
				_NotifyError(ex.Message(), ex.Details());
			} catch (BException ex) {
				_NotifyError(ex.Message());
			}
		}
	} else {
		_NotifyError(strerror(result));
	}

	return result;
}


void
LocalRepositoryUpdateProcess::_NotifyError(const BString& error) const
{
	_NotifyError(error, "");
}


void
LocalRepositoryUpdateProcess::_NotifyError(const BString& error,
	const BString& details) const
{
	printf("an error has arisen updating the local repositories : %s\n",
		error.String());

	BString alertText(B_TRANSLATE("An error occurred while refreshing the "
		"repository: %error%"));
	alertText.ReplaceFirst("%error%", error);

	if (!details.IsEmpty()) {
		alertText.Append(" (");
		alertText.Append(details);
		alertText.Append(")");
	}

	AppUtils::NotifySimpleError(
		B_TRANSLATE("Repository Update Error"),
		alertText);
}