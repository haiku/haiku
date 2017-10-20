/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-10-12T21:34:32.334783
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


bool
DumpExportRepositorySource::CodeIsNull()
{
    return fCode == NULL;
}

