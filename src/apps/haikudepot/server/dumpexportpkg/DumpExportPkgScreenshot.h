/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-12-07T23:22:17.118049
 */

#ifndef GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGSCREENSHOT_H
#define GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGSCREENSHOT_H

#include "List.h"
#include "String.h"


class DumpExportPkgScreenshot {
public:
    DumpExportPkgScreenshot();
    virtual ~DumpExportPkgScreenshot();
    

    int64 Ordering();
    void SetOrdering(int64 value);
    void SetOrderingNull();
    bool OrderingIsNull();


    int64 Width();
    void SetWidth(int64 value);
    void SetWidthNull();
    bool WidthIsNull();


    int64 Length();
    void SetLength(int64 value);
    void SetLengthNull();
    bool LengthIsNull();

    BString* Code();
    void SetCode(BString* value);
    void SetCodeNull();
    bool CodeIsNull();


    int64 Height();
    void SetHeight(int64 value);
    void SetHeightNull();
    bool HeightIsNull();

private:
    int64* fOrdering;
    int64* fWidth;
    int64* fLength;
    BString* fCode;
    int64* fHeight;
};

#endif // GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGSCREENSHOT_H