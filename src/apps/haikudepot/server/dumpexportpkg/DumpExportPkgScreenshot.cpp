/*
 * Generated Model Object
 * source json-schema : dumpexport.json
 * generated at : 2017-12-07T23:22:17.118221
 */
#include "DumpExportPkgScreenshot.h"


DumpExportPkgScreenshot::DumpExportPkgScreenshot()
{
    fOrdering = NULL;
    fWidth = NULL;
    fLength = NULL;
    fCode = NULL;
    fHeight = NULL;
}


DumpExportPkgScreenshot::~DumpExportPkgScreenshot()
{
    if (fOrdering != NULL) {
        delete fOrdering;
    }

    if (fWidth != NULL) {
        delete fWidth;
    }

    if (fLength != NULL) {
        delete fLength;
    }

    if (fCode != NULL) {
        delete fCode;
    }

    if (fHeight != NULL) {
        delete fHeight;
    }

}

int64
DumpExportPkgScreenshot::Ordering()
{
    return *fOrdering;
}


void
DumpExportPkgScreenshot::SetOrdering(int64 value)
{
    if (OrderingIsNull())
        fOrdering = new int64[1];
    fOrdering[0] = value;
}


void
DumpExportPkgScreenshot::SetOrderingNull()
{
    if (!OrderingIsNull()) {
        delete fOrdering;
        fOrdering = NULL;
    }
}


bool
DumpExportPkgScreenshot::OrderingIsNull()
{
    return fOrdering == NULL;
}


int64
DumpExportPkgScreenshot::Width()
{
    return *fWidth;
}


void
DumpExportPkgScreenshot::SetWidth(int64 value)
{
    if (WidthIsNull())
        fWidth = new int64[1];
    fWidth[0] = value;
}


void
DumpExportPkgScreenshot::SetWidthNull()
{
    if (!WidthIsNull()) {
        delete fWidth;
        fWidth = NULL;
    }
}


bool
DumpExportPkgScreenshot::WidthIsNull()
{
    return fWidth == NULL;
}


int64
DumpExportPkgScreenshot::Length()
{
    return *fLength;
}


void
DumpExportPkgScreenshot::SetLength(int64 value)
{
    if (LengthIsNull())
        fLength = new int64[1];
    fLength[0] = value;
}


void
DumpExportPkgScreenshot::SetLengthNull()
{
    if (!LengthIsNull()) {
        delete fLength;
        fLength = NULL;
    }
}


bool
DumpExportPkgScreenshot::LengthIsNull()
{
    return fLength == NULL;
}


BString*
DumpExportPkgScreenshot::Code()
{
    return fCode;
}


void
DumpExportPkgScreenshot::SetCode(BString* value)
{
    fCode = value;
}


void
DumpExportPkgScreenshot::SetCodeNull()
{
    if (!CodeIsNull()) {
        delete fCode;
        fCode = NULL;
    }
}


bool
DumpExportPkgScreenshot::CodeIsNull()
{
    return fCode == NULL;
}


int64
DumpExportPkgScreenshot::Height()
{
    return *fHeight;
}


void
DumpExportPkgScreenshot::SetHeight(int64 value)
{
    if (HeightIsNull())
        fHeight = new int64[1];
    fHeight[0] = value;
}


void
DumpExportPkgScreenshot::SetHeightNull()
{
    if (!HeightIsNull()) {
        delete fHeight;
        fHeight = NULL;
    }
}


bool
DumpExportPkgScreenshot::HeightIsNull()
{
    return fHeight == NULL;
}

