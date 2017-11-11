/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-11-05T22:30:10.255268
 */

#ifndef GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGVERSION_H
#define GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGVERSION_H

#include "List.h"
#include "String.h"


class DumpExportPkgVersion {
public:
    DumpExportPkgVersion();
    virtual ~DumpExportPkgVersion();
    
    BString* Major();
    void SetMajor(BString* value);
    void SetMajorNull();
    bool MajorIsNull();


    int64 PayloadLength();
    void SetPayloadLength(int64 value);
    void SetPayloadLengthNull();
    bool PayloadLengthIsNull();

    BString* Description();
    void SetDescription(BString* value);
    void SetDescriptionNull();
    bool DescriptionIsNull();

    BString* Title();
    void SetTitle(BString* value);
    void SetTitleNull();
    bool TitleIsNull();

    BString* Summary();
    void SetSummary(BString* value);
    void SetSummaryNull();
    bool SummaryIsNull();

    BString* Micro();
    void SetMicro(BString* value);
    void SetMicroNull();
    bool MicroIsNull();

    BString* PreRelease();
    void SetPreRelease(BString* value);
    void SetPreReleaseNull();
    bool PreReleaseIsNull();

    BString* ArchitectureCode();
    void SetArchitectureCode(BString* value);
    void SetArchitectureCodeNull();
    bool ArchitectureCodeIsNull();

    BString* Minor();
    void SetMinor(BString* value);
    void SetMinorNull();
    bool MinorIsNull();


    int64 Revision();
    void SetRevision(int64 value);
    void SetRevisionNull();
    bool RevisionIsNull();

private:
    BString* fMajor;
    int64* fPayloadLength;
    BString* fDescription;
    BString* fTitle;
    BString* fSummary;
    BString* fMicro;
    BString* fPreRelease;
    BString* fArchitectureCode;
    BString* fMinor;
    int64* fRevision;
};

#endif // GEN_JSON_SCHEMA_MODEL__DUMPEXPORTPKGVERSION_H