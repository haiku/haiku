/*
 * Copyright 2019-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ServerReferenceDataUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Catalog.h>
#include <FileIO.h>
#include <Url.h>

#include "DumpExportReference.h"
#include "DumpExportReferenceCountry.h"
#include "DumpExportReferenceJsonListener.h"
#include "DumpExportReferenceNaturalLanguage.h"
#include "DumpExportReferencePkgCategory.h"
#include "DumpExportReferenceUserRatingStability.h"
#include "LocaleUtils.h"
#include "Logger.h"
#include "ServerSettings.h"
#include "StorageUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerReferenceDataUpdateProcess"


ServerReferenceDataUpdateProcess::ServerReferenceDataUpdateProcess(Model* model,
	uint32 serverProcessOptions)
	:
	AbstractSingleFileServerProcess(serverProcessOptions),
	fModel(model)
{
}


ServerReferenceDataUpdateProcess::~ServerReferenceDataUpdateProcess()
{
}


const char*
ServerReferenceDataUpdateProcess::Name() const
{
	return "ServerReferenceDataUpdateProcess";
}


const char*
ServerReferenceDataUpdateProcess::Description() const
{
	return B_TRANSLATE("Synchronizing reference data from server");
}


BString
ServerReferenceDataUpdateProcess::UrlPathComponent()
{
	BString result;
	result.SetToFormat("/__reference/all-%s.json.gz", fModel->PreferredLanguage()->ID());
	return result;
}


status_t
ServerReferenceDataUpdateProcess::GetLocalPath(BPath& path) const
{
	return StorageUtils::DumpExportReferenceDataPath(path, fModel->PreferredLanguage());
}


status_t
ServerReferenceDataUpdateProcess::ProcessLocalData()
{
	SingleDumpExportReferenceJsonListener* listener = new SingleDumpExportReferenceJsonListener();

	BPath localPath;
	status_t result = GetLocalPath(localPath);

	if (result != B_OK)
		return result;

	result = ParseJsonFromFileWithListener(listener, localPath);

	if (result != B_OK)
		return result;

	result = listener->ErrorStatus();

	if (result != B_OK)
		return result;

	return _ProcessData(listener->Target());
}


status_t
ServerReferenceDataUpdateProcess::_ProcessData(DumpExportReference* data)
{
	status_t result = B_OK;
	if (result == B_OK)
		result = _ProcessNaturalLanguages(data);
	if (result == B_OK)
		result = _ProcessPkgCategories(data);
	if (result == B_OK)
		result = _ProcessRatingStabilities(data);
	return result;
}


status_t
ServerReferenceDataUpdateProcess::_ProcessNaturalLanguages(DumpExportReference* data)
{
	HDINFO("[%s] will populate from %" B_PRId32 " possible natural languages", Name(),
		data->CountNaturalLanguages());

	std::vector<LanguageRef> languages;
	int32 count = 0;

	for (int32 i = data->CountNaturalLanguages() - 1; i > 0; i--) {
		DumpExportReferenceNaturalLanguage* naturalLanguage = data->NaturalLanguagesItemAt(i);
		Language* language = new Language(*(naturalLanguage->Code()), *(naturalLanguage->Name()),
			naturalLanguage->IsPopular());
		languages.push_back(LanguageRef(language, true));
		count++;
	}

	if (!languages.empty()) {
		fModel->SetLanguagesAndPreferred(languages, LocaleUtils::DeriveDefaultLanguage(languages));
		HDINFO("[%s] did add %" B_PRId32 " supported languages", Name(), count);
	}

	return B_OK;
}


status_t
ServerReferenceDataUpdateProcess::_ProcessPkgCategories(DumpExportReference* data)
{
	HDINFO("[%s] will populate %" B_PRId32 " pkg categories", Name(), data->CountPkgCategories());

	std::vector<CategoryRef> assembledCategories;

	for (int32 i = 0; i < data->CountPkgCategories(); i++) {
		DumpExportReferencePkgCategory* pkgCategory = data->PkgCategoriesItemAt(i);
		assembledCategories.push_back(
			CategoryRef(new PackageCategory(*(pkgCategory->Code()), *(pkgCategory->Name())), true));
	}

	fModel->SetCategories(assembledCategories);

	return B_OK;
}


status_t
ServerReferenceDataUpdateProcess::_ProcessRatingStabilities(DumpExportReference* data)
{
	HDINFO("[%s] will populate %" B_PRId32 " rating stabilities", Name(),
		data->CountUserRatingStabilities());

	std::vector<RatingStabilityRef> assembledRatingStabilities;

	for (int32 i = 0; i < data->CountUserRatingStabilities(); i++) {
		DumpExportReferenceUserRatingStability* ratingStability
			= data->UserRatingStabilitiesItemAt(i);
		assembledRatingStabilities.push_back(
			RatingStabilityRef(new RatingStability(*(ratingStability->Code()),
								   *(ratingStability->Name()), ratingStability->Ordering()),
				true));
	}

	fModel->SetRatingStabilities(assembledRatingStabilities);

	return B_OK;
}


status_t
ServerReferenceDataUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	return GetLocalPath(path);
}


void
ServerReferenceDataUpdateProcess::GetStandardMetaDataJsonPath(BString& jsonPath) const
{
	jsonPath.SetTo("$.info");
}
