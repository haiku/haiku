/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-11-11T13:59:22.553529
 */
#include "DumpExportRepository.h"


DumpExportRepository::DumpExportRepository()
{
    fInformationUrl = NULL;
    fCode = NULL;
    fRepositorySources = NULL;
    fName = NULL;
    fDescription = NULL;
}


DumpExportRepository::~DumpExportRepository()
{
    int32 i;

    if (fInformationUrl != NULL) {
        delete fInformationUrl;
    }

    if (fCode != NULL) {
        delete fCode;
    }

    if (fRepositorySources != NULL) {
        int32 count = fRepositorySources->CountItems(); 
        for (i = 0; i < count; i++)
            delete fRepositorySources->ItemAt(i);
        delete fRepositorySources;
    }

    if (fName != NULL) {
        delete fName;
    }

    if (fDescription != NULL) {
        delete fDescription;
    }

}

BString*
DumpExportRepository::InformationUrl()
{
    return fInformationUrl;
}


void
DumpExportRepository::SetInformationUrl(BString* value)
{
    fInformationUrl = value;
}


void
DumpExportRepository::SetInformationUrlNull()
{
    if (!InformationUrlIsNull()) {
        delete fInformationUrl;
        fInformationUrl = NULL;
    }
}


bool
DumpExportRepository::InformationUrlIsNull()
{
    return fInformationUrl == NULL;
}


BString*
DumpExportRepository::Code()
{
    return fCode;
}


void
DumpExportRepository::SetCode(BString* value)
{
    fCode = value;
}


void
DumpExportRepository::SetCodeNull()
{
    if (!CodeIsNull()) {
        delete fCode;
        fCode = NULL;
    }
}


bool
DumpExportRepository::CodeIsNull()
{
    return fCode == NULL;
}


void
DumpExportRepository::AddToRepositorySources(DumpExportRepositorySource* value)
{
    if (fRepositorySources == NULL)
        fRepositorySources = new List<DumpExportRepositorySource*, true>();
    fRepositorySources->Add(value);
}


void
DumpExportRepository::SetRepositorySources(List<DumpExportRepositorySource*, true>* value)
{
   fRepositorySources = value; 
}


int32
DumpExportRepository::CountRepositorySources()
{
    if (fRepositorySources == NULL)
        return 0;
    return fRepositorySources->CountItems();
}


DumpExportRepositorySource*
DumpExportRepository::RepositorySourcesItemAt(int32 index)
{
    return fRepositorySources->ItemAt(index);
}


bool
DumpExportRepository::RepositorySourcesIsNull()
{
    return fRepositorySources == NULL;
}


BString*
DumpExportRepository::Name()
{
    return fName;
}


void
DumpExportRepository::SetName(BString* value)
{
    fName = value;
}


void
DumpExportRepository::SetNameNull()
{
    if (!NameIsNull()) {
        delete fName;
        fName = NULL;
    }
}


bool
DumpExportRepository::NameIsNull()
{
    return fName == NULL;
}


BString*
DumpExportRepository::Description()
{
    return fDescription;
}


void
DumpExportRepository::SetDescription(BString* value)
{
    fDescription = value;
}


void
DumpExportRepository::SetDescriptionNull()
{
    if (!DescriptionIsNull()) {
        delete fDescription;
        fDescription = NULL;
    }
}


bool
DumpExportRepository::DescriptionIsNull()
{
    return fDescription == NULL;
}

