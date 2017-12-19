/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-12-07T23:22:17.118521
 */

#ifndef GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGCATEGORY_H
#define GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGCATEGORY_H

#include "List.h"
#include "String.h"


class DumpExportPkgCategory {
public:
    DumpExportPkgCategory();
    virtual ~DumpExportPkgCategory();
    
    BString* Code();
    void SetCode(BString* value);
    void SetCodeNull();
    bool CodeIsNull();

private:
    BString* fCode;
};

#endif // GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGCATEGORY_H