/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-11-11T13:59:22.559237
 */

#ifndef GEN_JSON_SCHEMA_MODEL__DUMPEXPORTREPOSITORYSOURCE_H
#define GEN_JSON_SCHEMA_MODEL__DUMPEXPORTREPOSITORYSOURCE_H

#include "List.h"
#include "String.h"


class DumpExportRepositorySource {
public:
    DumpExportRepositorySource();
    virtual ~DumpExportRepositorySource();
    
    BString* Url();
    void SetUrl(BString* value);
    void SetUrlNull();
    bool UrlIsNull();

    BString* Code();
    void SetCode(BString* value);
    void SetCodeNull();
    bool CodeIsNull();

private:
    BString* fUrl;
    BString* fCode;
};

#endif // GEN_JSON_SCHEMA_MODEL__DUMPEXPORTREPOSITORYSOURCE_H