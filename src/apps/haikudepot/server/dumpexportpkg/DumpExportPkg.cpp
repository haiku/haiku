/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-12-07T23:22:17.116794
 */
#include "DumpExportPkg.h"


DumpExportPkg::DumpExportPkg()
{
    fPkgChangelogContent = NULL;
    fName = NULL;
    fPkgVersions = NULL;
    fDerivedRating = NULL;
    fPkgScreenshots = NULL;
    fProminenceOrdering = NULL;
    fPkgCategories = NULL;
    fModifyTimestamp = NULL;
}


DumpExportPkg::~DumpExportPkg()
{
    int32 i;

    if (fPkgChangelogContent != NULL) {
        delete fPkgChangelogContent;
    }

    if (fName != NULL) {
        delete fName;
    }

    if (fPkgVersions != NULL) {
        int32 count = fPkgVersions->CountItems(); 
        for (i = 0; i < count; i++)
            delete fPkgVersions->ItemAt(i);
        delete fPkgVersions;
    }

    if (fDerivedRating != NULL) {
        delete fDerivedRating;
    }

    if (fPkgScreenshots != NULL) {
        int32 count = fPkgScreenshots->CountItems(); 
        for (i = 0; i < count; i++)
            delete fPkgScreenshots->ItemAt(i);
        delete fPkgScreenshots;
    }

    if (fProminenceOrdering != NULL) {
        delete fProminenceOrdering;
    }

    if (fPkgCategories != NULL) {
        int32 count = fPkgCategories->CountItems(); 
        for (i = 0; i < count; i++)
            delete fPkgCategories->ItemAt(i);
        delete fPkgCategories;
    }

    if (fModifyTimestamp != NULL) {
        delete fModifyTimestamp;
    }

}

BString*
DumpExportPkg::PkgChangelogContent()
{
    return fPkgChangelogContent;
}


void
DumpExportPkg::SetPkgChangelogContent(BString* value)
{
    fPkgChangelogContent = value;
}


void
DumpExportPkg::SetPkgChangelogContentNull()
{
    if (!PkgChangelogContentIsNull()) {
        delete fPkgChangelogContent;
        fPkgChangelogContent = NULL;
    }
}


bool
DumpExportPkg::PkgChangelogContentIsNull()
{
    return fPkgChangelogContent == NULL;
}


BString*
DumpExportPkg::Name()
{
    return fName;
}


void
DumpExportPkg::SetName(BString* value)
{
    fName = value;
}


void
DumpExportPkg::SetNameNull()
{
    if (!NameIsNull()) {
        delete fName;
        fName = NULL;
    }
}


bool
DumpExportPkg::NameIsNull()
{
    return fName == NULL;
}


void
DumpExportPkg::AddToPkgVersions(DumpExportPkgVersion* value)
{
    if (fPkgVersions == NULL)
        fPkgVersions = new List<DumpExportPkgVersion*, true>();
    fPkgVersions->Add(value);
}


void
DumpExportPkg::SetPkgVersions(List<DumpExportPkgVersion*, true>* value)
{
   fPkgVersions = value; 
}


int32
DumpExportPkg::CountPkgVersions()
{
    if (fPkgVersions == NULL)
        return 0;
    return fPkgVersions->CountItems();
}


DumpExportPkgVersion*
DumpExportPkg::PkgVersionsItemAt(int32 index)
{
    return fPkgVersions->ItemAt(index);
}


bool
DumpExportPkg::PkgVersionsIsNull()
{
    return fPkgVersions == NULL;
}


double
DumpExportPkg::DerivedRating()
{
    return *fDerivedRating;
}


void
DumpExportPkg::SetDerivedRating(double value)
{
    if (DerivedRatingIsNull())
        fDerivedRating = new double[1];
    fDerivedRating[0] = value;
}


void
DumpExportPkg::SetDerivedRatingNull()
{
    if (!DerivedRatingIsNull()) {
        delete fDerivedRating;
        fDerivedRating = NULL;
    }
}


bool
DumpExportPkg::DerivedRatingIsNull()
{
    return fDerivedRating == NULL;
}


void
DumpExportPkg::AddToPkgScreenshots(DumpExportPkgScreenshot* value)
{
    if (fPkgScreenshots == NULL)
        fPkgScreenshots = new List<DumpExportPkgScreenshot*, true>();
    fPkgScreenshots->Add(value);
}


void
DumpExportPkg::SetPkgScreenshots(List<DumpExportPkgScreenshot*, true>* value)
{
   fPkgScreenshots = value; 
}


int32
DumpExportPkg::CountPkgScreenshots()
{
    if (fPkgScreenshots == NULL)
        return 0;
    return fPkgScreenshots->CountItems();
}


DumpExportPkgScreenshot*
DumpExportPkg::PkgScreenshotsItemAt(int32 index)
{
    return fPkgScreenshots->ItemAt(index);
}


bool
DumpExportPkg::PkgScreenshotsIsNull()
{
    return fPkgScreenshots == NULL;
}


int64
DumpExportPkg::ProminenceOrdering()
{
    return *fProminenceOrdering;
}


void
DumpExportPkg::SetProminenceOrdering(int64 value)
{
    if (ProminenceOrderingIsNull())
        fProminenceOrdering = new int64[1];
    fProminenceOrdering[0] = value;
}


void
DumpExportPkg::SetProminenceOrderingNull()
{
    if (!ProminenceOrderingIsNull()) {
        delete fProminenceOrdering;
        fProminenceOrdering = NULL;
    }
}


bool
DumpExportPkg::ProminenceOrderingIsNull()
{
    return fProminenceOrdering == NULL;
}


void
DumpExportPkg::AddToPkgCategories(DumpExportPkgCategory* value)
{
    if (fPkgCategories == NULL)
        fPkgCategories = new List<DumpExportPkgCategory*, true>();
    fPkgCategories->Add(value);
}


void
DumpExportPkg::SetPkgCategories(List<DumpExportPkgCategory*, true>* value)
{
   fPkgCategories = value; 
}


int32
DumpExportPkg::CountPkgCategories()
{
    if (fPkgCategories == NULL)
        return 0;
    return fPkgCategories->CountItems();
}


DumpExportPkgCategory*
DumpExportPkg::PkgCategoriesItemAt(int32 index)
{
    return fPkgCategories->ItemAt(index);
}


bool
DumpExportPkg::PkgCategoriesIsNull()
{
    return fPkgCategories == NULL;
}


int64
DumpExportPkg::ModifyTimestamp()
{
    return *fModifyTimestamp;
}


void
DumpExportPkg::SetModifyTimestamp(int64 value)
{
    if (ModifyTimestampIsNull())
        fModifyTimestamp = new int64[1];
    fModifyTimestamp[0] = value;
}


void
DumpExportPkg::SetModifyTimestampNull()
{
    if (!ModifyTimestampIsNull()) {
        delete fModifyTimestamp;
        fModifyTimestamp = NULL;
    }
}


bool
DumpExportPkg::ModifyTimestampIsNull()
{
    return fModifyTimestamp == NULL;
}

