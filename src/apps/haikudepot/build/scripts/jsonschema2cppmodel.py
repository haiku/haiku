#!/usr/bin/python

# =====================================
# Copyright 2017-2025, Andrew Lindesay
# Distributed under the terms of the MIT License.
# =====================================

# This simple tool will read a JSON schema and will then generate
# some model objects that can be used to hold the data-structure
# in the C++ environment.

import json
import argparse
import os
import hdsjsonschemacommon
import ustache


HEADER_TEMPLATE = """
/*
 * Generated Model Object for {{cppname}}
 */
 
#ifndef GEN_JSON_SCHEMA_MODEL__{{cppnameupper}}_H
#define GEN_JSON_SCHEMA_MODEL__{{cppnameupper}}_H

#include <ObjectList.h>
#include <String.h>

{{#referencedclasscpptypes}}#include "{{.}}.h"
{{/referencedclasscpptypes}}


class {{cppname}} {
public:
    {{cppname}}();
    virtual ~{{cppname}}();

    void Reset();
{{#propertyarray}}{{#property.iscppscalartype}}
    {{property.cpptype}} {{property.cppname}}() const;
    void Set{{property.cppname}}({{property.cpptype}} value);
    void Set{{property.cppname}}Null();
    bool {{property.cppname}}IsNull() const;
{{/property.iscppscalartype}}{{#property.isarray}}
    void AddTo{{property.cppname}}({{property.items.cpptype}}* value);
    void Set{{property.cppname}}({{property.cpptype}}* value);
    int32 Count{{property.cppname}}() const;
    {{property.items.cpptype}}* {{property.cppname}}ItemAt(int32 index) const;
    bool {{property.cppname}}IsNull() const;
{{/property.isarray}}{{#property.iscppnonscalarnoncollectiontype}}
    {{property.cpptype}}* {{property.cppname}}() const;
    void Set{{property.cppname}}({{property.cpptype}}* value);
    void Set{{property.cppname}}Null();
    bool {{property.cppname}}IsNull() const;
{{/property.iscppnonscalarnoncollectiontype}}
{{/propertyarray}}

public:
{{#propertyarray}}    static const uint16 k{{property.cppname}}Bitmask;
{{/propertyarray}}

private:
    uint16 fHasValueBitmask;
{{#propertyarray}}    {{property.cpptype}}{{^property.iscppscalartype}}*{{/property.iscppscalartype}} {{property.cppmembername}};
{{/propertyarray}}
};

#endif // GEN_JSON_SCHEMA_MODEL__{{cppnameupper}}_H
"""

IMPLEMENTATION_TEMPLATE = """
/*
 * Generated Model Object for {{cppname}}
 */
 
#include "{{cppname}}.h"


{{#propertyarray}}/*static*/ const uint16 {{cppname}}::k{{property.cppname}}Bitmask = {{cppbitmaskexpression}};
{{/propertyarray}}


{{cppname}}::{{cppname}}()
    :
    fHasValueBitmask(0),
{{#propertyarray}}    {{property.cppmembername}}({{property.cppdefaultvalue}}){{^islast}},{{/islast}}
{{/propertyarray}}{
}


{{cppname}}::~{{cppname}}()
{
    Reset();
}


void
{{cppname}}::Reset()
{
{{#propertyarray}}{{#property.isstring}}    delete {{property.cppmembername}};
{{/property.isstring}}{{#property.isobject}}    delete {{property.cppmembername}};
{{/property.isobject}}{{#property.isarray}}
    if ({{property.cppmembername}} != NULL) {
        for (int i = {{property.cppmembername}}->CountItems() - 1; i >= 0; i--)
            delete {{property.cppmembername}}->ItemAt(i);
        delete {{property.cppmembername}};
    }
{{/property.isarray}}    {{property.cppmembername}} = {{property.cppdefaultvalue}};
{{/propertyarray}}
    fHasValueBitmask = 0;
}

{{#propertyarray}}{{#property.iscppscalartype}}{{property.cpptype}}
{{cppobjectname}}::{{property.cppname}}() const
{
    return {{property.cppmembername}};
}


void
{{cppobjectname}}::Set{{property.cppname}}({{property.cpptype}} value)
{
    fHasValueBitmask |= k{{property.cppname}}Bitmask;
    {{property.cppmembername}} = value;
}


void
{{cppobjectname}}::Set{{property.cppname}}Null()
{
    fHasValueBitmask &= ~k{{property.cppname}}Bitmask;
    {{property.cppmembername}} = {{property.cppdefaultvalue}};
}


bool
{{cppobjectname}}::{{property.cppname}}IsNull() const
{
    return 0 == (fHasValueBitmask & k{{property.cppname}}Bitmask);
}

{{/property.iscppscalartype}}{{#property.isarray}}void
{{cppobjectname}}::AddTo{{property.cppname}}({{property.items.cpptype}}* value)
{
    if ({{property.cppmembername}} == NULL)
        {{property.cppmembername}} = new {{property.cpptype}}();
    {{property.cppmembername}}->AddItem(value);
}


void
{{cppobjectname}}::Set{{property.cppname}}({{property.cpptype}}* value)
{
    if ({{property.cppmembername}} != NULL) {
        delete {{property.cppmembername}};
    }
    {{property.cppmembername}} = value;
}


int32
{{cppobjectname}}::Count{{property.cppname}}() const
{
    if ({{property.cppmembername}} == NULL)
        return 0;
    return {{property.cppmembername}}->CountItems();
}


{{property.items.cpptype}}*
{{cppobjectname}}::{{property.cppname}}ItemAt(int32 index) const
{
    return {{property.cppmembername}}->ItemAt(index);
}


bool
{{cppobjectname}}::{{property.cppname}}IsNull() const
{
    return {{property.cppmembername}} == NULL;
}

{{/property.isarray}}{{#property.iscppnonscalarnoncollectiontype}}{{property.cpptype}}*
{{cppobjectname}}::{{property.cppname}}() const
{
    return {{property.cppmembername}};
}


void
{{cppobjectname}}::Set{{property.cppname}}({{property.cpptype}}* value)
{
    {{property.cppmembername}} = value;
}


void
{{cppobjectname}}::Set{{property.cppname}}Null()
{
    if (!{{property.cppname}}IsNull()) {
        delete {{property.cppmembername}};
        {{property.cppmembername}} = NULL;
    }
}


bool
{{cppobjectname}}::{{property.cppname}}IsNull() const
{
    return {{property.cppmembername}} == NULL;
}

{{/property.iscppnonscalarnoncollectiontype}}
{{/propertyarray}}
"""


def write_models_for_schema(schema: dict[str, any], output_directory: str) -> None:

    def write_model_object(obj: dict[str, any]) -> None:
        cpp_name = obj["cppname"]
        cpp_header_filename = os.path.join(output_directory, cpp_name + '.h')
        cpp_implementation_filename = os.path.join(output_directory, cpp_name + '.cpp')

        with open(cpp_header_filename, 'w') as cpp_h_file:
            cpp_h_file.write(ustache.render(
                HEADER_TEMPLATE,
                obj,
                escape= lambda x: x))

        with open(cpp_implementation_filename, 'w') as cpp_i_file:
            cpp_i_file.write(ustache.render(
                IMPLEMENTATION_TEMPLATE,
                obj,
                escape= lambda x: x))

    for obj in hdsjsonschemacommon.collect_all_objects(schema):
        write_model_object(obj)


def main():
    parser = argparse.ArgumentParser(
        description='Convert JSON schema to Haiku C++ Models')
    parser.add_argument(
        '-i', '--inputfile',
        required=True,
        help='The input filename containing the JSON schema')
    parser.add_argument(
        '--outputdirectory',
        help='The output directory where the C++ files should be written')

    args = parser.parse_args()

    output_directory = args.outputdirectory

    if not output_directory:
        output_directory = '.'

    with open(args.inputfile) as inputfile:
        schema = json.load(inputfile)
        hdsjsonschemacommon.augment_schema(schema)
        write_models_for_schema(schema, output_directory)


if __name__ == "__main__":
    main()

