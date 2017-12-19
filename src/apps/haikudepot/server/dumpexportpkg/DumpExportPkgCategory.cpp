/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-12-07T23:22:17.118616
 */
#include "DumpExportPkgCategory.h"


DumpExportPkgCategory::DumpExportPkgCategory()
{
    fCode = NULL;
}


DumpExportPkgCategory::~DumpExportPkgCategory()
{
    if (fCode != NULL) {
        delete fCode;
    }

}

BString*
DumpExportPkgCategory::Code()
{
    return fCode;
}


void
DumpExportPkgCategory::SetCode(BString* value)
{
    fCode = value;
}


void
DumpExportPkgCategory::SetCodeNull()
{
    if (!CodeIsNull()) {
        delete fCode;
        fCode = NULL;
    }
}


bool
DumpExportPkgCategory::CodeIsNull()
{
    return fCode == NULL;
}

