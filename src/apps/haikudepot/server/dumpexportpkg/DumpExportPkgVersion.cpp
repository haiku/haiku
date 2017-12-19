/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-12-07T23:22:17.117543
 */
#include "DumpExportPkgVersion.h"


DumpExportPkgVersion::DumpExportPkgVersion()
{
    fMajor = NULL;
    fPayloadLength = NULL;
    fDescription = NULL;
    fTitle = NULL;
    fSummary = NULL;
    fMicro = NULL;
    fPreRelease = NULL;
    fArchitectureCode = NULL;
    fMinor = NULL;
    fRevision = NULL;
}


DumpExportPkgVersion::~DumpExportPkgVersion()
{
    if (fMajor != NULL) {
        delete fMajor;
    }

    if (fPayloadLength != NULL) {
        delete fPayloadLength;
    }

    if (fDescription != NULL) {
        delete fDescription;
    }

    if (fTitle != NULL) {
        delete fTitle;
    }

    if (fSummary != NULL) {
        delete fSummary;
    }

    if (fMicro != NULL) {
        delete fMicro;
    }

    if (fPreRelease != NULL) {
        delete fPreRelease;
    }

    if (fArchitectureCode != NULL) {
        delete fArchitectureCode;
    }

    if (fMinor != NULL) {
        delete fMinor;
    }

    if (fRevision != NULL) {
        delete fRevision;
    }

}

BString*
DumpExportPkgVersion::Major()
{
    return fMajor;
}


void
DumpExportPkgVersion::SetMajor(BString* value)
{
    fMajor = value;
}


void
DumpExportPkgVersion::SetMajorNull()
{
    if (!MajorIsNull()) {
        delete fMajor;
        fMajor = NULL;
    }
}


bool
DumpExportPkgVersion::MajorIsNull()
{
    return fMajor == NULL;
}


int64
DumpExportPkgVersion::PayloadLength()
{
    return *fPayloadLength;
}


void
DumpExportPkgVersion::SetPayloadLength(int64 value)
{
    if (PayloadLengthIsNull())
        fPayloadLength = new int64[1];
    fPayloadLength[0] = value;
}


void
DumpExportPkgVersion::SetPayloadLengthNull()
{
    if (!PayloadLengthIsNull()) {
        delete fPayloadLength;
        fPayloadLength = NULL;
    }
}


bool
DumpExportPkgVersion::PayloadLengthIsNull()
{
    return fPayloadLength == NULL;
}


BString*
DumpExportPkgVersion::Description()
{
    return fDescription;
}


void
DumpExportPkgVersion::SetDescription(BString* value)
{
    fDescription = value;
}


void
DumpExportPkgVersion::SetDescriptionNull()
{
    if (!DescriptionIsNull()) {
        delete fDescription;
        fDescription = NULL;
    }
}


bool
DumpExportPkgVersion::DescriptionIsNull()
{
    return fDescription == NULL;
}


BString*
DumpExportPkgVersion::Title()
{
    return fTitle;
}


void
DumpExportPkgVersion::SetTitle(BString* value)
{
    fTitle = value;
}


void
DumpExportPkgVersion::SetTitleNull()
{
    if (!TitleIsNull()) {
        delete fTitle;
        fTitle = NULL;
    }
}


bool
DumpExportPkgVersion::TitleIsNull()
{
    return fTitle == NULL;
}


BString*
DumpExportPkgVersion::Summary()
{
    return fSummary;
}


void
DumpExportPkgVersion::SetSummary(BString* value)
{
    fSummary = value;
}


void
DumpExportPkgVersion::SetSummaryNull()
{
    if (!SummaryIsNull()) {
        delete fSummary;
        fSummary = NULL;
    }
}


bool
DumpExportPkgVersion::SummaryIsNull()
{
    return fSummary == NULL;
}


BString*
DumpExportPkgVersion::Micro()
{
    return fMicro;
}


void
DumpExportPkgVersion::SetMicro(BString* value)
{
    fMicro = value;
}


void
DumpExportPkgVersion::SetMicroNull()
{
    if (!MicroIsNull()) {
        delete fMicro;
        fMicro = NULL;
    }
}


bool
DumpExportPkgVersion::MicroIsNull()
{
    return fMicro == NULL;
}


BString*
DumpExportPkgVersion::PreRelease()
{
    return fPreRelease;
}


void
DumpExportPkgVersion::SetPreRelease(BString* value)
{
    fPreRelease = value;
}


void
DumpExportPkgVersion::SetPreReleaseNull()
{
    if (!PreReleaseIsNull()) {
        delete fPreRelease;
        fPreRelease = NULL;
    }
}


bool
DumpExportPkgVersion::PreReleaseIsNull()
{
    return fPreRelease == NULL;
}


BString*
DumpExportPkgVersion::ArchitectureCode()
{
    return fArchitectureCode;
}


void
DumpExportPkgVersion::SetArchitectureCode(BString* value)
{
    fArchitectureCode = value;
}


void
DumpExportPkgVersion::SetArchitectureCodeNull()
{
    if (!ArchitectureCodeIsNull()) {
        delete fArchitectureCode;
        fArchitectureCode = NULL;
    }
}


bool
DumpExportPkgVersion::ArchitectureCodeIsNull()
{
    return fArchitectureCode == NULL;
}


BString*
DumpExportPkgVersion::Minor()
{
    return fMinor;
}


void
DumpExportPkgVersion::SetMinor(BString* value)
{
    fMinor = value;
}


void
DumpExportPkgVersion::SetMinorNull()
{
    if (!MinorIsNull()) {
        delete fMinor;
        fMinor = NULL;
    }
}


bool
DumpExportPkgVersion::MinorIsNull()
{
    return fMinor == NULL;
}


int64
DumpExportPkgVersion::Revision()
{
    return *fRevision;
}


void
DumpExportPkgVersion::SetRevision(int64 value)
{
    if (RevisionIsNull())
        fRevision = new int64[1];
    fRevision[0] = value;
}


void
DumpExportPkgVersion::SetRevisionNull()
{
    if (!RevisionIsNull()) {
        delete fRevision;
        fRevision = NULL;
    }
}


bool
DumpExportPkgVersion::RevisionIsNull()
{
    return fRevision == NULL;
}

