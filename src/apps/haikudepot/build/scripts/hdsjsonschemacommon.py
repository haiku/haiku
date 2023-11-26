
# =====================================
# Copyright 2017-2023, Andrew Lindesay
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
CPP_TYPE_ARRAY = "BObjectList"
CPP_TYPE_BOOLEAN = "bool"
CPP_TYPE_INTEGER = "int64"
CPP_TYPE_NUMBER = "double"


# The possible C++ default values
CPP_DEFAULT_STRING = "NULL"
CPP_DEFAULT_OBJECT = "NULL"
CPP_DEFAULT_BOOLEAN = "false"
CPP_DEFAULT_INTEGER = "0"
CPP_DEFAULT_NUMBER = "0.0"


def uniondicts(d1, d2):
    d = dict(d1)
    d.update(d2)
    return d


def javatypetocppname(javaname):
    return javaname[javaname.rindex('.')+1:]


def propnametocppname(propname):
    return propname[0:1].upper() + propname[1:]


def propnametocppmembername(propname):
    return "f" + propnametocppname(propname)

def propmetadatatocppdefaultvalue(propmetadata):
    type = propmetadata['type']

    if type == JSON_TYPE_STRING:
        return CPP_DEFAULT_STRING
    if type == JSON_TYPE_BOOLEAN:
        return CPP_DEFAULT_BOOLEAN
    if type == JSON_TYPE_INTEGER:
        return CPP_DEFAULT_INTEGER
    if type == JSON_TYPE_NUMBER:
        return CPP_DEFAULT_NUMBER
    if type == JSON_TYPE_OBJECT:
        return CPP_DEFAULT_OBJECT

    raise Exception('unknown json-schema type [' + type + ']')

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
        itemstype = itemsmetadata['type']

        if not itemstype or 0 == len(itemstype):
            raise Exception('missing "type" field')

        if itemstype == JSON_TYPE_OBJECT:
            itemsjavatype = itemsmetadata['javaType']
            if not itemsjavatype or 0 == len(itemsjavatype):
                raise Exception('missing "javaType" field')
            return "%s<%s>" % (CPP_TYPE_ARRAY, javatypetocppname(itemsjavatype))

        if itemstype == JSON_TYPE_STRING:
            return "%s<%s>" % (CPP_TYPE_ARRAY, CPP_TYPE_STRING)

        raise Exception('unsupported type [%s]' % itemstype)

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

def collect_all_objects(schema: dict[str, any]) -> list[dict[str, any]]:
    assembly = dict[str, dict[str, any]]()

    def accumulate_all_objects(obj: dict[str, any]) -> None:
        assembly[obj["cppname"]] = obj

        for prop_name, prop in obj['properties'].items():
            if JSON_TYPE_ARRAY == prop["type"]:
                array_items = prop['items']

                if JSON_TYPE_OBJECT == array_items["type"] and not array_items["cppname"] in assembly:
                    accumulate_all_objects(array_items)

            if JSON_TYPE_OBJECT == prop["type"] and not prop["cppname"] in assembly:
                accumulate_all_objects(prop)

    accumulate_all_objects(schema)
    result = list(assembly.values())
    result.sort(key= lambda v: v["cppname"])

    return result


def augment_schema(schema: dict[str, any]) -> None:
    """This function will take the data from the JSON schema and will overlay any
    specific information required for rendering the templates.
    """

    def derive_cpp_classname(obj: dict[str, any]) -> str:
        obj_type = obj['type']

        if obj_type != JSON_TYPE_OBJECT:
            raise Exception('expecting object, but was [' + obj_type + ']')

        java_type = obj['javaType']

        return javatypetocppname(java_type)

    def augment_property(prop: dict[str, any]) -> None:
        prop_type = prop['type']

        prop["cpptype"] = propmetadatatocpptypename(prop)

        is_scalar_type = propmetadatatypeisscalar(prop)
        is_array_type = (prop_type == JSON_TYPE_ARRAY)

        prop["iscppscalartype"] = is_scalar_type
        prop["isarray"] = is_array_type
        prop["isstring"] = JSON_TYPE_STRING == prop_type
        prop["isboolean"] = JSON_TYPE_BOOLEAN == prop_type
        prop["isnumber"] = JSON_TYPE_NUMBER == prop_type
        prop["isinteger"] = JSON_TYPE_INTEGER == prop_type
        prop["isobject"] = JSON_TYPE_OBJECT == prop_type
        prop["iscppnonscalarnoncollectiontype"] = not is_scalar_type and not is_array_type
        prop["toplevelcppname"] = top_level_cpp_name

        if prop_type != "array":
            prop["cppdefaultvalue"] = propmetadatatocppdefaultvalue(prop)

        if not prop_type or 0 == len(prop_type):
            raise Exception('missing "type" field')

        if JSON_TYPE_ARRAY == prop_type:
            array_items = prop['items']
            array_items_type = array_items["type"]

            if array_items_type == JSON_TYPE_OBJECT:
                augment_object(array_items)
            else:
                augment_property(array_items)

        if JSON_TYPE_OBJECT == prop_type:
            augment_object(prop)

    def augment_object(obj: dict[str, any]) -> None:

        def collect_referenced_class_names() -> list[str]:
            result = set()

            for _, prop in obj['properties'].items():
                if prop['type'] == JSON_TYPE_ARRAY:
                    array_items = prop['items']
                    if array_items['type'] == JSON_TYPE_OBJECT:
                        result.add(array_items['cppname'])
                if prop['type'] == JSON_TYPE_OBJECT:
                    result.add(prop['cppname'])

            result_ordered = list(result)
            result_ordered.sort()
            return result_ordered

        def has_any_list_properties() -> bool:
            for _, prop in obj['properties'].items():
                if prop['type'] == JSON_TYPE_ARRAY:
                    return True
            return False

        obj_cpp_classname = derive_cpp_classname(obj)
        obj["cppname"] = obj_cpp_classname
        obj["cppnameupper"] = obj_cpp_classname.upper()
        obj["cpptype"] = obj_cpp_classname
        obj["hasanylistproperties"] = has_any_list_properties()

        # allows the object to know the top level object that contains it
        obj["toplevelcppname"] = top_level_cpp_name

        properties = obj['properties'].items()

        for prop_name, prop in properties:
            prop["cppname"] = propnametocppname(prop_name)
            prop["cppmembername"] = propnametocppmembername(prop_name)
            augment_property(prop)

        # mustache is not able to iterate over a dictionary; it can only
        # iterate over a list where each item in the list has some properties.

        property_array = sorted(
            [{
                "name": k,
                "property": v,
                "cppobjectname": obj_cpp_classname
                    # ^ this is the name of the containing object
            } for k,v in properties],
            key= lambda item: item["name"]
        )

        if 0 != len(property_array):
            property_array[0]["isfirst"] = True
            property_array[len(property_array) - 1]["islast"] = True

        obj["propertyarray"] = property_array
        obj["referencedclasscpptypes"] = collect_referenced_class_names()

    top_level_cpp_name = derive_cpp_classname(schema)
    augment_object(schema)