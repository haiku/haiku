/*
 * Generated Listener Object
 * source json-schema : dumpexport.json
 * generated at : 2017-10-02T22:02:18.621800
 */
#include "DumpExportRepositoryJsonListener.h"
#include "List.h"

#include <stdio.h>

// #pragma mark - private interfaces for the stacked listeners

        
/*! This class is the top level of the stacked listeners.  The stack structure
    is maintained in a linked list and sub-classes implement specific behaviors
    depending where in the parse tree the stacked listener is working at.
*/        
class AbstractStackedDumpExportRepositoryJsonListener : public BJsonEventListener {
public:
    AbstractStackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent);
    ~AbstractStackedDumpExportRepositoryJsonListener();

    void HandleError(status_t status, int32 line, const char* message);
    void Complete();

    status_t ErrorStatus();

    AbstractStackedDumpExportRepositoryJsonListener* Parent();

    virtual void WillPop();

protected:
    AbstractMainDumpExportRepositoryJsonListener* fMainListener;

    void Pop();
    void Push(AbstractStackedDumpExportRepositoryJsonListener* stackedListener);
    
private:
    AbstractStackedDumpExportRepositoryJsonListener* fParent;
};

class GeneralArrayStackedDumpExportRepositoryJsonListener : public AbstractStackedDumpExportRepositoryJsonListener {
public:
    GeneralArrayStackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent);
    ~GeneralArrayStackedDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
};

class GeneralObjectStackedDumpExportRepositoryJsonListener : public AbstractStackedDumpExportRepositoryJsonListener {
public:
    GeneralObjectStackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent);
    ~GeneralObjectStackedDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
};

class DumpExportRepository_StackedDumpExportRepositoryJsonListener : public AbstractStackedDumpExportRepositoryJsonListener {
public:
    DumpExportRepository_StackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent);
    ~DumpExportRepository_StackedDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    DumpExportRepository* Target();
    
protected:
    DumpExportRepository* fTarget;
    BString fNextItemName;
};

class DumpExportRepository_List_StackedDumpExportRepositoryJsonListener : public AbstractStackedDumpExportRepositoryJsonListener {
public:
    DumpExportRepository_List_StackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent);
    ~DumpExportRepository_List_StackedDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    List<DumpExportRepository*, true>* Target(); // list of %s pointers
    
private:
    List<DumpExportRepository*, true>* fTarget;
};

class DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener : public AbstractStackedDumpExportRepositoryJsonListener {
public:
    DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent);
    ~DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    DumpExportRepositorySource* Target();
    
protected:
    DumpExportRepositorySource* fTarget;
    BString fNextItemName;
};

class DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener : public AbstractStackedDumpExportRepositoryJsonListener {
public:
    DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent);
    ~DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
    List<DumpExportRepositorySource*, true>* Target(); // list of %s pointers
    
private:
    List<DumpExportRepositorySource*, true>* fTarget;
};

class ItemEmittingStackedDumpExportRepositoryJsonListener : public DumpExportRepository_StackedDumpExportRepositoryJsonListener {
public:
    ItemEmittingStackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent,
        DumpExportRepositoryListener* itemListener);
    ~ItemEmittingStackedDumpExportRepositoryJsonListener();
    
    void WillPop();
        
private:
    DumpExportRepositoryListener* fItemListener;
};


class BulkContainerStackedDumpExportRepositoryJsonListener : public AbstractStackedDumpExportRepositoryJsonListener {
public:
    BulkContainerStackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent,
        DumpExportRepositoryListener* itemListener);
    ~BulkContainerStackedDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
        
private:
    BString fNextItemName;
    DumpExportRepositoryListener* fItemListener;
};


class BulkContainerItemsStackedDumpExportRepositoryJsonListener : public AbstractStackedDumpExportRepositoryJsonListener {
public:
    BulkContainerItemsStackedDumpExportRepositoryJsonListener(
        AbstractMainDumpExportRepositoryJsonListener* mainListener,
        AbstractStackedDumpExportRepositoryJsonListener* parent,
        DumpExportRepositoryListener* itemListener);
    ~BulkContainerItemsStackedDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
    void WillPop();
        
private:
    DumpExportRepositoryListener* fItemListener;
};
// #pragma mark - implementations for the stacked listeners


AbstractStackedDumpExportRepositoryJsonListener::AbstractStackedDumpExportRepositoryJsonListener (
    AbstractMainDumpExportRepositoryJsonListener* mainListener,
    AbstractStackedDumpExportRepositoryJsonListener* parent)
{
    fMainListener = mainListener;
    fParent = parent;
}

AbstractStackedDumpExportRepositoryJsonListener::~AbstractStackedDumpExportRepositoryJsonListener()
{
}

void
AbstractStackedDumpExportRepositoryJsonListener::HandleError(status_t status, int32 line, const char* message)
{
    fMainListener->HandleError(status, line, message);
}

void
AbstractStackedDumpExportRepositoryJsonListener::Complete()
{
   fMainListener->Complete();
}

status_t
AbstractStackedDumpExportRepositoryJsonListener::ErrorStatus()
{
    return fMainListener->ErrorStatus();
}

AbstractStackedDumpExportRepositoryJsonListener*
AbstractStackedDumpExportRepositoryJsonListener::Parent()
{
    return fParent;
}

void
AbstractStackedDumpExportRepositoryJsonListener::Push(AbstractStackedDumpExportRepositoryJsonListener* stackedListener)
{
    fMainListener->SetStackedListener(stackedListener);
}

void
AbstractStackedDumpExportRepositoryJsonListener::WillPop()
{
}

void
AbstractStackedDumpExportRepositoryJsonListener::Pop()
{
    WillPop();
    fMainListener->SetStackedListener(fParent);
}

GeneralObjectStackedDumpExportRepositoryJsonListener::GeneralObjectStackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener,
    AbstractStackedDumpExportRepositoryJsonListener* parent)
    :
    AbstractStackedDumpExportRepositoryJsonListener(mainListener, parent)
{
}

GeneralObjectStackedDumpExportRepositoryJsonListener::~GeneralObjectStackedDumpExportRepositoryJsonListener()
{
}

bool
GeneralObjectStackedDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
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
            Push(new GeneralObjectStackedDumpExportRepositoryJsonListener(fMainListener, this));
            break;
            
        case B_JSON_ARRAY_START:
            Push(new GeneralArrayStackedDumpExportRepositoryJsonListener(fMainListener, this));
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

GeneralArrayStackedDumpExportRepositoryJsonListener::GeneralArrayStackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener,
    AbstractStackedDumpExportRepositoryJsonListener* parent)
    :
    AbstractStackedDumpExportRepositoryJsonListener(mainListener, parent)
{
}

GeneralArrayStackedDumpExportRepositoryJsonListener::~GeneralArrayStackedDumpExportRepositoryJsonListener()
{
}

bool
GeneralArrayStackedDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
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
            Push(new GeneralObjectStackedDumpExportRepositoryJsonListener(fMainListener, this));
            break;
            
        case B_JSON_ARRAY_START:
            Push(new GeneralArrayStackedDumpExportRepositoryJsonListener(fMainListener, this));
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

DumpExportRepository_StackedDumpExportRepositoryJsonListener::DumpExportRepository_StackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener,
    AbstractStackedDumpExportRepositoryJsonListener* parent)
    :
    AbstractStackedDumpExportRepositoryJsonListener(mainListener, parent)
{
    fTarget = new DumpExportRepository();
}


DumpExportRepository_StackedDumpExportRepositoryJsonListener::~DumpExportRepository_StackedDumpExportRepositoryJsonListener()
{
}


DumpExportRepository*
DumpExportRepository_StackedDumpExportRepositoryJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportRepository_StackedDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
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

            if (fNextItemName == "informationUrl")
                fTarget->SetInformationUrl(new BString(event.Content()));
        
            if (fNextItemName == "code")
                fTarget->SetCode(new BString(event.Content()));
        
            if (fNextItemName == "name")
                fTarget->SetName(new BString(event.Content()));
        
            if (fNextItemName == "description")
                fTarget->SetDescription(new BString(event.Content()));
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

            if (fNextItemName == "informationUrl")
                fTarget->SetInformationUrl(NULL);
        
            if (fNextItemName == "code")
                fTarget->SetCode(NULL);
        
            if (fNextItemName == "name")
                fTarget->SetName(NULL);
        
            if (fNextItemName == "description")
                fTarget->SetDescription(NULL);
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
                GeneralObjectStackedDumpExportRepositoryJsonListener* nextListener = new GeneralObjectStackedDumpExportRepositoryJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_ARRAY_START:
        {
            if (fNextItemName == "repositorySources") {
                DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener* nextListener = new DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener(fMainListener, this);
                fTarget->SetRepositorySources(nextListener->Target());
                Push(nextListener);
            }
            else if (1 == 1) {
                AbstractStackedDumpExportRepositoryJsonListener* nextListener = new GeneralArrayStackedDumpExportRepositoryJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }

    }
    
    return ErrorStatus() == B_OK;
}

DumpExportRepository_List_StackedDumpExportRepositoryJsonListener::DumpExportRepository_List_StackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener,
    AbstractStackedDumpExportRepositoryJsonListener* parent)
    :
    AbstractStackedDumpExportRepositoryJsonListener(mainListener, parent)
{
    fTarget = new List<DumpExportRepository*, true>();
}


DumpExportRepository_List_StackedDumpExportRepositoryJsonListener::~DumpExportRepository_List_StackedDumpExportRepositoryJsonListener()
{
}


List<DumpExportRepository*, true>*
DumpExportRepository_List_StackedDumpExportRepositoryJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportRepository_List_StackedDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            Pop();
            delete this;
            break;   
        
        case B_JSON_OBJECT_START:
        {
            DumpExportRepository_StackedDumpExportRepositoryJsonListener* nextListener =
                new DumpExportRepository_StackedDumpExportRepositoryJsonListener(fMainListener, this);
            fTarget->Add(nextListener->Target());
            Push(nextListener);
            break;
        }
            
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing an array of DumpExportRepository");
            break;
    }
    
    return ErrorStatus() == B_OK;
}

DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener::DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener,
    AbstractStackedDumpExportRepositoryJsonListener* parent)
    :
    AbstractStackedDumpExportRepositoryJsonListener(mainListener, parent)
{
    fTarget = new DumpExportRepositorySource();
}


DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener::~DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener()
{
}


DumpExportRepositorySource*
DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
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

            if (fNextItemName == "url")
                fTarget->SetUrl(new BString(event.Content()));
        
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

            if (fNextItemName == "url")
                fTarget->SetUrl(NULL);
        
            if (fNextItemName == "code")
                fTarget->SetCode(NULL);
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
                GeneralObjectStackedDumpExportRepositoryJsonListener* nextListener = new GeneralObjectStackedDumpExportRepositoryJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_ARRAY_START:
        {
            if (1 == 1) {
                AbstractStackedDumpExportRepositoryJsonListener* nextListener = new GeneralArrayStackedDumpExportRepositoryJsonListener(fMainListener, this);
                Push(nextListener);
            }
            fNextItemName.SetTo("");
            break;
        }

    }
    
    return ErrorStatus() == B_OK;
}

DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener::DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener,
    AbstractStackedDumpExportRepositoryJsonListener* parent)
    :
    AbstractStackedDumpExportRepositoryJsonListener(mainListener, parent)
{
    fTarget = new List<DumpExportRepositorySource*, true>();
}


DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener::~DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener()
{
}


List<DumpExportRepositorySource*, true>*
DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener::Target()
{
    return fTarget;
}


bool
DumpExportRepositorySource_List_StackedDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            Pop();
            delete this;
            break;   
        
        case B_JSON_OBJECT_START:
        {
            DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener* nextListener =
                new DumpExportRepositorySource_StackedDumpExportRepositoryJsonListener(fMainListener, this);
            fTarget->Add(nextListener->Target());
            Push(nextListener);
            break;
        }
            
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing an array of DumpExportRepositorySource");
            break;
    }
    
    return ErrorStatus() == B_OK;
}

ItemEmittingStackedDumpExportRepositoryJsonListener::ItemEmittingStackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener, AbstractStackedDumpExportRepositoryJsonListener* parent,
    DumpExportRepositoryListener* itemListener)
:
DumpExportRepository_StackedDumpExportRepositoryJsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


ItemEmittingStackedDumpExportRepositoryJsonListener::~ItemEmittingStackedDumpExportRepositoryJsonListener()
{
}


void
ItemEmittingStackedDumpExportRepositoryJsonListener::WillPop()
{
    fItemListener->Handle(fTarget);
    delete fTarget;
}


BulkContainerStackedDumpExportRepositoryJsonListener::BulkContainerStackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener, AbstractStackedDumpExportRepositoryJsonListener* parent,
    DumpExportRepositoryListener* itemListener)
:
AbstractStackedDumpExportRepositoryJsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


BulkContainerStackedDumpExportRepositoryJsonListener::~BulkContainerStackedDumpExportRepositoryJsonListener()
{
}


bool
BulkContainerStackedDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
            
        case B_JSON_OBJECT_START:
            Push(new GeneralObjectStackedDumpExportRepositoryJsonListener(fMainListener, this));
            break;
            
        case B_JSON_ARRAY_START:
            if (fNextItemName == "items")
                Push(new BulkContainerItemsStackedDumpExportRepositoryJsonListener(fMainListener, this, fItemListener));
            else
                Push(new GeneralArrayStackedDumpExportRepositoryJsonListener(fMainListener, this));
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


BulkContainerItemsStackedDumpExportRepositoryJsonListener::BulkContainerItemsStackedDumpExportRepositoryJsonListener(
    AbstractMainDumpExportRepositoryJsonListener* mainListener, AbstractStackedDumpExportRepositoryJsonListener* parent,
    DumpExportRepositoryListener* itemListener)
:
AbstractStackedDumpExportRepositoryJsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


BulkContainerItemsStackedDumpExportRepositoryJsonListener::~BulkContainerItemsStackedDumpExportRepositoryJsonListener()
{
}


bool
BulkContainerItemsStackedDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        
        case B_JSON_OBJECT_START:
            Push(new ItemEmittingStackedDumpExportRepositoryJsonListener(fMainListener, this, fItemListener));
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
BulkContainerItemsStackedDumpExportRepositoryJsonListener::WillPop()
{
    fItemListener->Complete();
}


// #pragma mark - implementations for the main listeners


AbstractMainDumpExportRepositoryJsonListener::AbstractMainDumpExportRepositoryJsonListener()
{
    fStackedListener = NULL;
    fErrorStatus = B_OK;
}


AbstractMainDumpExportRepositoryJsonListener::~AbstractMainDumpExportRepositoryJsonListener()
{
}


void
AbstractMainDumpExportRepositoryJsonListener::HandleError(status_t status, int32 line, const char* message)
{
    fprintf(stderr, "an error has arisen processing json for 'DumpExportRepository'; %s", message);
    fErrorStatus = status;
}


void
AbstractMainDumpExportRepositoryJsonListener::Complete()
{
}


status_t
AbstractMainDumpExportRepositoryJsonListener::ErrorStatus()
{
    return fErrorStatus;
}

void
AbstractMainDumpExportRepositoryJsonListener::SetStackedListener(
    AbstractStackedDumpExportRepositoryJsonListener* stackedListener)
{
    fStackedListener = stackedListener;
}


SingleDumpExportRepositoryJsonListener::SingleDumpExportRepositoryJsonListener()
:
AbstractMainDumpExportRepositoryJsonListener()
{
    fTarget = NULL;
}


SingleDumpExportRepositoryJsonListener::~SingleDumpExportRepositoryJsonListener()
{
}


bool
SingleDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
{
    if (fErrorStatus != B_OK)
       return false;
       
    if (fStackedListener != NULL)
        return fStackedListener->Handle(event);
    
    switch (event.EventType()) {
        
        case B_JSON_OBJECT_START:
        {
            DumpExportRepository_StackedDumpExportRepositoryJsonListener* nextListener = new DumpExportRepository_StackedDumpExportRepositoryJsonListener(
                this, NULL);
            fTarget = nextListener->Target();
            SetStackedListener(nextListener);
            break;
        }
              
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing top level for DumpExportRepository");
            break;
    }
    
    return ErrorStatus() == B_OK;
}


DumpExportRepository*
SingleDumpExportRepositoryJsonListener::Target()
{
    return fTarget;
}


BulkContainerDumpExportRepositoryJsonListener::BulkContainerDumpExportRepositoryJsonListener(
    DumpExportRepositoryListener* itemListener) : AbstractMainDumpExportRepositoryJsonListener()
{
    fItemListener = itemListener;
}


BulkContainerDumpExportRepositoryJsonListener::~BulkContainerDumpExportRepositoryJsonListener()
{
}


bool
BulkContainerDumpExportRepositoryJsonListener::Handle(const BJsonEvent& event)
{
    if (fErrorStatus != B_OK)
       return false;
       
    if (fStackedListener != NULL)
        return fStackedListener->Handle(event);
    
    switch (event.EventType()) {
        
        case B_JSON_OBJECT_START:
        {
            BulkContainerStackedDumpExportRepositoryJsonListener* nextListener =
                new BulkContainerStackedDumpExportRepositoryJsonListener(
                    this, NULL, fItemListener);
            SetStackedListener(nextListener);
            return true;
            break;
        }
              
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing top level for BulkContainerDumpExportRepositoryJsonListener");
            break;
    }
    
    return ErrorStatus() == B_OK;
}

