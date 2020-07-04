#!/usr/bin/python

# =====================================
# Copyright 2017-2020, Andrew Lindesay
# Distributed under the terms of the MIT License.
# =====================================

# This simple tool will read a JSON schema and will then generate a
# listener for use the 'BJson' class in the Haiku system.  This
# allows data conforming to the schema to be able to be parsed.

import string
import json
import argparse
import os
import hdsjsonschemacommon as jscom


# This naming is related to a sub-type in the schema; maybe not the top-level.

class CppParserSubTypeNaming:
    _cppmodelclassname = None
    _naming = None

    def __init__(self, schema, naming):
        type = schema['type']

        if not type or 0 == len(type):
            raise Exception('missing "type" field')

        def derivecppmodelclassname():
            if type == jscom.JSON_TYPE_OBJECT:
                javatype = schema['javaType']
                if not javatype or 0 == len(javatype):
                    raise Exception('missing "javaType" field')
                return jscom.javatypetocppname(javatype)
            if type == jscom.JSON_TYPE_STRING:
                return jscom.CPP_TYPE_STRING
            raise Exception('unsupported "type" of "%s"' % type)

        self._cppmodelclassname = derivecppmodelclassname()
        self._naming = naming

    def cppmodelclassname(self):
        return self._cppmodelclassname

    def cppstackedlistenerclassname(self):
        return self._cppmodelclassname + '_' + self._naming.generatejsonlistenername('Stacked')

    def cppstackedlistlistenerclassname(self):
        if self._cppmodelclassname == jscom.CPP_TYPE_STRING:
            return self._naming.cppstringliststackedlistenerclassname()
        return self._cppmodelclassname + '_List_' + self._naming.generatejsonlistenername('Stacked')

    def todict(self):
        return {
            'subtype_cppmodelclassname': self.cppmodelclassname(),
            'subtype_cppstackedlistenerclassname': self.cppstackedlistenerclassname(),
            'subtype_cppstackedlistlistenerclassname': self.cppstackedlistlistenerclassname()
        }


# This naming relates to the whole schema.  It's point of
# reference is the top level.

class CppParserNaming:
    _schemaroot = None

    def __init__(self, schemaroot):
        self._schemaroot = schemaroot

    def cpprootmodelclassname(self):
        type = self._schemaroot['type']
        if type != 'object':
            raise Exception('expecting object, but was [' + type + "]")

        javatype = self._schemaroot['javaType']

        if not javatype or 0 == len(javatype):
            raise Exception('missing "javaType" field')

        return jscom.javatypetocppname(javatype)

    def generatejsonlistenername(self, prefix):
        return prefix + self.cpprootmodelclassname() + 'JsonListener'

    def cppsupermainlistenerclassname(self):
        return self.generatejsonlistenername('AbstractMain')

    def cppsinglemainlistenerclassname(self):
        return self.generatejsonlistenername("Single")

    def cppbulkcontainermainlistenerclassname(self):
        return self.generatejsonlistenername('BulkContainer')

    def cppsuperstackedlistenerclassname(self):
        return self.generatejsonlistenername('AbstractStacked')

    def cppstringliststackedlistenerclassname(self):
        return self.generatejsonlistenername('StringList')

    def cppbulkcontainerstackedlistenerclassname(self):
        return self.generatejsonlistenername('BulkContainerStacked')

    def cppbulkcontaineritemliststackedlistenerclassname(self):
        return self.generatejsonlistenername('BulkContainerItemsStacked')

# this is a sub-class of the root object json listener that can call out to the
# item listener as the root objects are parsed.

    def cppitemlistenerstackedlistenerclassname(self):
        return self.generatejsonlistenername('ItemEmittingStacked')

    def cppgeneralobjectstackedlistenerclassname(self):
        return self.generatejsonlistenername('GeneralObjectStacked')

    def cppgeneralarraystackedlistenerclassname(self):
        return self.generatejsonlistenername('GeneralArrayStacked')

    def cppitemlistenerclassname(self):
        return self.cpprootmodelclassname() + 'Listener'

    def todict(self):
        return {
            'cpprootmodelclassname': self.cpprootmodelclassname(),
            'cppsupermainlistenerclassname': self.cppsupermainlistenerclassname(),
            'cppsinglemainlistenerclassname': self.cppsinglemainlistenerclassname(),
            'cppbulkcontainermainlistenerclassname': self.cppbulkcontainermainlistenerclassname(),
            'cppbulkcontainerstackedlistenerclassname': self.cppbulkcontainerstackedlistenerclassname(),
            'cppbulkcontaineritemliststackedlistenerclassname': self.cppbulkcontaineritemliststackedlistenerclassname(),
            'cppsuperstackedlistenerclassname': self.cppsuperstackedlistenerclassname(),
            'cppitemlistenerstackedlistenerclassname': self.cppitemlistenerstackedlistenerclassname(),
            'cppstringliststackedlistenerclassname': self.cppstringliststackedlistenerclassname(),
            'cppgeneralobjectstackedlistenerclassname': self.cppgeneralobjectstackedlistenerclassname(),
            'cppgeneralarraystackedlistenerclassname': self.cppgeneralarraystackedlistenerclassname(),
            'cppitemlistenerclassname': self.cppitemlistenerclassname()
        }


class CppParserImplementationState:

    _interfacehandledcppnames = []
    _implementationhandledcppnames = []
    _outputfile = None
    _naming = None

    def __init__(self, outputfile, naming):
        self._outputfile = outputfile
        self._naming = naming

    def isinterfacehandledcppname(self, name):
        return name in self._interfacehandledcppnames

    def addinterfacehandledcppname(self, name):
        self._interfacehandledcppnames.append(name)

    def isimplementationhandledcppname(self, name):
        return name in self._implementationhandledcppnames

    def addimplementationhandledcppname(self, name):
        self._implementationhandledcppnames.append(name)

    def naming(self):
        return self._naming

    def outputfile(self):
        return self._outputfile


def writerootstackedlistenerinterface(istate):
    istate.outputfile().write(
        string.Template("""        
/*! This class is the top level of the stacked listeners.  The stack structure
    is maintained in a linked list and sub-classes implement specific behaviors
    depending where in the parse tree the stacked listener is working at.
*/        
class ${cppsuperstackedlistenerclassname} : public BJsonEventListener {
public:
    ${cppsuperstackedlistenerclassname}(
        ${cppsupermainlistenerclassname}* mainListener,
        ${cppsuperstackedlistenerclassname}* parent);
    ~${cppsuperstackedlistenerclassname}();

    void HandleError(status_t status, int32 line, const char* message);
    void Complete();

    status_t ErrorStatus();

    ${cppsuperstackedlistenerclassname}* Parent();

    virtual bool WillPop();

protected:
    ${cppsupermainlistenerclassname}* fMainListener;

    bool Pop();
    void Push(${cppsuperstackedlistenerclassname}* stackedListener);


private:
    ${cppsuperstackedlistenerclassname}* fParent;
};
""").substitute(istate.naming().todict()))


def writerootstackedlistenerimplementation(istate):
    istate.outputfile().write(
        string.Template("""
${cppsuperstackedlistenerclassname}::${cppsuperstackedlistenerclassname} (
    ${cppsupermainlistenerclassname}* mainListener,
    ${cppsuperstackedlistenerclassname}* parent)
{
    fMainListener = mainListener;
    fParent = parent;
}

${cppsuperstackedlistenerclassname}::~${cppsuperstackedlistenerclassname}()
{
}

void
${cppsuperstackedlistenerclassname}::HandleError(status_t status, int32 line, const char* message)
{
    fMainListener->HandleError(status, line, message);
}

void
${cppsuperstackedlistenerclassname}::Complete()
{
   fMainListener->Complete();
}

status_t
${cppsuperstackedlistenerclassname}::ErrorStatus()
{
    return fMainListener->ErrorStatus();
}

${cppsuperstackedlistenerclassname}*
${cppsuperstackedlistenerclassname}::Parent()
{
    return fParent;
}

void
${cppsuperstackedlistenerclassname}::Push(${cppsuperstackedlistenerclassname}* stackedListener)
{
    fMainListener->SetStackedListener(stackedListener);
}

bool
${cppsuperstackedlistenerclassname}::WillPop()
{
    return true;
}

bool
${cppsuperstackedlistenerclassname}::Pop()
{
    bool result = WillPop();
    fMainListener->SetStackedListener(fParent);
    return result;
}
""").substitute(istate.naming().todict()))


def writestringliststackedlistenerinterface(istate):
    istate.outputfile().write(
        string.Template("""
        
/*! Sometimes attributes of objects are able to be arrays of strings.  This
    listener will parse and return the array of strings.
*/
        
class ${cppstringliststackedlistenerclassname} : public ${cppsuperstackedlistenerclassname} {
public:
    ${cppstringliststackedlistenerclassname}(
        ${cppsupermainlistenerclassname}* mainListener,
        ${cppsuperstackedlistenerclassname}* parent);
    ~${cppstringliststackedlistenerclassname}();

    bool Handle(const BJsonEvent& event);
    BObjectList<BString>* Target();

protected:
    BObjectList<BString>* fTarget;
};
""").substitute(istate.naming().todict()))


def writestringliststackedlistenerimplementation(istate):
    istate.outputfile().write(
        string.Template("""
${cppstringliststackedlistenerclassname}::${cppstringliststackedlistenerclassname}(
    ${cppsupermainlistenerclassname}* mainListener,
    ${cppsuperstackedlistenerclassname}* parent)
    :
    ${cppsuperstackedlistenerclassname}(mainListener, parent)
{
    fTarget = new BObjectList<BString>();
}


${cppstringliststackedlistenerclassname}::~${cppstringliststackedlistenerclassname}()
{
}


BObjectList<BString>*
${cppstringliststackedlistenerclassname}::Target()
{
    return fTarget;
}


bool
${cppstringliststackedlistenerclassname}::Handle(const BJsonEvent& event)
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
""").substitute(istate.naming().todict()))


def writeageneralstackedlistenerinterface(istate, alistenerclassname):
    istate.outputfile().write(
        string.Template("""
class ${alistenerclassname} : public ${cppsuperstackedlistenerclassname} {
public:
    ${alistenerclassname}(
        ${cppsupermainlistenerclassname}* mainListener,
        ${cppsuperstackedlistenerclassname}* parent);
    ~${alistenerclassname}();


    bool Handle(const BJsonEvent& event);
};
""").substitute(jscom.uniondicts(
            istate.naming().todict(),
            {'alistenerclassname': alistenerclassname}
        )))


def writegeneralstackedlistenerinterface(istate):
    writeageneralstackedlistenerinterface(istate, istate.naming().cppgeneralarraystackedlistenerclassname())
    writeageneralstackedlistenerinterface(istate, istate.naming().cppgeneralobjectstackedlistenerclassname())


def writegeneralnoopstackedlistenerconstructordestructor(istate, aclassname):

    istate.outputfile().write(
        string.Template("""
${aclassname}::${aclassname}(
    ${cppsupermainlistenerclassname}* mainListener,
    ${cppsuperstackedlistenerclassname}* parent)
    :
    ${cppsuperstackedlistenerclassname}(mainListener, parent)
{
}

${aclassname}::~${aclassname}()
{
}
""").substitute(jscom.uniondicts(
            istate.naming().todict(),
            {'aclassname': aclassname}))
    )


def writegeneralstackedlistenerimplementation(istate):
    outfile = istate.outputfile()
    generalobjectclassname = istate.naming().cppgeneralobjectstackedlistenerclassname()
    generalarrayclassname = istate.naming().cppgeneralarraystackedlistenerclassname()
    substitutedict = {
        'generalobjectclassname': generalobjectclassname,
        'generalarrayclassname': generalarrayclassname
    }

# general object consumer that will parse-and-discard any json objects.

    writegeneralnoopstackedlistenerconstructordestructor(istate, generalobjectclassname)

    istate.outputfile().write(
        string.Template("""
bool
${generalobjectclassname}::Handle(const BJsonEvent& event)
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
            Push(new ${generalobjectclassname}(fMainListener, this));
            break;
        case B_JSON_ARRAY_START:
            Push(new ${generalarrayclassname}(fMainListener, this));
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
""").substitute(substitutedict))

    # general array consumer that will parse-and-discard any json arrays.

    writegeneralnoopstackedlistenerconstructordestructor(istate, generalarrayclassname)

    outfile.write(
        string.Template("""
bool
${generalarrayclassname}::Handle(const BJsonEvent& event)
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
            Push(new ${generalobjectclassname}(fMainListener, this));
            break;
        case B_JSON_ARRAY_START:
            Push(new ${generalarrayclassname}(fMainListener, this));
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
""").substitute(substitutedict))


def writestackedlistenerinterface(istate, subschema):
    naming = istate.naming()
    subtypenaming = CppParserSubTypeNaming(subschema, naming)

    if not istate.isinterfacehandledcppname(subtypenaming.cppmodelclassname()):
        istate.addinterfacehandledcppname(subtypenaming.cppmodelclassname())

        istate.outputfile().write(
            string.Template("""
class ${subtype_cppstackedlistenerclassname} : public ${cppsuperstackedlistenerclassname} {
public:
    ${subtype_cppstackedlistenerclassname}(
        ${cppsupermainlistenerclassname}* mainListener,
        ${cppsuperstackedlistenerclassname}* parent);
    ~${subtype_cppstackedlistenerclassname}();


    bool Handle(const BJsonEvent& event);


    ${subtype_cppmodelclassname}* Target();


protected:
    ${subtype_cppmodelclassname}* fTarget;
    BString fNextItemName;
};

class ${subtype_cppstackedlistlistenerclassname} : public ${cppsuperstackedlistenerclassname} {
public:
    ${subtype_cppstackedlistlistenerclassname}(
        ${cppsupermainlistenerclassname}* mainListener,
        ${cppsuperstackedlistenerclassname}* parent);
    ~${subtype_cppstackedlistlistenerclassname}();

    bool Handle(const BJsonEvent& event);

    BObjectList<${subtype_cppmodelclassname}>* Target();
        // list of ${subtype_cppmodelclassname} pointers

private:
    BObjectList<${subtype_cppmodelclassname}>* fTarget;
};
""").substitute(jscom.uniondicts(naming.todict(), subtypenaming.todict())))

        if 'properties' in subschema:

            for propname, propmetadata in subschema['properties'].items():
                if propmetadata['type'] == jscom.JSON_TYPE_ARRAY:
                    if propmetadata['items']['type'] == jscom.JSON_TYPE_OBJECT:
                        writestackedlistenerinterface(istate, propmetadata['items'])
                elif propmetadata['type'] == jscom.JSON_TYPE_OBJECT:
                    writestackedlistenerinterface(istate, propmetadata)


def writebulkcontainerstackedlistenerinterface(istate, schema):
    naming = istate.naming()
    subtypenaming = CppParserSubTypeNaming(schema, naming)
    outfile = istate.outputfile()

# This is a sub-class of the main model object listener.  It will ping out to an item listener
# when parsing is complete.

    outfile.write(
        string.Template("""
class ${cppitemlistenerstackedlistenerclassname} : public ${subtype_cppstackedlistenerclassname} {
public:
    ${cppitemlistenerstackedlistenerclassname}(
        ${cppsupermainlistenerclassname}* mainListener,
        ${cppsuperstackedlistenerclassname}* parent,
        ${cppitemlistenerclassname}* itemListener);
    ~${cppitemlistenerstackedlistenerclassname}();


    bool WillPop();


private:
    ${cppitemlistenerclassname}* fItemListener;
};


class ${cppbulkcontainerstackedlistenerclassname} : public ${cppsuperstackedlistenerclassname} {
public:
    ${cppbulkcontainerstackedlistenerclassname}(
        ${cppsupermainlistenerclassname}* mainListener,
        ${cppsuperstackedlistenerclassname}* parent,
        ${cppitemlistenerclassname}* itemListener);
    ~${cppbulkcontainerstackedlistenerclassname}();


    bool Handle(const BJsonEvent& event);


private:
    BString fNextItemName;
    ${cppitemlistenerclassname}* fItemListener;
};


class ${cppbulkcontaineritemliststackedlistenerclassname} : public ${cppsuperstackedlistenerclassname} {
public:
    ${cppbulkcontaineritemliststackedlistenerclassname}(
        ${cppsupermainlistenerclassname}* mainListener,
        ${cppsuperstackedlistenerclassname}* parent,
        ${cppitemlistenerclassname}* itemListener);
    ~${cppbulkcontaineritemliststackedlistenerclassname}();


    bool Handle(const BJsonEvent& event);
    bool WillPop();


private:
    ${cppitemlistenerclassname}* fItemListener;
};
""").substitute(jscom.uniondicts(naming.todict(), subtypenaming.todict())))


def writestackedlistenerfieldimplementation(
        istate,
        propname,
        cppeventdataexpression):

    istate.outputfile().write(
        string.Template("""
            if (fNextItemName == "${propname}")
                fTarget->Set${cpppropname}(${cppeventdataexpression});
        """).substitute({
            'propname': propname,
            'cpppropname': jscom.propnametocppname(propname),
            'cppeventdataexpression': cppeventdataexpression
        }))


def writenullstackedlistenerfieldimplementation(
        istate,
        propname):

    istate.outputfile().write(
        string.Template("""
            if (fNextItemName == "${propname}")
                fTarget->Set${cpppropname}Null();
        """).substitute({
            'propname': propname,
            'cpppropname': jscom.propnametocppname(propname)
        }))


def writestackedlistenerfieldsimplementation(
        istate,
        schema,
        selectedcpptypename,
        jsoneventtypename,
        cppeventdataexpression):

    outfile = istate.outputfile()

    outfile.write('        case ' + jsoneventtypename + ':\n')

    for propname, propmetadata in schema['properties'].items():
        cpptypename = jscom.propmetadatatocpptypename(propmetadata)

        if cpptypename == selectedcpptypename:
            writestackedlistenerfieldimplementation(istate, propname, cppeventdataexpression)

    outfile.write('            fNextItemName.SetTo("");\n')
    outfile.write('            break;\n')


def writestackedlistenertypedobjectimplementation(istate, schema):
    outfile = istate.outputfile()
    naming = istate.naming();
    subtypenaming = CppParserSubTypeNaming(schema, naming)

    outfile.write(
        string.Template("""
${subtype_cppstackedlistenerclassname}::${subtype_cppstackedlistenerclassname}(
    ${cppsupermainlistenerclassname}* mainListener,
    ${cppsuperstackedlistenerclassname}* parent)
    :
    ${cppsuperstackedlistenerclassname}(mainListener, parent)
{
    fTarget = new ${subtype_cppmodelclassname}();
}


${subtype_cppstackedlistenerclassname}::~${subtype_cppstackedlistenerclassname}()
{
}


${subtype_cppmodelclassname}*
${subtype_cppstackedlistenerclassname}::Target()
{
    return fTarget;
}


bool
${subtype_cppstackedlistenerclassname}::Handle(const BJsonEvent& event)
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

""").substitute(jscom.uniondicts(
            naming.todict(),
            subtypenaming.todict())))

    # now extract the fields from the schema that need to be fed in.

    writestackedlistenerfieldsimplementation(
        istate, schema,
        jscom.CPP_TYPE_STRING, 'B_JSON_STRING', 'new BString(event.Content())')

    writestackedlistenerfieldsimplementation(
        istate, schema,
        jscom.CPP_TYPE_BOOLEAN, 'B_JSON_TRUE', 'true')

    writestackedlistenerfieldsimplementation(
        istate, schema,
        jscom.CPP_TYPE_BOOLEAN, 'B_JSON_FALSE', 'false')

    outfile.write('        case B_JSON_NULL:\n')
    outfile.write('        {\n')

    for propname, propmetadata in schema['properties'].items():
        # TODO; deal with array case somehow.
        if 'array' != propmetadata['type']:
            writenullstackedlistenerfieldimplementation(istate, propname)
    outfile.write('            fNextItemName.SetTo("");\n')
    outfile.write('            break;\n')
    outfile.write('        }\n')

    # number type is a bit complex because it can either be a double or it can be an
    # integral value.

    outfile.write('        case B_JSON_NUMBER:\n')
    outfile.write('        {\n')

    for propname, propmetadata in schema['properties'].items():
        propcpptypename = jscom.propmetadatatocpptypename(propmetadata)
        if propcpptypename == jscom.CPP_TYPE_INTEGER:
            writestackedlistenerfieldimplementation(istate, propname, 'event.ContentInteger()')
        elif propcpptypename == jscom.CPP_TYPE_NUMBER:
            writestackedlistenerfieldimplementation(istate, propname, 'event.ContentDouble()')
    outfile.write('            fNextItemName.SetTo("");\n')
    outfile.write('            break;\n')
    outfile.write('        }\n')

    # object type; could be a sub-type or otherwise just drop into a placebo consumer to keep the parse
    # structure working.  This would most likely be additional sub-objects that are additional to the
    # expected schema.

    outfile.write('        case B_JSON_OBJECT_START:\n')
    outfile.write('        {\n')

    objectifclausekeyword = 'if'

    for propname, propmetadata in schema['properties'].items():
        if propmetadata['type'] == jscom.JSON_TYPE_OBJECT:
            subtypenaming = CppParserSubTypeNaming(propmetadata, naming)

            outfile.write('            %s (fNextItemName == "%s") {\n' % (objectifclausekeyword, propname))
            outfile.write('                %s* nextListener = new %s(fMainListener, this);\n' % (
                subtypenaming.cppstackedlistenerclassname(),
                subtypenaming.cppstackedlistenerclassname()))
            outfile.write('                fTarget->Set%s(nextListener->Target());\n' % (
                subtypenaming.cppmodelclassname()))
            outfile.write('                Push(nextListener);\n')
            outfile.write('            }\n')

            objectifclausekeyword = 'else if'

    outfile.write('            %s (1 == 1) {\n' % objectifclausekeyword)
    outfile.write('                %s* nextListener = new %s(fMainListener, this);\n' % (
        naming.cppgeneralobjectstackedlistenerclassname(),
        naming.cppgeneralobjectstackedlistenerclassname()))
    outfile.write('                Push(nextListener);\n')
    outfile.write('            }\n')
    outfile.write('            fNextItemName.SetTo("");\n')
    outfile.write('            break;\n')
    outfile.write('        }\n')

    # array type; could be an array of objects or otherwise just drop into a placebo consumer to keep
    # the parse structure working.

    outfile.write('        case B_JSON_ARRAY_START:\n')
    outfile.write('        {\n')

    objectifclausekeyword = 'if'

    for propname, propmetadata in schema['properties'].items():
        if propmetadata['type'] == jscom.JSON_TYPE_ARRAY:
            subtypenaming = CppParserSubTypeNaming(propmetadata['items'], naming)

            outfile.write('            %s (fNextItemName == "%s") {\n' % (objectifclausekeyword, propname))
            outfile.write('                %s* nextListener = new %s(fMainListener, this);\n' % (
                subtypenaming.cppstackedlistlistenerclassname(),
                subtypenaming.cppstackedlistlistenerclassname()))
            outfile.write('                fTarget->Set%s(nextListener->Target());\n' % (
                jscom.propnametocppname(propname)))
            outfile.write('                Push(nextListener);\n')
            outfile.write('            }\n')

            objectifclausekeyword = 'else if'

    outfile.write('            %s (1 == 1) {\n' % objectifclausekeyword)
    outfile.write('                %s* nextListener = new %s(fMainListener, this);\n' % (
        naming.cppsuperstackedlistenerclassname(),
        naming.cppgeneralarraystackedlistenerclassname()))
    outfile.write('                Push(nextListener);\n')
    outfile.write('            }\n')
    outfile.write('            fNextItemName.SetTo("");\n')
    outfile.write('            break;\n')
    outfile.write('        }\n')

    outfile.write("""
    }


    return ErrorStatus() == B_OK;
}
""")


def writestackedlistenertypedobjectlistimplementation(istate, schema):
    naming = istate.naming()
    subtypenaming = CppParserSubTypeNaming(schema, naming)
    outfile = istate.outputfile()

    outfile.write(
        string.Template("""
${subtype_cppstackedlistlistenerclassname}::${subtype_cppstackedlistlistenerclassname}(
    ${cppsupermainlistenerclassname}* mainListener,
    ${cppsuperstackedlistenerclassname}* parent)
    :
    ${cppsuperstackedlistenerclassname}(mainListener, parent)
{
    fTarget = new BObjectList<${subtype_cppmodelclassname}>();
}


${subtype_cppstackedlistlistenerclassname}::~${subtype_cppstackedlistlistenerclassname}()
{
}


BObjectList<${subtype_cppmodelclassname}>*
${subtype_cppstackedlistlistenerclassname}::Target()
{
    return fTarget;
}


bool
${subtype_cppstackedlistlistenerclassname}::Handle(const BJsonEvent& event)
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
            ${subtype_cppstackedlistenerclassname}* nextListener =
                new ${subtype_cppstackedlistenerclassname}(fMainListener, this);
            fTarget->AddItem(nextListener->Target());
            Push(nextListener);
            break;
        }
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing an array of ${subtype_cppmodelclassname}");
            break;
    }

    return ErrorStatus() == B_OK;
}
""").substitute(jscom.uniondicts(naming.todict(), subtypenaming.todict())))


def writebulkcontainerstackedlistenerimplementation(istate, schema):
    naming = istate.naming()
    subtypenaming = CppParserSubTypeNaming(schema, naming)
    outfile = istate.outputfile()

    outfile.write(
        string.Template("""
${cppitemlistenerstackedlistenerclassname}::${cppitemlistenerstackedlistenerclassname}(
    ${cppsupermainlistenerclassname}* mainListener, ${cppsuperstackedlistenerclassname}* parent,
    ${cppitemlistenerclassname}* itemListener)
:
${subtype_cppstackedlistenerclassname}(mainListener, parent)
{
    fItemListener = itemListener;
}


${cppitemlistenerstackedlistenerclassname}::~${cppitemlistenerstackedlistenerclassname}()
{
}


bool
${cppitemlistenerstackedlistenerclassname}::WillPop()
{
    bool result = fItemListener->Handle(fTarget);
    delete fTarget;
    fTarget = NULL;
    return result;
}


${cppbulkcontainerstackedlistenerclassname}::${cppbulkcontainerstackedlistenerclassname}(
    ${cppsupermainlistenerclassname}* mainListener, ${cppsuperstackedlistenerclassname}* parent,
    ${cppitemlistenerclassname}* itemListener)
:
${cppsuperstackedlistenerclassname}(mainListener, parent)
{
    fItemListener = itemListener;
}


${cppbulkcontainerstackedlistenerclassname}::~${cppbulkcontainerstackedlistenerclassname}()
{
}


bool
${cppbulkcontainerstackedlistenerclassname}::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        case B_JSON_ARRAY_END:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE, "illegal state - unexpected start of array");
            break;
        case B_JSON_OBJECT_NAME:
            fNextItemName = event.Content();
            break;
        case B_JSON_OBJECT_START:
            Push(new ${cppgeneralobjectstackedlistenerclassname}(fMainListener, this));
            break;
        case B_JSON_ARRAY_START:
            if (fNextItemName == "items")
                Push(new ${cppbulkcontaineritemliststackedlistenerclassname}(fMainListener, this, fItemListener));
            else
                Push(new ${cppgeneralarraystackedlistenerclassname}(fMainListener, this));
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


${cppbulkcontaineritemliststackedlistenerclassname}::${cppbulkcontaineritemliststackedlistenerclassname}(
    ${cppsupermainlistenerclassname}* mainListener, ${cppsuperstackedlistenerclassname}* parent,
    ${cppitemlistenerclassname}* itemListener)
:
${cppsuperstackedlistenerclassname}(mainListener, parent)
{
    fItemListener = itemListener;
}


${cppbulkcontaineritemliststackedlistenerclassname}::~${cppbulkcontaineritemliststackedlistenerclassname}()
{
}


bool
${cppbulkcontaineritemliststackedlistenerclassname}::Handle(const BJsonEvent& event)
{
    switch (event.EventType()) {
        case B_JSON_OBJECT_START:
            Push(new ${cppitemlistenerstackedlistenerclassname}(fMainListener, this, fItemListener));
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
${cppbulkcontaineritemliststackedlistenerclassname}::WillPop()
{
    fItemListener->Complete();
    return true;
}


""").substitute(jscom.uniondicts(naming.todict(), subtypenaming.todict())))


def writestackedlistenerimplementation(istate, schema):
    subtypenaming = CppParserSubTypeNaming(schema, istate.naming())

    if not istate.isimplementationhandledcppname(subtypenaming.cppmodelclassname()):
        istate.addimplementationhandledcppname(subtypenaming.cppmodelclassname())

        writestackedlistenertypedobjectimplementation(istate, schema)
        writestackedlistenertypedobjectlistimplementation(istate, schema)  # TODO; only if necessary.

        # now create the parser types for any subordinate objects descending.

        for propname, propmetadata in schema['properties'].items():
            if propmetadata['type'] == 'array':
                items = propmetadata['items']
                if items['type'] == jscom.JSON_TYPE_OBJECT:
                    writestackedlistenerimplementation(istate, items)
            elif propmetadata['type'] == 'object':
                writestackedlistenerimplementation(istate, propmetadata)


def writemainlistenerimplementation(istate, schema, supportbulkcontainer):
    outfile = istate.outputfile()
    naming = istate.naming()
    subtypenaming = CppParserSubTypeNaming(schema, istate.naming())

# super (abstract) listener

    outfile.write(
        string.Template("""
${cppsupermainlistenerclassname}::${cppsupermainlistenerclassname}()
{
    fStackedListener = NULL;
    fErrorStatus = B_OK;
}


${cppsupermainlistenerclassname}::~${cppsupermainlistenerclassname}()
{
}


void
${cppsupermainlistenerclassname}::HandleError(status_t status, int32 line, const char* message)
{
    if (message != NULL) {
        fprintf(stderr, "an error has arisen processing json for '${cpprootmodelclassname}'; %s\\n", message);
    } else {
        fprintf(stderr, "an error has arisen processing json for '${cpprootmodelclassname}'\\n");
    }
    fErrorStatus = status;
}


void
${cppsupermainlistenerclassname}::Complete()
{
}


status_t
${cppsupermainlistenerclassname}::ErrorStatus()
{
    return fErrorStatus;
}

void
${cppsupermainlistenerclassname}::SetStackedListener(
    ${cppsuperstackedlistenerclassname}* stackedListener)
{
    fStackedListener = stackedListener;
}

""").substitute(naming.todict()))

# single parser

    outfile.write(
        string.Template("""
${cppsinglemainlistenerclassname}::${cppsinglemainlistenerclassname}()
:
${cppsupermainlistenerclassname}()
{
    fTarget = NULL;
}


${cppsinglemainlistenerclassname}::~${cppsinglemainlistenerclassname}()
{
}


bool
${cppsinglemainlistenerclassname}::Handle(const BJsonEvent& event)
{
    if (fErrorStatus != B_OK)
       return false;

    if (fStackedListener != NULL)
        return fStackedListener->Handle(event);

    switch (event.EventType()) {
        case B_JSON_OBJECT_START:
        {
            ${subtype_cppstackedlistenerclassname}* nextListener = new ${subtype_cppstackedlistenerclassname}(
                this, NULL);
            fTarget = nextListener->Target();
            SetStackedListener(nextListener);
            break;
        }
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing top level for ${cpprootmodelclassname}");
            break;
    }


    return ErrorStatus() == B_OK;
}


${cpprootmodelclassname}*
${cppsinglemainlistenerclassname}::Target()
{
    return fTarget;
}

""").substitute(jscom.uniondicts(naming.todict(), subtypenaming.todict())))

    if supportbulkcontainer:

        # create a main listener that can work through the list of top level model objects and ping the listener

        outfile.write(
            string.Template("""
${cppbulkcontainermainlistenerclassname}::${cppbulkcontainermainlistenerclassname}(
    ${cppitemlistenerclassname}* itemListener) : ${cppsupermainlistenerclassname}()
{
    fItemListener = itemListener;
}


${cppbulkcontainermainlistenerclassname}::~${cppbulkcontainermainlistenerclassname}()
{
}


bool
${cppbulkcontainermainlistenerclassname}::Handle(const BJsonEvent& event)
{
    if (fErrorStatus != B_OK)
       return false;

    if (fStackedListener != NULL)
        return fStackedListener->Handle(event);

    switch (event.EventType()) {
        case B_JSON_OBJECT_START:
        {
            ${cppbulkcontainerstackedlistenerclassname}* nextListener =
                new ${cppbulkcontainerstackedlistenerclassname}(
                    this, NULL, fItemListener);
            SetStackedListener(nextListener);
            return true;
            break;
        }
        default:
            HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
                "illegal state - unexpected json event parsing top level for ${cppbulkcontainermainlistenerclassname}");
            break;
    }


    return ErrorStatus() == B_OK;
}

""").substitute(jscom.uniondicts(naming.todict(), subtypenaming.todict())))


def schematocppparser(inputfile, schema, outputdirectory, supportbulkcontainer):
    naming = CppParserNaming(schema)
    cppheaderleafname = naming.cpprootmodelclassname() + 'JsonListener.h'
    cppheaderfilename = os.path.join(outputdirectory, cppheaderleafname)
    cppimplementationfilename = os.path.join(outputdirectory, naming.cpprootmodelclassname() + 'JsonListener.cpp')

    with open(cppheaderfilename, 'w') as cpphfile:
        jscom.writetopcomment(cpphfile, os.path.split(inputfile)[1], 'Listener')
        guarddefname = 'GEN_JSON_SCHEMA_PARSER__%s_H' % (naming.cppsinglemainlistenerclassname().upper())

        cpphfile.write(
            string.Template("""
#ifndef ${guarddefname}
#define ${guarddefname}
""").substitute({'guarddefname': guarddefname}))

        cpphfile.write(
            string.Template("""            
#include <JsonEventListener.h>
#include <ObjectList.h>

#include "${cpprootmodelclassname}.h"

class ${cppsuperstackedlistenerclassname};

class ${cppsupermainlistenerclassname} : public BJsonEventListener {
friend class ${cppsuperstackedlistenerclassname};
public:
    ${cppsupermainlistenerclassname}();
    virtual ~${cppsupermainlistenerclassname}();

    void HandleError(status_t status, int32 line, const char* message);
    void Complete();
    status_t ErrorStatus();

protected:
    void SetStackedListener(
        ${cppsuperstackedlistenerclassname}* listener);
    status_t fErrorStatus;
    ${cppsuperstackedlistenerclassname}* fStackedListener;
};


/*! Use this listener when you want to parse some JSON data that contains
    just a single instance of ${cpprootmodelclassname}.
*/
class ${cppsinglemainlistenerclassname}
    : public ${cppsupermainlistenerclassname} {
friend class ${cppsuperstackedlistenerclassname};
public:
    ${cppsinglemainlistenerclassname}();
    virtual ~${cppsinglemainlistenerclassname}();

    bool Handle(const BJsonEvent& event);
    ${cpprootmodelclassname}* Target();

private:
    ${cpprootmodelclassname}* fTarget;
};

""").substitute(naming.todict()))

# class interface for concrete class of single listener


        # If bulk enveloping is selected then also output a listener and an interface
        # which can deal with call-backs.

        if supportbulkcontainer:
            cpphfile.write(
                string.Template("""    
/*! Concrete sub-classes of this class are able to respond to each
    ${cpprootmodelclassname}* instance as
    it is parsed from the bulk container.  When the stream is
    finished, the Complete() method is invoked.

    Note that the item object will be deleted after the Handle method
    is invoked.  The Handle method need not take responsibility
    for deleting the item itself.
*/         
class ${cppitemlistenerclassname} {
public:
    virtual bool Handle(${cpprootmodelclassname}* item) = 0;
    virtual void Complete() = 0;
};
""").substitute(naming.todict()))

            cpphfile.write(
                string.Template("""


/*! Use this listener, together with an instance of a concrete
    subclass of ${cppitemlistenerclassname}
    in order to parse the JSON data in a specific "bulk
    container" format.  Each time that an instance of
    ${cpprootmodelclassname}
    is parsed, the instance item listener will be invoked.
*/ 
class ${cppbulkcontainermainlistenerclassname}
    : public ${cppsupermainlistenerclassname} {
friend class ${cppsuperstackedlistenerclassname};
public:
    ${cppbulkcontainermainlistenerclassname}(
        ${cppitemlistenerclassname}* itemListener);
    ~${cppbulkcontainermainlistenerclassname}();

    bool Handle(const BJsonEvent& event);

private:
    ${cppitemlistenerclassname}* fItemListener;
};
""").substitute(naming.todict()))

        cpphfile.write('\n#endif // %s' % guarddefname)

    with open(cppimplementationfilename, 'w') as cppifile:
        istate = CppParserImplementationState(cppifile, naming)
        jscom.writetopcomment(cppifile, os.path.split(inputfile)[1], 'Listener')
        cppifile.write('#include "%s"\n' % cppheaderleafname)
        cppifile.write('#include <ObjectList.h>\n\n')
        cppifile.write('#include <stdio.h>\n\n')

        cppifile.write('// #pragma mark - private interfaces for the stacked listeners\n\n')

        writerootstackedlistenerinterface(istate)
        writegeneralstackedlistenerinterface(istate)
        writestringliststackedlistenerinterface(istate)
        writestackedlistenerinterface(istate, schema)

        if supportbulkcontainer:
            writebulkcontainerstackedlistenerinterface(istate, schema)

        cppifile.write('// #pragma mark - implementations for the stacked listeners\n\n')

        writerootstackedlistenerimplementation(istate)
        writegeneralstackedlistenerimplementation(istate)
        writestringliststackedlistenerimplementation(istate)
        writestackedlistenerimplementation(istate, schema)

        if supportbulkcontainer:
            writebulkcontainerstackedlistenerimplementation(istate, schema)

        cppifile.write('// #pragma mark - implementations for the main listeners\n\n')

        writemainlistenerimplementation(istate, schema, supportbulkcontainer)


def main():
    parser = argparse.ArgumentParser(description='Convert JSON schema to Haiku C++ Parsers')
    parser.add_argument('-i', '--inputfile', required=True, help='The input filename containing the JSON schema')
    parser.add_argument('--outputdirectory', help='The output directory where the C++ files should be written')
    parser.add_argument('--supportbulkcontainer', help='Produce a parser that deals with a bulk envelope of items',
                        action='store_true')

    args = parser.parse_args()

    outputdirectory = args.outputdirectory

    if not outputdirectory:
        outputdirectory = '.'

    with open(args.inputfile) as inputfile:
        schema = json.load(inputfile)
        schematocppparser(args.inputfile, schema, outputdirectory, args.supportbulkcontainer or False)

if __name__ == "__main__":
    main()