/*
 * Generated Listener Object
 * source json-schema : dumpexport.json
 * generated at : 2017-11-11T14:09:03.498648
 */
#include "DumpExportPkgJsonListener.h"
#include "List.h"

#include <stdio.h>

// #pragma mark - private interfaces for the stacked listeners

        
/*! This class is the top level of the stacked listeners.  The stack structure
    is maintained in a linked list and sub-classes implement specific behaviors
    depending where in the parse tree the stacked listener is working at.
*/        
class AbstractStackedDumpExportPkgJsonListener : public BJsonEventListener {
public:
    AbstractStackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~AbstractStackedDumpExportPkgJsonListener();

    void HandleError(status_t status, int32 line, const char* message);
    void Complete();

    status_t ErrorStatus();

    AbstractStackedDumpExportPkgJsonListener* Parent();

    virtual void WillPop();

protected:
    AbstractMainDumpExportPkgJsonListener* fMainListener;

    void Pop();
    void Push(AbstractStackedDumpExportPkgJsonListener* stackedListener);
    
private:
    AbstractStackedDumpExportPkgJsonListener* fParent;
};

class GeneralArrayStackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    GeneralArrayStackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~GeneralArrayStackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
};

class GeneralObjectStackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    GeneralObjectStackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~GeneralObjectStackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
};

class DumpExportPkg_StackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    DumpExportPkg_StackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~DumpExportPkg_StackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    DumpExportPkg* Target();
    
protected:
    DumpExportPkg* fTarget;
    BString fNextItemName;
};

class DumpExportPkg_List_StackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    DumpExportPkg_List_StackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~DumpExportPkg_List_StackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    List<DumpExportPkg*, true>* Target(); // list of %s pointers
    
private:
    List<DumpExportPkg*, true>* fTarget;
};

class DumpExportPkgVersion_StackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    DumpExportPkgVersion_StackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~DumpExportPkgVersion_StackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    DumpExportPkgVersion* Target();
    
protected:
    DumpExportPkgVersion* fTarget;
    BString fNextItemName;
};

class DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    List<DumpExportPkgVersion*, true>* Target(); // list of %s pointers
    
private:
    List<DumpExportPkgVersion*, true>* fTarget;
};

class DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    DumpExportPkgScreenshot* Target();
    
protected:
    DumpExportPkgScreenshot* fTarget;
    BString fNextItemName;
};

class DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    List<DumpExportPkgScreenshot*, true>* Target(); // list of %s pointers
    
private:
    List<DumpExportPkgScreenshot*, true>* fTarget;
};

class DumpExportPkgCategory_StackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    DumpExportPkgCategory_StackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~DumpExportPkgCategory_StackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    DumpExportPkgCategory* Target();
    
protected:
    DumpExportPkgCategory* fTarget;
    BString fNextItemName;
};

class DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent);
    ~DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    List<DumpExportPkgCategory*, true>* Target(); // list of %s pointers
    
private:
    List<DumpExportPkgCategory*, true>* fTarget;
};

class ItemEmittingStackedDumpExportPkgJsonListener : public DumpExportPkg_StackedDumpExportPkgJsonListener {
public:
    ItemEmittingStackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent,
        DumpExportPkgListener* itemListener);
    ~ItemEmittingStackedDumpExportPkgJsonListener();
    
    void WillPop();
        
private:
    DumpExportPkgListener* fItemListener;
};


class BulkContainerStackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    BulkContainerStackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent,
        DumpExportPkgListener* itemListener);
    ~BulkContainerStackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
        
private:
    BString fNextItemName;
    DumpExportPkgListener* fItemListener;
};


class BulkContainerItemsStackedDumpExportPkgJsonListener : public AbstractStackedDumpExportPkgJsonListener {
public:
    BulkContainerItemsStackedDumpExportPkgJsonListener(
        AbstractMainDumpExportPkgJsonListener* mainListener,
        AbstractStackedDumpExportPkgJsonListener* parent,
        DumpExportPkgListener* itemListener);
    ~BulkContainerItemsStackedDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    void WillPop();
        
private:
    DumpExportPkgListener* fItemListener;
};
// #pragma mark - implementations for the stacked listeners


AbstractStackedDumpExportPkgJsonListener::AbstractStackedDumpExportPkgJsonListener (
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
{
    fMainListener = mainListener;
    fParent = parent;
}

AbstractStackedDumpExportPkgJsonListener::~AbstractStackedDumpExportPkgJsonListener()
{
}

void
AbstractStackedDumpExportPkgJsonListener::HandleError(status_t status, int32 line, const char* message)
{
    fMainListener->HandleError(status, line, message);
}

void
AbstractStackedDumpExportPkgJsonListener::Complete()
{
   fMainListener->Complete();
}

status_t
AbstractStackedDumpExportPkgJsonListener::ErrorStatus()
{
    return fMainListener->ErrorStatus();
}

AbstractStackedDumpExportPkgJsonListener*
AbstractStackedDumpExportPkgJsonListener::Parent()
{
    return fParent;
}

void
AbstractStackedDumpExportPkgJsonListener::Push(AbstractStackedDumpExportPkgJsonListener* stackedListener)
{
    fMainListener->SetStackedListener(stackedListener);
}

void
AbstractStackedDumpExportPkgJsonListener::WillPop()
{
}

void
AbstractStackedDumpExportPkgJsonListener::Pop()
{
    WillPop();
    fMainListener->SetStackedListener(fParent);
}

GeneralObjectStackedDumpExportPkgJsonListener::GeneralObjectStackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
}

GeneralObjectStackedDumpExportPkgJsonListener::~GeneralObjectStackedDumpExportPkgJsonListener()
{
}

bool
GeneralObjectStackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {

        case B_JSON_OBJECT_NAME:
        case B_JSON_NUMBER:
        case B_JSON_STRING:
        case B_JSON_TRUE:
        case B_JSON_FALSE:
        case B_JSON_NULL:
            // ignore
            break;
            
        case B_JSON_OBJECT_START:
            Push(new GeneralObjectStackedDumpExportPkgJsonListener(fMainListener, this));
            break;
            
        case B_JSON_ARRAY_START:
            Push(new GeneralArrayStackedDumpExportPkgJsonListener(fMainListener, this));
            break;
            
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected end of array");
            break;
            
        case B_JSON_OBJECT_END:
            Pop();
            delete this;
            break;
        
    }
    
    return ErrorStatus() == B_OK;
}

GeneralArrayStackedDumpExportPkgJsonListener::GeneralArrayStackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
}

GeneralArrayStackedDumpExportPkgJsonListener::~GeneralArrayStackedDumpExportPkgJsonListener()
{
}

bool
GeneralArrayStackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {

        case B_JSON_OBJECT_NAME:
        case B_JSON_NUMBER:
        case B_JSON_STRING:
        case B_JSON_TRUE:
        case B_JSON_FALSE:
        case B_JSON_NULL:
            // ignore
            break;
            
        case B_JSON_OBJECT_START:
            Push(new GeneralObjectStackedDumpExportPkgJsonListener(fMainListener, this));
            break;
            
        case B_JSON_ARRAY_START:
            Push(new GeneralArrayStackedDumpExportPkgJsonListener(fMainListener, this));
            break;
            
        case B_JSON_OBJECT_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected end of object");
            break;
            
        case B_JSON_ARRAY_END:
            Pop();
            delete this;
            break;
        
    }
    
    return ErrorStatus() == B_OK;
}

DumpExportPkg_StackedDumpExportPkgJsonListener::DumpExportPkg_StackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fTarget = new DumpExportPkg();
}


DumpExportPkg_StackedDumpExportPkgJsonListener::~DumpExportPkg_StackedDumpExportPkgJsonListener()
{
}


DumpExportPkg*
DumpExportPkg_StackedDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportPkg_StackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
            
        case B_JSON_OBJECT_END:
            Pop();
            delete this;
            break;

        case B_JSON_STRING:

            if (fNextItemName == "pkgChangelogContent")
                fTarget->SetPkgChangelogContent(new BString(event.Content()));
        
            if (fNextItemName == "name")
                fTarget->SetName(new BString(event.Content()));
                    fNextItemName.SetTo("");
            break;
        case B_JSON_TRUE:
            fNextItemName.SetTo("");
            break;
        case B_JSON_FALSE:
            fNextItemName.SetTo("");
            break;
        case B_JSON_NULL:
        {

            if (fNextItemName == "pkgChangelogContent")
                fTarget->SetPkgChangelogContentNull();
        
            if (fNextItemName == "name")
                fTarget->SetNameNull();
        
            if (fNextItemName == "derivedRating")
                fTarget->SetDerivedRatingNull();
        
            if (fNextItemName == "prominenceOrdering")
                fTarget->SetProminenceOrderingNull();
        
            if (fNextItemName == "modifyTimestamp")
                fTarget->SetModifyTimestampNull();
                    fNextItemName.SetTo("");
            break;
        }
        case B_JSON_NUMBER:
        {

            if (fNextItemName == "derivedRating")
                fTarget->SetDerivedRating(event.ContentDouble());
        
            if (fNextItemName == "prominenceOrdering")
                fTarget->SetProminenceOrdering(event.ContentInteger());
        
            if (fNextItemName == "modifyTimestamp")
                fTarget->SetModifyTimestamp(event.ContentInteger());
                    fNextItemName.SetTo("");
            break;
        }
        case B_JSON_OBJECT_START:
        {
            if (1 == 1) {
                GeneralObjectStackedDumpExportPkgJsonListener* nextListener = new GeneralObjectStackedDumpExportPkgJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_ARRAY_START:
        {
            if (fNextItemName == "pkgVersions") {
                DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener* nextListener = new DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener(fMainListener, this);
                fTarget->SetPkgVersions(nextListener->Target());
                Push(nextListener);
            }
            else if (fNextItemName == "pkgScreenshots") {
                DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener* nextListener = new DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener(fMainListener, this);
                fTarget->SetPkgScreenshots(nextListener->Target());
                Push(nextListener);
            }
            else if (fNextItemName == "pkgCategories") {
                DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener* nextListener = new DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener(fMainListener, this);
                fTarget->SetPkgCategories(nextListener->Target());
                Push(nextListener);
            }
            else if (1 == 1) {
                AbstractStackedDumpExportPkgJsonListener* nextListener = new GeneralArrayStackedDumpExportPkgJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }

    }
    
    return ErrorStatus() == B_OK;
}

DumpExportPkg_List_StackedDumpExportPkgJsonListener::DumpExportPkg_List_StackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fTarget = new List<DumpExportPkg*, true>();
}


DumpExportPkg_List_StackedDumpExportPkgJsonListener::~DumpExportPkg_List_StackedDumpExportPkgJsonListener()
{
}


List<DumpExportPkg*, true>*
DumpExportPkg_List_StackedDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportPkg_List_StackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            Pop();
            delete this;
            break;   
        
        case B_JSON_OBJECT_START:
        {
            DumpExportPkg_StackedDumpExportPkgJsonListener* nextListener =
                new DumpExportPkg_StackedDumpExportPkgJsonListener(fMainListener, this);
            fTarget->Add(nextListener->Target());
            Push(nextListener);
            break;
        }
            
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing an array of DumpExportPkg");
            break;
    }
    
    return ErrorStatus() == B_OK;
}

DumpExportPkgVersion_StackedDumpExportPkgJsonListener::DumpExportPkgVersion_StackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fTarget = new DumpExportPkgVersion();
}


DumpExportPkgVersion_StackedDumpExportPkgJsonListener::~DumpExportPkgVersion_StackedDumpExportPkgJsonListener()
{
}


DumpExportPkgVersion*
DumpExportPkgVersion_StackedDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportPkgVersion_StackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
            
        case B_JSON_OBJECT_END:
            Pop();
            delete this;
            break;

        case B_JSON_STRING:

            if (fNextItemName == "major")
                fTarget->SetMajor(new BString(event.Content()));
        
            if (fNextItemName == "description")
                fTarget->SetDescription(new BString(event.Content()));
        
            if (fNextItemName == "title")
                fTarget->SetTitle(new BString(event.Content()));
        
            if (fNextItemName == "summary")
                fTarget->SetSummary(new BString(event.Content()));
        
            if (fNextItemName == "micro")
                fTarget->SetMicro(new BString(event.Content()));
        
            if (fNextItemName == "preRelease")
                fTarget->SetPreRelease(new BString(event.Content()));
        
            if (fNextItemName == "architectureCode")
                fTarget->SetArchitectureCode(new BString(event.Content()));
        
            if (fNextItemName == "minor")
                fTarget->SetMinor(new BString(event.Content()));
                    fNextItemName.SetTo("");
            break;
        case B_JSON_TRUE:
            fNextItemName.SetTo("");
            break;
        case B_JSON_FALSE:
            fNextItemName.SetTo("");
            break;
        case B_JSON_NULL:
        {

            if (fNextItemName == "major")
                fTarget->SetMajorNull();
        
            if (fNextItemName == "payloadLength")
                fTarget->SetPayloadLengthNull();
        
            if (fNextItemName == "description")
                fTarget->SetDescriptionNull();
        
            if (fNextItemName == "title")
                fTarget->SetTitleNull();
        
            if (fNextItemName == "summary")
                fTarget->SetSummaryNull();
        
            if (fNextItemName == "micro")
                fTarget->SetMicroNull();
        
            if (fNextItemName == "preRelease")
                fTarget->SetPreReleaseNull();
        
            if (fNextItemName == "architectureCode")
                fTarget->SetArchitectureCodeNull();
        
            if (fNextItemName == "minor")
                fTarget->SetMinorNull();
        
            if (fNextItemName == "revision")
                fTarget->SetRevisionNull();
                    fNextItemName.SetTo("");
            break;
        }
        case B_JSON_NUMBER:
        {

            if (fNextItemName == "payloadLength")
                fTarget->SetPayloadLength(event.ContentInteger());
        
            if (fNextItemName == "revision")
                fTarget->SetRevision(event.ContentInteger());
                    fNextItemName.SetTo("");
            break;
        }
        case B_JSON_OBJECT_START:
        {
            if (1 == 1) {
                GeneralObjectStackedDumpExportPkgJsonListener* nextListener = new GeneralObjectStackedDumpExportPkgJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_ARRAY_START:
        {
            if (1 == 1) {
                AbstractStackedDumpExportPkgJsonListener* nextListener = new GeneralArrayStackedDumpExportPkgJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }

    }
    
    return ErrorStatus() == B_OK;
}

DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener::DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fTarget = new List<DumpExportPkgVersion*, true>();
}


DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener::~DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener()
{
}


List<DumpExportPkgVersion*, true>*
DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportPkgVersion_List_StackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            Pop();
            delete this;
            break;   
        
        case B_JSON_OBJECT_START:
        {
            DumpExportPkgVersion_StackedDumpExportPkgJsonListener* nextListener =
                new DumpExportPkgVersion_StackedDumpExportPkgJsonListener(fMainListener, this);
            fTarget->Add(nextListener->Target());
            Push(nextListener);
            break;
        }
            
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing an array of DumpExportPkgVersion");
            break;
    }
    
    return ErrorStatus() == B_OK;
}

DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener::DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fTarget = new DumpExportPkgScreenshot();
}


DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener::~DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener()
{
}


DumpExportPkgScreenshot*
DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
            
        case B_JSON_OBJECT_END:
            Pop();
            delete this;
            break;

        case B_JSON_STRING:

            if (fNextItemName == "code")
                fTarget->SetCode(new BString(event.Content()));
                    fNextItemName.SetTo("");
            break;
        case B_JSON_TRUE:
            fNextItemName.SetTo("");
            break;
        case B_JSON_FALSE:
            fNextItemName.SetTo("");
            break;
        case B_JSON_NULL:
        {

            if (fNextItemName == "ordering")
                fTarget->SetOrderingNull();
        
            if (fNextItemName == "width")
                fTarget->SetWidthNull();
        
            if (fNextItemName == "length")
                fTarget->SetLengthNull();
        
            if (fNextItemName == "code")
                fTarget->SetCodeNull();
        
            if (fNextItemName == "height")
                fTarget->SetHeightNull();
                    fNextItemName.SetTo("");
            break;
        }
        case B_JSON_NUMBER:
        {

            if (fNextItemName == "ordering")
                fTarget->SetOrdering(event.ContentInteger());
        
            if (fNextItemName == "width")
                fTarget->SetWidth(event.ContentInteger());
        
            if (fNextItemName == "length")
                fTarget->SetLength(event.ContentInteger());
        
            if (fNextItemName == "height")
                fTarget->SetHeight(event.ContentInteger());
                    fNextItemName.SetTo("");
            break;
        }
        case B_JSON_OBJECT_START:
        {
            if (1 == 1) {
                GeneralObjectStackedDumpExportPkgJsonListener* nextListener = new GeneralObjectStackedDumpExportPkgJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_ARRAY_START:
        {
            if (1 == 1) {
                AbstractStackedDumpExportPkgJsonListener* nextListener = new GeneralArrayStackedDumpExportPkgJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }

    }
    
    return ErrorStatus() == B_OK;
}

DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener::DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fTarget = new List<DumpExportPkgScreenshot*, true>();
}


DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener::~DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener()
{
}


List<DumpExportPkgScreenshot*, true>*
DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportPkgScreenshot_List_StackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            Pop();
            delete this;
            break;   
        
        case B_JSON_OBJECT_START:
        {
            DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener* nextListener =
                new DumpExportPkgScreenshot_StackedDumpExportPkgJsonListener(fMainListener, this);
            fTarget->Add(nextListener->Target());
            Push(nextListener);
            break;
        }
            
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing an array of DumpExportPkgScreenshot");
            break;
    }
    
    return ErrorStatus() == B_OK;
}

DumpExportPkgCategory_StackedDumpExportPkgJsonListener::DumpExportPkgCategory_StackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fTarget = new DumpExportPkgCategory();
}


DumpExportPkgCategory_StackedDumpExportPkgJsonListener::~DumpExportPkgCategory_StackedDumpExportPkgJsonListener()
{
}


DumpExportPkgCategory*
DumpExportPkgCategory_StackedDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportPkgCategory_StackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
            
        case B_JSON_OBJECT_END:
            Pop();
            delete this;
            break;

        case B_JSON_STRING:

            if (fNextItemName == "code")
                fTarget->SetCode(new BString(event.Content()));
                    fNextItemName.SetTo("");
            break;
        case B_JSON_TRUE:
            fNextItemName.SetTo("");
            break;
        case B_JSON_FALSE:
            fNextItemName.SetTo("");
            break;
        case B_JSON_NULL:
        {

            if (fNextItemName == "code")
                fTarget->SetCodeNull();
                    fNextItemName.SetTo("");
            break;
        }
        case B_JSON_NUMBER:
        {
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_OBJECT_START:
        {
            if (1 == 1) {
                GeneralObjectStackedDumpExportPkgJsonListener* nextListener = new GeneralObjectStackedDumpExportPkgJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_ARRAY_START:
        {
            if (1 == 1) {
                AbstractStackedDumpExportPkgJsonListener* nextListener = new GeneralArrayStackedDumpExportPkgJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }

    }
    
    return ErrorStatus() == B_OK;
}

DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener::DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener,
    AbstractStackedDumpExportPkgJsonListener* parent)
    :
    AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fTarget = new List<DumpExportPkgCategory*, true>();
}


DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener::~DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener()
{
}


List<DumpExportPkgCategory*, true>*
DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportPkgCategory_List_StackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            Pop();
            delete this;
            break;   
        
        case B_JSON_OBJECT_START:
        {
            DumpExportPkgCategory_StackedDumpExportPkgJsonListener* nextListener =
                new DumpExportPkgCategory_StackedDumpExportPkgJsonListener(fMainListener, this);
            fTarget->Add(nextListener->Target());
            Push(nextListener);
            break;
        }
            
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing an array of DumpExportPkgCategory");
            break;
    }
    
    return ErrorStatus() == B_OK;
}

ItemEmittingStackedDumpExportPkgJsonListener::ItemEmittingStackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener, AbstractStackedDumpExportPkgJsonListener* parent,
    DumpExportPkgListener* itemListener)
:
DumpExportPkg_StackedDumpExportPkgJsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


ItemEmittingStackedDumpExportPkgJsonListener::~ItemEmittingStackedDumpExportPkgJsonListener()
{
}


void
ItemEmittingStackedDumpExportPkgJsonListener::WillPop()
{
    fItemListener->Handle(fTarget);
    delete fTarget;
}


BulkContainerStackedDumpExportPkgJsonListener::BulkContainerStackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener, AbstractStackedDumpExportPkgJsonListener* parent,
    DumpExportPkgListener* itemListener)
:
AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


BulkContainerStackedDumpExportPkgJsonListener::~BulkContainerStackedDumpExportPkgJsonListener()
{
}


bool
BulkContainerStackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
            
        case B_JSON_OBJECT_START:
            Push(new GeneralObjectStackedDumpExportPkgJsonListener(fMainListener, this));
            break;
            
        case B_JSON_ARRAY_START:
            if (fNextItemName == "items")
                Push(new BulkContainerItemsStackedDumpExportPkgJsonListener(fMainListener, this, fItemListener));
            else
                Push(new GeneralArrayStackedDumpExportPkgJsonListener(fMainListener, this));
            break;
            
        case B_JSON_OBJECT_END:
            Pop();
            delete this;
            break;
            
        default:
                // ignore
            break;
    }
    
    return ErrorStatus() == B_OK;
}


BulkContainerItemsStackedDumpExportPkgJsonListener::BulkContainerItemsStackedDumpExportPkgJsonListener(
    AbstractMainDumpExportPkgJsonListener* mainListener, AbstractStackedDumpExportPkgJsonListener* parent,
    DumpExportPkgListener* itemListener)
:
AbstractStackedDumpExportPkgJsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


BulkContainerItemsStackedDumpExportPkgJsonListener::~BulkContainerItemsStackedDumpExportPkgJsonListener()
{
}


bool
BulkContainerItemsStackedDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_OBJECT_START:
            Push(new ItemEmittingStackedDumpExportPkgJsonListener(fMainListener, this, fItemListener));
            break;
            
        case B_JSON_ARRAY_END:
            Pop();
            delete this;
            break;
            
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected json event");
            break;
    }
    
    return ErrorStatus() == B_OK;
}


void
BulkContainerItemsStackedDumpExportPkgJsonListener::WillPop()
{
    fItemListener->Complete();
}


// #pragma mark - implementations for the main listeners


AbstractMainDumpExportPkgJsonListener::AbstractMainDumpExportPkgJsonListener()
{
    fStackedListener = NULL;
    fErrorStatus = B_OK;
}


AbstractMainDumpExportPkgJsonListener::~AbstractMainDumpExportPkgJsonListener()
{
}


void
AbstractMainDumpExportPkgJsonListener::HandleError(status_t status, int32 line, const char* message)
{
    if (message != NULL) {
        fprintf(stderr, "an error has arisen processing json for 'DumpExportPkg'; %s\n", message);
    } else {
        fprintf(stderr, "an error has arisen processing json for 'DumpExportPkg'\n");
    }
    fErrorStatus = status;
}


void
AbstractMainDumpExportPkgJsonListener::Complete()
{
}


status_t
AbstractMainDumpExportPkgJsonListener::ErrorStatus()
{
    return fErrorStatus;
}

void
AbstractMainDumpExportPkgJsonListener::SetStackedListener(
    AbstractStackedDumpExportPkgJsonListener* stackedListener)
{
    fStackedListener = stackedListener;
}


SingleDumpExportPkgJsonListener::SingleDumpExportPkgJsonListener()
:
AbstractMainDumpExportPkgJsonListener()
{
    fTarget = NULL;
}


SingleDumpExportPkgJsonListener::~SingleDumpExportPkgJsonListener()
{
}


bool
SingleDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    if (fErrorStatus != B_OK)
       return false;
       
    if (fStackedListener != NULL)
        return fStackedListener->Handle(event);
    
    switch (event.EventType()) {
        
        case B_JSON_OBJECT_START:
        {
            DumpExportPkg_StackedDumpExportPkgJsonListener* nextListener = new DumpExportPkg_StackedDumpExportPkgJsonListener(
                this, NULL);
            fTarget = nextListener->Target();
            SetStackedListener(nextListener);
            break;
        }
              
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing top level for DumpExportPkg");
            break;
    }
    
    return ErrorStatus() == B_OK;
}


DumpExportPkg*
SingleDumpExportPkgJsonListener::Target()
{
    return fTarget;
}


BulkContainerDumpExportPkgJsonListener::BulkContainerDumpExportPkgJsonListener(
    DumpExportPkgListener* itemListener) : AbstractMainDumpExportPkgJsonListener()
{
    fItemListener = itemListener;
}


BulkContainerDumpExportPkgJsonListener::~BulkContainerDumpExportPkgJsonListener()
{
}


bool
BulkContainerDumpExportPkgJsonListener::Handle(const BJsonEvent& event)
{
    if (fErrorStatus != B_OK)
       return false;
       
    if (fStackedListener != NULL)
        return fStackedListener->Handle(event);
    
    switch (event.EventType()) {
        
        case B_JSON_OBJECT_START:
        {
            BulkContainerStackedDumpExportPkgJsonListener* nextListener =
                new BulkContainerStackedDumpExportPkgJsonListener(
                    this, NULL, fItemListener);
            SetStackedListener(nextListener);
            return true;
            break;
        }
              
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing top level for BulkContainerDumpExportPkgJsonListener");
            break;
    }
    
    return ErrorStatus() == B_OK;
}

