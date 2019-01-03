
# =====================================
# Copyright 2017-2019, Andrew Lindesay
# Distributed under the terms of the MIT License.
# =====================================

# common material related to generation of schema-generated artifacts.

import datetime


# The possible JSON types
JSON_TYPE_STRING = "string"
JSON_TYPE_OBJECT = "object"
JSON_TYPE_ARRAY = "array"
JSON_TYPE_BOOLEAN = "boolean"
JSON_TYPE_INTEGER = "integer"
JSON_TYPE_NUMBER = "number"


# The possible C++ types
CPP_TYPE_STRING = "BString"
CPP_TYPE_ARRAY = "List"
CPP_TYPE_BOOLEAN = "bool"
CPP_TYPE_INTEGER = "int64"
CPP_TYPE_NUMBER = "double"


def uniondicts(d1, d2):
    d = dict(d1)
    d.update(d2)
    return d


def javatypetocppname(javaname):
    return javaname[javaname.rindex('.')+1:]


def propnametocppname(propname):
    return propname[0:1].upper() + propname[1:]


def propnametocppmembername(propname):
    return 'f' + propnametocppname(propname)


def propmetatojsoneventtypename(propmetadata):
    type = propmetadata['type']



def propmetadatatocpptypename(propmetadata):
    type = propmetadata['type']

    if type == JSON_TYPE_STRING:
        return CPP_TYPE_STRING
    if type == JSON_TYPE_BOOLEAN:
        return CPP_TYPE_BOOLEAN
    if type == JSON_TYPE_INTEGER:
        return CPP_TYPE_INTEGER
    if type == JSON_TYPE_NUMBER:
        return CPP_TYPE_NUMBER
    if type == JSON_TYPE_OBJECT:
        javatype = propmetadata['javaType']

        if not javatype or 0 == len(javatype):
            raise Exception('missing "javaType" field')

        return javatypetocppname(javatype)

    if type == JSON_TYPE_ARRAY:
        itemsmetadata = propmetadata['items']
        itemsjavatype = itemsmetadata['javaType']

        if not itemsjavatype or 0 == len(itemsjavatype):
            raise Exception('missing "javaType" field')

        return "%s <%s*, true>" % (CPP_TYPE_ARRAY, javatypetocppname(itemsjavatype))

    raise Exception('unknown json-schema type [' + type + ']')


def propmetadatatypeisscalar(propmetadata):
    type = propmetadata['type']
    return type == JSON_TYPE_BOOLEAN or type == JSON_TYPE_INTEGER or type == JSON_TYPE_NUMBER


def writetopcomment(f, inputfilename, variant):
    f.write((
                '/*\n'
                ' * Generated %s Object\n'
                ' * source json-schema : %s\n'
                ' * generated at : %s\n'
                ' */\n'
            ) % (variant, inputfilename, datetime.datetime.now().isoformat()))
