/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-11-11T13:59:22.559632
 */
#include "DumpExportRepositorySource.h"


DumpExportRepositorySource::DumpExportRepositorySource()
{
    fUrl = NULL;
    fCode = NULL;
}


DumpExportRepositorySource::~DumpExportRepositorySource()
{
    if (fUrl != NULL) {
        delete fUrl;
    }

    if (fCode != NULL) {
        delete fCode;
    }

}

BString*
DumpExportRepositorySource::Url()
{
    return fUrl;
}


void
DumpExportRepositorySource::SetUrl(BString* value)
{
    fUrl = value;
}


void
DumpExportRepositorySource::SetUrlNull()
{
    if (!UrlIsNull()) {
        delete fUrl;
        fUrl = NULL;
    }
}


bool
DumpExportRepositorySource::UrlIsNull()
{
    return fUrl == NULL;
}


BString*
DumpExportRepositorySource::Code()
{
    return fCode;
}


void
DumpExportRepositorySource::SetCode(BString* value)
{
    fCode = value;
}


void
DumpExportRepositorySource::SetCodeNull()
{
    if (!CodeIsNull()) {
        delete fCode;
        fCode = NULL;
    }
}


bool
DumpExportRepositorySource::CodeIsNull()
{
    return fCode == NULL;
}

