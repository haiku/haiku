/*
 * Generated Listener Object
 * source json-schema : dumpexport.json
 * generated at : 2017-12-17T20:45:25.514143
 */

#ifndef GEN_JSON_SCHEMA_PARSER__SINGLEDUMPEXPORTPKGJSONLISTENER_H
#define GEN_JSON_SCHEMA_PARSER__SINGLEDUMPEXPORTPKGJSONLISTENER_H

#include <JsonEventListener.h>

#include "DumpExportPkg.h"

class AbstractStackedDumpExportPkgJsonListener;

class AbstractMainDumpExportPkgJsonListener : public BJsonEventListener {
friend class AbstractStackedDumpExportPkgJsonListener;
public:
    AbstractMainDumpExportPkgJsonListener();
    virtual ~AbstractMainDumpExportPkgJsonListener();
    
    void HandleError(status_t status, int32 line, const char* message);
    void Complete();
    status_t ErrorStatus();
    
protected:
    void SetStackedListener(
        AbstractStackedDumpExportPkgJsonListener* listener);
    status_t fErrorStatus;
    AbstractStackedDumpExportPkgJsonListener* fStackedListener;
};


/*! Use this listener when you want to parse some JSON data that contains
    just a single instance of DumpExportPkg.
*/
class SingleDumpExportPkgJsonListener
    : public AbstractMainDumpExportPkgJsonListener {
friend class AbstractStackedDumpExportPkgJsonListener;
public:
    SingleDumpExportPkgJsonListener();
    virtual ~SingleDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    DumpExportPkg* Target();
    
private:
    DumpExportPkg* fTarget;
};

    
/*! Concrete sub-classes of this class are able to respond to each
    DumpExportPkg* instance as
    it is parsed from the bulk container.  When the stream is
    finished, the Complete() method is invoked.
    
    Note that the item object will be deleted after the Handle method
    is invoked.  The Handle method need not take responsibility
    for deleting the item itself.
*/         
class DumpExportPkgListener {
public:
    virtual bool Handle(DumpExportPkg* item) = 0;
    virtual void Complete() = 0;
};

                
/*! Use this listener, together with an instance of a concrete
    subclass of DumpExportPkgListener
    in order to parse the JSON data in a specific "bulk
    container" format.  Each time that an instance of
    DumpExportPkg
    is parsed, the instance item listener will be invoked.
*/ 
class BulkContainerDumpExportPkgJsonListener
    : public AbstractMainDumpExportPkgJsonListener {
friend class AbstractStackedDumpExportPkgJsonListener;
public:
    BulkContainerDumpExportPkgJsonListener(
        DumpExportPkgListener* itemListener);
    ~BulkContainerDumpExportPkgJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
private:
    DumpExportPkgListener* fItemListener;
};

#endif // GEN_JSON_SCHEMA_PARSER__SINGLEDUMPEXPORTPKGJSONLISTENER_H