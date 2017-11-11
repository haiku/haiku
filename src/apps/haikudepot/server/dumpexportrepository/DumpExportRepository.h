/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-11-11T13:59:22.550365
 */

#ifndef GEN_JSON_SCHEMA_MODEL__DUMPEXPORTREPOSITORY_H
#define GEN_JSON_SCHEMA_MODEL__DUMPEXPORTREPOSITORY_H

#include "List.h"
#include "String.h"

#include "DumpExportRepositorySource.h"

class DumpExportRepository {
public:
    DumpExportRepository();
    virtual ~DumpExportRepository();
    
    BString* InformationUrl();
    void SetInformationUrl(BString* value);
    void SetInformationUrlNull();
    bool InformationUrlIsNull();

    BString* Code();
    void SetCode(BString* value);
    void SetCodeNull();
    bool CodeIsNull();

    void AddToRepositorySources(DumpExportRepositorySource* value);
    void SetRepositorySources(List<DumpExportRepositorySource*, true>* value);
    int32 CountRepositorySources();
    DumpExportRepositorySource* RepositorySourcesItemAt(int32 index);
    bool RepositorySourcesIsNull();

    BString* Name();
    void SetName(BString* value);
    void SetNameNull();
    bool NameIsNull();

    BString* Description();
    void SetDescription(BString* value);
    void SetDescriptionNull();
    bool DescriptionIsNull();

private:
    BString* fInformationUrl;
    BString* fCode;
    List <DumpExportRepositorySource*, true>* fRepositorySources;
    BString* fName;
    BString* fDescription;
};

#endif // GEN_JSON_SCHEMA_MODEL__DUMPEXPORTREPOSITORY_H