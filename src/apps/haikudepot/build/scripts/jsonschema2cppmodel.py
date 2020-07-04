#!/usr/bin/python

# =====================================
# Copyright 2017-2020, Andrew Lindesay
# Distributed under the terms of the MIT License.
# =====================================

# This simple tool will read a JSON schema and will then generate
# some model objects that can be used to hold the data-structure
# in the C++ environment.

import json
import argparse
import os
import hdsjsonschemacommon as jscom
import string


def hasanylistproperties(schema):
    for propname, propmetadata in schema['properties'].items():
        if propmetadata['type'] == jscom.JSON_TYPE_ARRAY:
            return True
    return False


def writelistaccessors(outputfile, cppclassname, cppname, cppmembername, cppcontainertype):

    dict = {
        'cppclassname' : cppclassname,
        'cppname': cppname,
        'cppmembername': cppmembername,
        'cppcontainertype': cppcontainertype
    }

    outputfile.write(
        string.Template("""
void
${cppclassname}::AddTo${cppname}(${cppcontainertype}* value)
{
    if (${cppmembername} == NULL)
        ${cppmembername} = new BObjectList<${cppcontainertype}>();
    ${cppmembername}->AddItem(value);
}


void
${cppclassname}::Set${cppname}(BObjectList<${cppcontainertype}>* value)
{
   ${cppmembername} = value;
}


int32
${cppclassname}::Count${cppname}()
{
    if (${cppmembername} == NULL)
        return 0;
    return ${cppmembername}->CountItems();
}


${cppcontainertype}*
${cppclassname}::${cppname}ItemAt(int32 index)
{
    return ${cppmembername}->ItemAt(index);
}


bool
${cppclassname}::${cppname}IsNull()
{
    return ${cppmembername} == NULL;
}
""").substitute(dict))


def writelistaccessorsheader(outputfile, cppname, cppcontainertype):
    dict = {
        'cppname': cppname,
        'cppcontainertype': cppcontainertype
    }

    outputfile.write(
        string.Template("""    void AddTo${cppname}(${cppcontainertype}* value);
    void Set${cppname}(BObjectList<${cppcontainertype}>* value);
    int32 Count${cppname}();
    ${cppcontainertype}* ${cppname}ItemAt(int32 index);
    bool ${cppname}IsNull();
""").substitute(dict))


def writetakeownershipaccessors(outputfile, cppclassname, cppname, cppmembername, cpptype):

    dict = {
        'cppclassname': cppclassname,
        'cppname': cppname,
        'cppmembername': cppmembername,
        'cpptype': cpptype
    }

    outputfile.write(
        string.Template("""
${cpptype}*
${cppclassname}::${cppname}()
{
    return ${cppmembername};
}


void
${cppclassname}::Set${cppname}(${cpptype}* value)
{
    ${cppmembername} = value;
}


void
${cppclassname}::Set${cppname}Null()
{
    if (!${cppname}IsNull()) {
        delete ${cppmembername};
        ${cppmembername} = NULL;
    }
}


bool
${cppclassname}::${cppname}IsNull()
{
    return ${cppmembername} == NULL;
}
""").substitute(dict))


def writetakeownershipaccessorsheader(outputfile, cppname, cpptype):
    outputfile.write('    %s* %s();\n' % (cpptype, cppname))
    outputfile.write('    void Set%s(%s* value);\n' % (cppname, cpptype))
    outputfile.write('    void Set%sNull();\n' % cppname)
    outputfile.write('    bool %sIsNull();\n' % cppname)


def writescalaraccessors(outputfile, cppclassname, cppname, cppmembername, cpptype):

    dict = {
        'cppclassname': cppclassname,
        'cppname': cppname,
        'cppmembername': cppmembername,
        'cpptype': cpptype
    }

    outputfile.write(
        string.Template("""
${cpptype}
${cppclassname}::${cppname}()
{""").substitute(dict))

    if cpptype == jscom.CPP_TYPE_BOOLEAN:
        outputfile.write(string.Template("""
    if (${cppname}IsNull())
        return false;
""").substitute(dict))

    if cpptype == jscom.CPP_TYPE_INTEGER:
        outputfile.write(string.Template("""
    if (${cppname}IsNull())
        return 0;
""").substitute(dict))

    if cpptype == jscom.CPP_TYPE_NUMBER:
        outputfile.write(string.Template("""
    if (${cppname}IsNull())
        return 0.0;
""").substitute(dict))

    outputfile.write(string.Template("""
    return *${cppmembername};
}


void
${cppclassname}::Set${cppname}(${cpptype} value)
{
    if (${cppname}IsNull())
        ${cppmembername} = new ${cpptype}[1];
    ${cppmembername}[0] = value;
}


void
${cppclassname}::Set${cppname}Null()
{
    if (!${cppname}IsNull()) {
        delete ${cppmembername};
        ${cppmembername} = NULL;
    }
}


bool
${cppclassname}::${cppname}IsNull()
{
    return ${cppmembername} == NULL;
}
""").substitute(dict))


def writescalaraccessorsheader(outputfile, cppname, cpptype):
    outputfile.write(
        string.Template("""
    ${cpptype} ${cppname}();
    void Set${cppname}(${cpptype} value);
    void Set${cppname}Null();
    bool ${cppname}IsNull();
""").substitute({'cppname': cppname, 'cpptype': cpptype}))


def writeaccessors(outputfile, cppclassname, propname, propmetadata):
    type = propmetadata['type']

    if type == jscom.JSON_TYPE_ARRAY:
        writelistaccessors(outputfile,
                           cppclassname,
                           jscom.propnametocppname(propname),
                           jscom.propnametocppmembername(propname),
                           jscom.propmetadatatocpptypename(propmetadata['items']))
    elif jscom.propmetadatatypeisscalar(propmetadata):
        writescalaraccessors(outputfile,
                             cppclassname,
                             jscom.propnametocppname(propname),
                             jscom.propnametocppmembername(propname),
                             jscom.propmetadatatocpptypename(propmetadata))
    else:
        writetakeownershipaccessors(outputfile,
                                    cppclassname,
                                    jscom.propnametocppname(propname),
                                    jscom.propnametocppmembername(propname),
                                    jscom.propmetadatatocpptypename(propmetadata))


def writeaccessorsheader(outputfile, propname, propmetadata):
    type = propmetadata['type']

    if type == jscom.JSON_TYPE_ARRAY:
        writelistaccessorsheader(outputfile,
                                 jscom.propnametocppname(propname),
                                 jscom.propmetadatatocpptypename(propmetadata['items']))
    elif jscom.propmetadatatypeisscalar(propmetadata):
        writescalaraccessorsheader(outputfile,
                                   jscom.propnametocppname(propname),
                                   jscom.propmetadatatocpptypename(propmetadata))
    else:
        writetakeownershipaccessorsheader(outputfile,
                                          jscom.propnametocppname(propname),
                                          jscom.propmetadatatocpptypename(propmetadata))


def writedestructorlogicforlist(outputfile, propname, propmetadata):
    dict = {
        'cppmembername': jscom.propnametocppmembername(propname),
        'cpptype': jscom.javatypetocppname(propmetadata['items']['javaType'])
    }

    outputfile.write(
        string.Template("""        int32 count = ${cppmembername}->CountItems(); 
        for (i = 0; i < count; i++)
            delete ${cppmembername}->ItemAt(i);
""").substitute(dict))


def writedestructor(outputfile, cppname, schema):
    outputfile.write('\n\n%s::~%s()\n{\n' % (cppname, cppname))

    if hasanylistproperties(schema):
        outputfile.write('    int32 i;\n\n')

    for propname, propmetadata in schema['properties'].items():
        propmembername = jscom.propnametocppmembername(propname)

        outputfile.write('    if (%s != NULL) {\n' % propmembername)

        if propmetadata['type'] == jscom.JSON_TYPE_ARRAY:
            writedestructorlogicforlist(outputfile, propname, propmetadata)

        outputfile.write((
            '        delete %s;\n'
        ) % propmembername)

        outputfile.write('    }\n\n')

    outputfile.write('}\n')


def writeconstructor(outputfile, cppname, schema):
    outputfile.write('\n\n%s::%s()\n{\n' % (cppname, cppname))

    for propname, propmetadata in schema['properties'].items():
        outputfile.write('    %s = NULL;\n' % jscom.propnametocppmembername(propname))

    outputfile.write('}\n')


def writeheaderincludes(outputfile, properties):
    for propname, propmetadata in properties.items():
        jsontype = propmetadata['type']
        javatype = None

        if jsontype == jscom.JSON_TYPE_OBJECT:
            javatype = propmetadata['javaType']

        if jsontype == jscom.JSON_TYPE_ARRAY:
            javatype = propmetadata['items']['javaType']

        if javatype is not None:
            outputfile.write('#include "%s.h"\n' % jscom.javatypetocppname(javatype))


def schematocppmodels(inputfile, schema, outputdirectory):
    type = schema['type']
    if type != jscom.JSON_TYPE_OBJECT:
        raise Exception('expecting object, but was [' + type + ']')

    javatype = schema['javaType']

    if not javatype or 0 == len(javatype):
        raise Exception('missing "javaType" field')

    cppclassname = jscom.javatypetocppname(javatype)
    cpphfilename = os.path.join(outputdirectory, cppclassname + '.h')
    cppifilename = os.path.join(outputdirectory, cppclassname + '.cpp')

    with open(cpphfilename, 'w') as cpphfile:

        jscom.writetopcomment(cpphfile, os.path.split(inputfile)[1], 'Model')
        guarddefname = 'GEN_JSON_SCHEMA_MODEL__%s_H' % (cppclassname.upper())

        cpphfile.write(string.Template("""
#ifndef ${guarddefname}
#define ${guarddefname}

#include <ObjectList.h>
#include <String.h>

""").substitute({'guarddefname': guarddefname}))

        writeheaderincludes(cpphfile, schema['properties'])

        cpphfile.write(string.Template("""
class ${cppclassname} {
public:
    ${cppclassname}();
    virtual ~${cppclassname}();


""").substitute({'cppclassname': cppclassname}))

        for propname, propmetadata in schema['properties'].items():
            writeaccessorsheader(cpphfile, propname, propmetadata)
            cpphfile.write('\n')

        # Now add the instance variables for the object as well.

        cpphfile.write('private:\n')

        for propname, propmetadata in schema['properties'].items():
            cpphfile.write('    %s* %s;\n' % (
                jscom.propmetadatatocpptypename(propmetadata),
                jscom.propnametocppmembername(propname)))

        cpphfile.write((
            '};\n\n'
            '#endif // %s'
        ) % guarddefname)

    with open(cppifilename, 'w') as cppifile:

        jscom.writetopcomment(cppifile, os.path.split(inputfile)[1], 'Model')

        cppifile.write('#include "%s.h"\n' % cppclassname)

        writeconstructor(cppifile, cppclassname, schema)
        writedestructor(cppifile, cppclassname, schema)

        for propname, propmetadata in schema['properties'].items():
            writeaccessors(cppifile, cppclassname, propname, propmetadata)
            cppifile.write('\n')

    # Now write out any subordinate structures.

    for propname, propmetadata in schema['properties'].items():
        jsontype = propmetadata['type']

        if jsontype == jscom.JSON_TYPE_ARRAY:
            arraySchema = propmetadata['items']
            if arraySchema['type'] == jscom.JSON_TYPE_OBJECT:
                schematocppmodels(inputfile, arraySchema, outputdirectory)

        if jsontype == jscom.JSON_TYPE_OBJECT:
            schematocppmodels(inputfile, propmetadata, outputdirectory)


def main():
    parser = argparse.ArgumentParser(description='Convert JSON schema to Haiku C++ Models')
    parser.add_argument('-i', '--inputfile', required=True, help='The input filename containing the JSON schema')
    parser.add_argument('--outputdirectory', help='The output directory where the C++ files should be written')

    args = parser.parse_args()

    outputdirectory = args.outputdirectory

    if not outputdirectory:
        outputdirectory = '.'

    with open(args.inputfile) as inputfile:
        schema = json.load(inputfile)
        schematocppmodels(args.inputfile, schema, outputdirectory)

if __name__ == "__main__":
    main()

