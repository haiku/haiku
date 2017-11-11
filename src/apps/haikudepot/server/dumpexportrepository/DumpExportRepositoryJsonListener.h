/*
 * Generated Listener Object
 * source json-schema : dumpexport.json
 * generated at : 2017-11-11T14:08:39.274913
 */

#ifndef GEN_JSON_SCHEMA_PARSER__SINGLEDUMPEXPORTREPOSITORYJSONLISTENER_H
#define GEN_JSON_SCHEMA_PARSER__SINGLEDUMPEXPORTREPOSITORYJSONLISTENER_H

#include <JsonEventListener.h>

#include "DumpExportRepository.h"

class AbstractStackedDumpExportRepositoryJsonListener;

class AbstractMainDumpExportRepositoryJsonListener : public BJsonEventListener {
friend class AbstractStackedDumpExportRepositoryJsonListener;
public:
    AbstractMainDumpExportRepositoryJsonListener();
    virtual ~AbstractMainDumpExportRepositoryJsonListener();
    
    void HandleError(status_t status, int32 line, const char* message);
    void Complete();
    status_t ErrorStatus();
    
protected:
    void SetStackedListener(
        AbstractStackedDumpExportRepositoryJsonListener* listener);
    status_t fErrorStatus;
    AbstractStackedDumpExportRepositoryJsonListener* fStackedListener;
};


/*! Use this listener when you want to parse some JSON data that contains
    just a single instance of DumpExportRepository.
*/
class SingleDumpExportRepositoryJsonListener
    : public AbstractMainDumpExportRepositoryJsonListener {
friend class AbstractStackedDumpExportRepositoryJsonListener;
public:
    SingleDumpExportRepositoryJsonListener();
    virtual ~SingleDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
    DumpExportRepository* Target();
    
private:
    DumpExportRepository* fTarget;
};

    
/*! Concrete sub-classes of this class are able to respond to each
    DumpExportRepository* instance as
    it is parsed from the bulk container.  When the stream is
    finished, the Complete() method is invoked.
    
    Note that the item object will be deleted after the Handle method
    is invoked.  The Handle method need not take responsibility
    for deleting the item itself.
*/         
class DumpExportRepositoryListener {
public:
    virtual void Handle(DumpExportRepository* item) = 0;
    virtual void Complete() = 0;
};

                
/*! Use this listener, together with an instance of a concrete
    subclass of DumpExportRepositoryListener
    in order to parse the JSON data in a specific "bulk
    container" format.  Each time that an instance of
    DumpExportRepository
    is parsed, the instance item listener will be invoked.
*/ 
class BulkContainerDumpExportRepositoryJsonListener
    : public AbstractMainDumpExportRepositoryJsonListener {
friend class AbstractStackedDumpExportRepositoryJsonListener;
public:
    BulkContainerDumpExportRepositoryJsonListener(
        DumpExportRepositoryListener* itemListener);
    ~BulkContainerDumpExportRepositoryJsonListener();
    
    bool Handle(const BJsonEvent& event);
    
private:
    DumpExportRepositoryListener* fItemListener;
};

#endif // GEN_JSON_SCHEMA_PARSER__SINGLEDUMPEXPORTREPOSITORYJSONLISTENER_H