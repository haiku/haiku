#!/usr/bin/python

# =====================================
# Copyright 2017-2023, Andrew Lindesay
# Distributed under the terms of the MIT License.
# =====================================

# This simple tool will read a JSON schema and will then generate a
# listener for use the 'BJson' class in the Haiku system.  This
# allows data conforming to the schema to be able to be parsed.

import string
import json
import argparse
import os
import ustache
import hdsjsonschemacommon

HEADER_TEMPLATE = """
/*
 * Generated Listener Object for {{toplevel.cppname}}
 */

#ifndef GEN_JSON_SCHEMA_PARSER__{{toplevel.cppnameupper}}_H
#define GEN_JSON_SCHEMA_PARSER__{{toplevel.cppnameupper}}_H
            
#include <JsonEventListener.h>
#include <ObjectList.h>

#include "{{toplevel.cppname}}.h"

class AbstractStacked{{toplevel.cppname}}JsonListener;

class AbstractMain{{toplevel.cppname}}JsonListener : public BJsonEventListener {
friend class AbstractStacked{{toplevel.cppname}}JsonListener;
public:
    AbstractMain{{toplevel.cppname}}JsonListener();
    virtual ~AbstractMain{{toplevel.cppname}}JsonListener();

    void HandleError(status_t status, int32 line, const char* message);
    void Complete();
    status_t ErrorStatus();

protected:
    void SetStackedListener(
        AbstractStacked{{toplevel.cppname}}JsonListener* listener);
    status_t fErrorStatus;
    AbstractStacked{{toplevel.cppname}}JsonListener* fStackedListener;
};

/*! Use this listener when you want to parse some JSON data that contains
    just a single instance of {{toplevel.cppname}}.
*/
class Single{{toplevel.cppname}}JsonListener
    : public AbstractMain{{toplevel.cppname}}JsonListener {
friend class AbstractStacked{{toplevel.cppname}}JsonListener;
public:
    Single{{toplevel.cppname}}JsonListener();
    virtual ~Single{{toplevel.cppname}}JsonListener();

    bool Handle(const BJsonEvent& event);
    {{toplevel.cppname}}* Target();

private:
    {{toplevel.cppname}}* fTarget;
};

/*! Concrete sub-classes of this class are able to respond to each
    {{toplevel.cppname}}* instance as
    it is parsed from the bulk container.  When the stream is
    finished, the Complete() method is invoked.

    Note that the item object will be deleted after the Handle method
    is invoked.  The Handle method need not take responsibility
    for deleting the item itself.
*/         
class {{toplevel.cppname}}Listener {
public:
    virtual bool Handle({{toplevel.cppname}}* item) = 0;
    virtual void Complete() = 0;
};

/*! Use this listener, together with an instance of a concrete
    subclass of {{toplevel.cppname}}Listener
    in order to parse the JSON data in a specific "bulk
    container" format.  Each time that an instance of
    {{toplevel.cppname}}
    is parsed, the instance item listener will be invoked.
*/ 
class BulkContainer{{toplevel.cppname}}JsonListener
    : public AbstractMain{{toplevel.cppname}}JsonListener {
friend class AbstractStacked{{toplevel.cppname}}JsonListener;
public:
    BulkContainer{{toplevel.cppname}}JsonListener(
        {{toplevel.cppname}}Listener* itemListener);
    ~BulkContainer{{toplevel.cppname}}JsonListener();

    bool Handle(const BJsonEvent& event);

private:
    {{toplevel.cppname}}Listener* fItemListener;
};

#endif // GEN_JSON_SCHEMA_PARSER__{{toplevel.cppnameupper}}_H
"""

IMPLEMENTATION_TEMPLATE = """
/*
 * Generated Listener Object for {{toplevel.cppname}}
 */
 
#include "{{toplevel.cppname}}JsonListener.h"
#include <ObjectList.h>

#include <stdio.h>

// #pragma mark - private interfaces for the stacked listeners

        
/*! This class is the top level of the stacked listeners.  The stack structure
    is maintained in a linked list and sub-classes implement specific behaviors
    depending where in the parse tree the stacked listener is working at.
*/        
class AbstractStacked{{toplevel.cppname}}JsonListener : public BJsonEventListener {
public:
    AbstractStacked{{toplevel.cppname}}JsonListener(
        AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
        AbstractStacked{{toplevel.cppname}}JsonListener* parent);
    ~AbstractStacked{{toplevel.cppname}}JsonListener();

    void HandleError(status_t status, int32 line, const char* message);
    void Complete();

    status_t ErrorStatus();

    AbstractStacked{{toplevel.cppname}}JsonListener* Parent();

    virtual bool WillPop();

protected:
    AbstractMain{{toplevel.cppname}}JsonListener* fMainListener;

    bool Pop();
    void Push(AbstractStacked{{toplevel.cppname}}JsonListener* stackedListener);


private:
    AbstractStacked{{toplevel.cppname}}JsonListener* fParent;
};

class GeneralArrayStacked{{toplevel.cppname}}JsonListener : public AbstractStacked{{toplevel.cppname}}JsonListener {
public:
    GeneralArrayStacked{{toplevel.cppname}}JsonListener(
        AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
        AbstractStacked{{toplevel.cppname}}JsonListener* parent);
    ~GeneralArrayStacked{{toplevel.cppname}}JsonListener();

    bool Handle(const BJsonEvent& event);
};

class GeneralObjectStacked{{toplevel.cppname}}JsonListener : public AbstractStacked{{toplevel.cppname}}JsonListener {
public:
    GeneralObjectStacked{{toplevel.cppname}}JsonListener(
        AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
        AbstractStacked{{toplevel.cppname}}JsonListener* parent);
    ~GeneralObjectStacked{{toplevel.cppname}}JsonListener();

    bool Handle(const BJsonEvent& event);
};

        
/*! Sometimes attributes of objects are able to be arrays of strings.  This
    listener will parse and return the array of strings.
*/
        
class StringList{{toplevel.cppname}}JsonListener : public AbstractStacked{{toplevel.cppname}}JsonListener {
public:
    StringList{{toplevel.cppname}}JsonListener(
        AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
        AbstractStacked{{toplevel.cppname}}JsonListener* parent);
    ~StringList{{toplevel.cppname}}JsonListener();

    bool Handle(const BJsonEvent& event);
    BObjectList<BString>* Target();

protected:
    BObjectList<BString>* fTarget;
};

{{#allobjs}}
class {{cppname}}_Stacked{{toplevelcppname}}JsonListener : public AbstractStacked{{toplevelcppname}}JsonListener {
public:
    {{cppname}}_Stacked{{toplevelcppname}}JsonListener(
        AbstractMain{{toplevelcppname}}JsonListener* mainListener,
        AbstractStacked{{toplevelcppname}}JsonListener* parent);
    ~{{cppname}}_Stacked{{toplevelcppname}}JsonListener();

    bool Handle(const BJsonEvent& event);
    {{cppname}}* Target();

protected:
    {{cppname}}* fTarget;
    BString fNextItemName;
};

class {{cppname}}_List_Stacked{{toplevelcppname}}JsonListener : public AbstractStacked{{toplevelcppname}}JsonListener {
public:
    {{cppname}}_List_Stacked{{toplevelcppname}}JsonListener(
        AbstractMain{{toplevelcppname}}JsonListener* mainListener,
        AbstractStacked{{toplevelcppname}}JsonListener* parent);
    ~{{cppname}}_List_Stacked{{toplevelcppname}}JsonListener();

    bool Handle(const BJsonEvent& event);

    BObjectList<{{cppname}}>* Target();
        // list of {{cppname}} pointers

private:
    BObjectList<{{cppname}}>* fTarget;
};
{{/allobjs}}


class ItemEmittingStacked{{toplevel.cppname}}JsonListener : public {{toplevel.cppname}}_Stacked{{toplevel.cppname}}JsonListener {
public:
    ItemEmittingStacked{{toplevel.cppname}}JsonListener(
        AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
        AbstractStacked{{toplevel.cppname}}JsonListener* parent,
        {{toplevel.cppname}}Listener* itemListener);
    ~ItemEmittingStacked{{toplevel.cppname}}JsonListener();

    bool WillPop();

private:
    {{toplevel.cppname}}Listener* fItemListener;
};


class BulkContainerStacked{{toplevel.cppname}}JsonListener : public AbstractStacked{{toplevel.cppname}}JsonListener {
public:
    BulkContainerStacked{{toplevel.cppname}}JsonListener(
        AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
        AbstractStacked{{toplevel.cppname}}JsonListener* parent,
        {{toplevel.cppname}}Listener* itemListener);
    ~BulkContainerStacked{{toplevel.cppname}}JsonListener();

    bool Handle(const BJsonEvent& event);

private:
    BString fNextItemName;
    {{toplevel.cppname}}Listener* fItemListener;
};


class BulkContainerItemsStacked{{toplevel.cppname}}JsonListener : public AbstractStacked{{toplevel.cppname}}JsonListener {
public:
    BulkContainerItemsStacked{{toplevel.cppname}}JsonListener(
        AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
        AbstractStacked{{toplevel.cppname}}JsonListener* parent,
        {{toplevel.cppname}}Listener* itemListener);
    ~BulkContainerItemsStacked{{toplevel.cppname}}JsonListener();

    bool Handle(const BJsonEvent& event);
    bool WillPop();

private:
    {{toplevel.cppname}}Listener* fItemListener;
};

// #pragma mark - implementations for the stacked listeners

AbstractStacked{{toplevel.cppname}}JsonListener::AbstractStacked{{toplevel.cppname}}JsonListener (
    AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
    AbstractStacked{{toplevel.cppname}}JsonListener* parent)
{
    fMainListener = mainListener;
    fParent = parent;
}

AbstractStacked{{toplevel.cppname}}JsonListener::~AbstractStacked{{toplevel.cppname}}JsonListener()
{
}

void
AbstractStacked{{toplevel.cppname}}JsonListener::HandleError(status_t status, int32 line, const char* message)
{
    fMainListener->HandleError(status, line, message);
}

void
AbstractStacked{{toplevel.cppname}}JsonListener::Complete()
{
   fMainListener->Complete();
}

status_t
AbstractStacked{{toplevel.cppname}}JsonListener::ErrorStatus()
{
    return fMainListener->ErrorStatus();
}

AbstractStacked{{toplevel.cppname}}JsonListener*
AbstractStacked{{toplevel.cppname}}JsonListener::Parent()
{
    return fParent;
}

void
AbstractStacked{{toplevel.cppname}}JsonListener::Push(AbstractStacked{{toplevel.cppname}}JsonListener* stackedListener)
{
    fMainListener->SetStackedListener(stackedListener);
}

bool
AbstractStacked{{toplevel.cppname}}JsonListener::WillPop()
{
    return true;
}

bool
AbstractStacked{{toplevel.cppname}}JsonListener::Pop()
{
    bool result = WillPop();
    fMainListener->SetStackedListener(fParent);
    return result;
}

GeneralObjectStacked{{toplevel.cppname}}JsonListener::GeneralObjectStacked{{toplevel.cppname}}JsonListener(
    AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
    AbstractStacked{{toplevel.cppname}}JsonListener* parent)
    :
    AbstractStacked{{toplevel.cppname}}JsonListener(mainListener, parent)
{
}

GeneralObjectStacked{{toplevel.cppname}}JsonListener::~GeneralObjectStacked{{toplevel.cppname}}JsonListener()
{
}

bool
GeneralObjectStacked{{toplevel.cppname}}JsonListener::Handle(const BJsonEvent& event)
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
            Push(new GeneralObjectStacked{{toplevel.cppname}}JsonListener(fMainListener, this));
            break;
        case B_JSON_ARRAY_START:
            Push(new GeneralArrayStacked{{toplevel.cppname}}JsonListener(fMainListener, this));
            break;
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected end of array");
            break;
        case B_JSON_OBJECT_END:
        {
            bool status = Pop() && (ErrorStatus() == B_OK);
            delete this;
            return status;
        }
    }

    return ErrorStatus() == B_OK;
}

GeneralArrayStacked{{toplevel.cppname}}JsonListener::GeneralArrayStacked{{toplevel.cppname}}JsonListener(
    AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
    AbstractStacked{{toplevel.cppname}}JsonListener* parent)
    :
    AbstractStacked{{toplevel.cppname}}JsonListener(mainListener, parent)
{
}

GeneralArrayStacked{{toplevel.cppname}}JsonListener::~GeneralArrayStacked{{toplevel.cppname}}JsonListener()
{
}

bool
GeneralArrayStacked{{toplevel.cppname}}JsonListener::Handle(const BJsonEvent& event)
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
            Push(new GeneralObjectStacked{{toplevel.cppname}}JsonListener(fMainListener, this));
            break;
        case B_JSON_ARRAY_START:
            Push(new GeneralArrayStacked{{toplevel.cppname}}JsonListener(fMainListener, this));
            break;
        case B_JSON_OBJECT_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected end of object");
            break;
        case B_JSON_ARRAY_END:
        {
            bool status = Pop() && (ErrorStatus() == B_OK);
            delete this;
            return status;
        }
    }


    return ErrorStatus() == B_OK;
}

StringList{{toplevel.cppname}}JsonListener::StringList{{toplevel.cppname}}JsonListener(
    AbstractMain{{toplevel.cppname}}JsonListener* mainListener,
    AbstractStacked{{toplevel.cppname}}JsonListener* parent)
    :
    AbstractStacked{{toplevel.cppname}}JsonListener(mainListener, parent)
{
    fTarget = new BObjectList<BString>();
}


StringList{{toplevel.cppname}}JsonListener::~StringList{{toplevel.cppname}}JsonListener()
{
}


BObjectList<BString>*
StringList{{toplevel.cppname}}JsonListener::Target()
{
    return fTarget;
}


bool
StringList{{toplevel.cppname}}JsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        case B_JSON_ARRAY_END:
        {
            bool status = Pop() && (ErrorStatus() == B_OK);
            delete this;
            return status;
        }
        case B_JSON_STRING:
        {
            fTarget->AddItem(new BString(event.Content()));
            break;
        }
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing a string array");
            break;
    }

    return ErrorStatus() == B_OK;
}

{{#allobjs}}
{{cppname}}_Stacked{{toplevelcppname}}JsonListener::{{cppname}}_Stacked{{toplevelcppname}}JsonListener(
    AbstractMain{{toplevelcppname}}JsonListener* mainListener,
    AbstractStacked{{toplevelcppname}}JsonListener* parent)
    :
    AbstractStacked{{toplevelcppname}}JsonListener(mainListener, parent)
{
    fTarget = new {{cppname}}();
}


{{cppname}}_Stacked{{toplevelcppname}}JsonListener::~{{cppname}}_Stacked{{toplevelcppname}}JsonListener()
{
}


{{cppname}}*
{{cppname}}_Stacked{{toplevelcppname}}JsonListener::Target()
{
    return fTarget;
}


bool
{{cppname}}_Stacked{{toplevelcppname}}JsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
        case B_JSON_OBJECT_END:
        {
            bool status = Pop() && (ErrorStatus() == B_OK);
            delete this;
            return status;
        }
        case B_JSON_STRING:
        {
{{#propertyarray}} {{#property.isstring}}
            if (fNextItemName == "{{name}}")
                fTarget->Set{{property.cppname}}(new BString(event.Content()));
{{/property.isstring}}{{/propertyarray}}
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_TRUE:
{{#propertyarray}}{{#property.isboolean}}            if (fNextItemName == "{{name}}")
                fTarget->Set{{property.cppname}}(true);
{{/property.isboolean}}{{/propertyarray}}
            fNextItemName.SetTo("");
            break;
        case B_JSON_FALSE:
{{#propertyarray}}{{#property.isboolean}}            if (fNextItemName == "{{name}}")
                fTarget->Set{{property.cppname}}(false);
{{/property.isboolean}}
{{/propertyarray}}
            fNextItemName.SetTo("");
            break;
        case B_JSON_NULL:
        {
{{#propertyarray}}{{^property.isarray}}            if (fNextItemName == "{{name}}")
                fTarget->Set{{property.cppname}}Null();
{{/property.isarray}}{{/propertyarray}}
            fNextItemName.SetTo("");
            break;
        }     
        case B_JSON_NUMBER:
        {
{{#propertyarray}}{{#property.isinteger}}            if (fNextItemName == "{{name}}")
                fTarget->Set{{property.cppname}}(event.ContentInteger());
{{/property.isinteger}}
{{#property.isnumber}}            if (fNextItemName == "{{name}}")
                fTarget->Set{{property.cppname}}(event.ContentDouble());
{{/property.isnumber}}
{{/propertyarray}}
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_OBJECT_START:
        {
{{#propertyarray}}{{#property.isobject}}            if (fNextItemName == "{{name}}") {
                GeneralObjectStacked{{toplevelcppname}}JsonListener* nextListener
                    = new GeneralObjectStacked{{toplevelcppname}}JsonListener(fMainListener, this);
                Push(nextListener);
                fNextItemName.SetTo("");
                break;
            }
{{/property.isobject}}{{/propertyarray}}
            // a general consumer that will clear any spurious data that was not
            // part of the schema.
            GeneralObjectStacked{{toplevelcppname}}JsonListener* nextListener
                = new GeneralObjectStacked{{toplevelcppname}}JsonListener(fMainListener, this);
            Push(nextListener);
            fNextItemName.SetTo("");
            break;
        }
        case B_JSON_ARRAY_START:
        {
{{#propertyarray}}{{#property.isarray}}            if (fNextItemName == "{{name}}") {
                {{#property.items.isstring}}
                StringList{{toplevelcppname}}JsonListener* nextListener = new StringList{{toplevelcppname}}JsonListener(fMainListener, this);
                fTarget->Set{{property.cppname}}(nextListener->Target());
                {{/property.items.isstring}}{{^property.items.isstring}}
                {{property.items.cppname}}_List_Stacked{{toplevelcppname}}JsonListener* nextListener =
                    new {{property.items.cppname}}_List_Stacked{{toplevelcppname}}JsonListener(fMainListener, this);
                fTarget->Set{{property.cppname}}(nextListener->Target());                
                {{/property.items.isstring}}
                Push(nextListener);
                fNextItemName.SetTo("");
                break;
            }
{{/property.isarray}}{{/propertyarray}}
            // a general consumer that will clear any spurious data that was not
            // part of the schema.
            AbstractStacked{{toplevelcppname}}JsonListener* nextListener
                = new GeneralArrayStacked{{toplevelcppname}}JsonListener(fMainListener, this);
            Push(nextListener);
            fNextItemName.SetTo("");
            break;
        }
    }

    return ErrorStatus() == B_OK;
}


{{cppname}}_List_Stacked{{toplevelcppname}}JsonListener::{{cppname}}_List_Stacked{{toplevelcppname}}JsonListener(
    AbstractMain{{toplevelcppname}}JsonListener* mainListener,
    AbstractStacked{{toplevelcppname}}JsonListener* parent)
    :
    AbstractStacked{{toplevelcppname}}JsonListener(mainListener, parent)
{
    fTarget = new BObjectList<{{cppname}}>();
}


{{cppname}}_List_Stacked{{toplevelcppname}}JsonListener::~{{cppname}}_List_Stacked{{toplevelcppname}}JsonListener()
{
}


BObjectList<{{cppname}}>*
{{cppname}}_List_Stacked{{toplevelcppname}}JsonListener::Target()
{
    return fTarget;
}


bool
{{cppname}}_List_Stacked{{toplevelcppname}}JsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        case B_JSON_ARRAY_END:
        {
            bool status = Pop() && (ErrorStatus() == B_OK);
            delete this;
            return status;
        }
        case B_JSON_OBJECT_START:
        {
            {{cppname}}_Stacked{{toplevelcppname}}JsonListener* nextListener =
                new {{cppname}}_Stacked{{toplevelcppname}}JsonListener(fMainListener, this);
            fTarget->AddItem(nextListener->Target());
            Push(nextListener);
            break;
        }
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing an array of {{cppname}}");
            break;
    }

    return ErrorStatus() == B_OK;
}
{{/allobjs}}

ItemEmittingStacked{{toplevel.cppname}}JsonListener::ItemEmittingStacked{{toplevel.cppname}}JsonListener(
    AbstractMain{{toplevel.cppname}}JsonListener* mainListener, AbstractStacked{{toplevel.cppname}}JsonListener* parent,
    {{toplevel.cppname}}Listener* itemListener)
:
{{toplevel.cppname}}_Stacked{{toplevel.cppname}}JsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


ItemEmittingStacked{{toplevel.cppname}}JsonListener::~ItemEmittingStacked{{toplevel.cppname}}JsonListener()
{
}


bool
ItemEmittingStacked{{toplevel.cppname}}JsonListener::WillPop()
{
    bool result = fItemListener->Handle(fTarget);
    delete fTarget;
    fTarget = NULL;
    return result;
}


BulkContainerStacked{{toplevel.cppname}}JsonListener::BulkContainerStacked{{toplevel.cppname}}JsonListener(
    AbstractMain{{toplevel.cppname}}JsonListener* mainListener, AbstractStacked{{toplevel.cppname}}JsonListener* parent,
    {{toplevel.cppname}}Listener* itemListener)
:
AbstractStacked{{toplevel.cppname}}JsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


BulkContainerStacked{{toplevel.cppname}}JsonListener::~BulkContainerStacked{{toplevel.cppname}}JsonListener()
{
}


bool
BulkContainerStacked{{toplevel.cppname}}JsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
        case B_JSON_OBJECT_START:
            Push(new GeneralObjectStacked{{toplevel.cppname}}JsonListener(fMainListener, this));
            break;
        case B_JSON_ARRAY_START:
            if (fNextItemName == "items")
                Push(new BulkContainerItemsStacked{{toplevel.cppname}}JsonListener(fMainListener, this, fItemListener));
            else
                Push(new GeneralArrayStacked{{toplevel.cppname}}JsonListener(fMainListener, this));
            break;
        case B_JSON_OBJECT_END:
        {
            bool status = Pop() && (ErrorStatus() == B_OK);
            delete this;
            return status;
        }
        default:
                // ignore
            break;
    }

    return ErrorStatus() == B_OK;
}


BulkContainerItemsStacked{{toplevel.cppname}}JsonListener::BulkContainerItemsStacked{{toplevel.cppname}}JsonListener(
    AbstractMain{{toplevel.cppname}}JsonListener* mainListener, AbstractStacked{{toplevel.cppname}}JsonListener* parent,
    {{toplevel.cppname}}Listener* itemListener)
:
AbstractStacked{{toplevel.cppname}}JsonListener(mainListener, parent)
{
    fItemListener = itemListener;
}


BulkContainerItemsStacked{{toplevel.cppname}}JsonListener::~BulkContainerItemsStacked{{toplevel.cppname}}JsonListener()
{
}


bool
BulkContainerItemsStacked{{toplevel.cppname}}JsonListener::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        case B_JSON_OBJECT_START:
            Push(new ItemEmittingStacked{{toplevel.cppname}}JsonListener(fMainListener, this, fItemListener));
            break;
        case B_JSON_ARRAY_END:
        {
            bool status = Pop() && (ErrorStatus() == B_OK);
            delete this;
            return status;
        }
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected json event");
            break;
    }

    return ErrorStatus() == B_OK;
}


bool
BulkContainerItemsStacked{{toplevel.cppname}}JsonListener::WillPop()
{
    fItemListener->Complete();
    return true;
}


// #pragma mark - implementations for the main listeners


AbstractMain{{toplevel.cppname}}JsonListener::AbstractMain{{toplevel.cppname}}JsonListener()
{
    fStackedListener = NULL;
    fErrorStatus = B_OK;
}


AbstractMain{{toplevel.cppname}}JsonListener::~AbstractMain{{toplevel.cppname}}JsonListener()
{
}


void
AbstractMain{{toplevel.cppname}}JsonListener::HandleError(status_t status, int32 line, const char* message)
{
    if (message != NULL) {
        fprintf(stderr, "an error has arisen processing json for '{{toplevel.cppname}}'; %s\\n", message);
    } else {
        fprintf(stderr, "an error has arisen processing json for '{{toplevel.cppname}}'\\n");
    }
    fErrorStatus = status;
}


void
AbstractMain{{toplevel.cppname}}JsonListener::Complete()
{
}


status_t
AbstractMain{{toplevel.cppname}}JsonListener::ErrorStatus()
{
    return fErrorStatus;
}

void
AbstractMain{{toplevel.cppname}}JsonListener::SetStackedListener(
    AbstractStacked{{toplevel.cppname}}JsonListener* stackedListener)
{
    fStackedListener = stackedListener;
}


Single{{toplevel.cppname}}JsonListener::Single{{toplevel.cppname}}JsonListener()
    :
    AbstractMain{{toplevel.cppname}}JsonListener()
{
    fTarget = NULL;
}


Single{{toplevel.cppname}}JsonListener::~Single{{toplevel.cppname}}JsonListener()
{
}


bool
Single{{toplevel.cppname}}JsonListener::Handle(const BJsonEvent& event)
{
    if (fErrorStatus != B_OK)
       return false;

    if (fStackedListener != NULL)
        return fStackedListener->Handle(event);

    switch (event.EventType()) {
        case B_JSON_OBJECT_START:
        {
            {{toplevel.cppname}}_Stacked{{toplevel.cppname}}JsonListener* nextListener
                = new {{toplevel.cppname}}_Stacked{{toplevel.cppname}}JsonListener(this, NULL);
            fTarget = nextListener->Target();
            SetStackedListener(nextListener);
            break;
        }
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing top level for {{toplevel.cppname}}");
            break;
    }

    return ErrorStatus() == B_OK;
}


{{toplevel.cppname}}*
Single{{toplevel.cppname}}JsonListener::Target()
{
    return fTarget;
}


BulkContainer{{toplevel.cppname}}JsonListener::BulkContainer{{toplevel.cppname}}JsonListener(
    {{toplevel.cppname}}Listener* itemListener) : AbstractMain{{toplevel.cppname}}JsonListener()
{
    fItemListener = itemListener;
}


BulkContainer{{toplevel.cppname}}JsonListener::~BulkContainer{{toplevel.cppname}}JsonListener()
{
}


bool
BulkContainer{{toplevel.cppname}}JsonListener::Handle(const BJsonEvent& event)
{
    if (fErrorStatus != B_OK)
       return false;

    if (fStackedListener != NULL)
        return fStackedListener->Handle(event);

    switch (event.EventType()) {
        case B_JSON_OBJECT_START:
        {
            BulkContainerStacked{{toplevel.cppname}}JsonListener* nextListener =
                new BulkContainerStacked{{toplevel.cppname}}JsonListener(this, NULL, fItemListener);
            SetStackedListener(nextListener);
            return true;
            break;
        }
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing top level for BulkContainerStacked{{toplevel.cppname}}JsonListener");
            break;
    }

    return ErrorStatus() == B_OK;
}
"""


def write_files(schema : dict[str, any], output_directory: str) -> None:
    cpp_listener_name = schema["cppname"] + "JsonListener"
    cpp_header_filename = os.path.join(output_directory, cpp_listener_name + '.h')
    cpp_implementation_filename = os.path.join(output_directory, cpp_listener_name + '.cpp')

    model = dict[string, any]()

    model['toplevel'] = schema
    model['allobjs'] = hdsjsonschemacommon.collect_all_objects(schema)

    with open(cpp_header_filename, 'w') as cpp_h_file:
        cpp_h_file.write(ustache.render(
            HEADER_TEMPLATE,
            model,
            escape= lambda x: x))

    with open(cpp_implementation_filename, 'w') as cpp_i_file:
        cpp_i_file.write(ustache.render(
            IMPLEMENTATION_TEMPLATE,
            model,
            escape= lambda x: x))


def main():

    parser = argparse.ArgumentParser(description='Convert JSON schema to Haiku C++ Parsers')
    parser.add_argument('-i', '--inputfile', required=True, help='The input filename containing the JSON schema')
    parser.add_argument('--outputdirectory', help='The output directory where the C++ files should be written')

    args = parser.parse_args()

    outputdirectory = args.outputdirectory

    if not outputdirectory:
        outputdirectory = '.'

    with open(args.inputfile) as inputfile:
        schema = json.load(inputfile)
        hdsjsonschemacommon.augment_schema(schema)
        write_files(schema, outputdirectory)


if __name__ == "__main__":
    main()