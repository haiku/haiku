/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-12-07T23:22:33.022114
 */
#include "DumpExportRepositorySource.h"


DumpExportRepositorySource::DumpExportRepositorySource()
{
    fUrl = NULL;
    fRepoInfoUrl = NULL;
    fCode = NULL;
}


DumpExportRepositorySource::~DumpExportRepositorySource()
{
    if (fUrl != NULL) {
        delete fUrl;
    }

    if (fRepoInfoUrl != NULL) {
        delete fRepoInfoUrl;
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
DumpExportRepositorySource::RepoInfoUrl()
{
    return fRepoInfoUrl;
}


void
DumpExportRepositorySource::SetRepoInfoUrl(BString* value)
{
    fRepoInfoUrl = value;
}


void
DumpExportRepositorySource::SetRepoInfoUrlNull()
{
    if (!RepoInfoUrlIsNull()) {
        delete fRepoInfoUrl;
        fRepoInfoUrl = NULL;
    }
}


bool
DumpExportRepositorySource::RepoInfoUrlIsNull()
{
    return fRepoInfoUrl == NULL;
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

