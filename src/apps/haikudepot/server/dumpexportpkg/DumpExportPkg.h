/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-11-05T22:30:10.253178
 */

#ifndef GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKG_H
#define GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKG_H

#include "List.h"
#include "String.h"

#include "DumpExportPkgVersion.h"
#include "DumpExportPkgScreenshot.h"
#include "DumpExportPkgCategory.h"

class DumpExportPkg {
public:
    DumpExportPkg();
    virtual ~DumpExportPkg();
    
    BString* PkgChangelogContent();
    void SetPkgChangelogContent(BString* value);
    void SetPkgChangelogContentNull();
    bool PkgChangelogContentIsNull();

    BString* Name();
    void SetName(BString* value);
    void SetNameNull();
    bool NameIsNull();

    void AddToPkgVersions(DumpExportPkgVersion* value);
    void SetPkgVersions(List<DumpExportPkgVersion*, true>* value);
    int32 CountPkgVersions();
    DumpExportPkgVersion* PkgVersionsItemAt(int32 index);
    bool PkgVersionsIsNull();


    double DerivedRating();
    void SetDerivedRating(double value);
    void SetDerivedRatingNull();
    bool DerivedRatingIsNull();

    void AddToPkgScreenshots(DumpExportPkgScreenshot* value);
    void SetPkgScreenshots(List<DumpExportPkgScreenshot*, true>* value);
    int32 CountPkgScreenshots();
    DumpExportPkgScreenshot* PkgScreenshotsItemAt(int32 index);
    bool PkgScreenshotsIsNull();


    int64 ProminenceOrdering();
    void SetProminenceOrdering(int64 value);
    void SetProminenceOrderingNull();
    bool ProminenceOrderingIsNull();

    void AddToPkgCategories(DumpExportPkgCategory* value);
    void SetPkgCategories(List<DumpExportPkgCategory*, true>* value);
    int32 CountPkgCategories();
    DumpExportPkgCategory* PkgCategoriesItemAt(int32 index);
    bool PkgCategoriesIsNull();


    int64 ModifyTimestamp();
    void SetModifyTimestamp(int64 value);
    void SetModifyTimestampNull();
    bool ModifyTimestampIsNull();

private:
    BString* fPkgChangelogContent;
    BString* fName;
    List <DumpExportPkgVersion*, true>* fPkgVersions;
    double* fDerivedRating;
    List <DumpExportPkgScreenshot*, true>* fPkgScreenshots;
    int64* fProminenceOrdering;
    List <DumpExportPkgCategory*, true>* fPkgCategories;
    int64* fModifyTimestamp;
};

#endif // GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKG_H