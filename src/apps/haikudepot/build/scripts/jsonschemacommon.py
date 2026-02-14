# =====================================
# Copyright 2017-2026, Andrew Lindesay
# Distributed under the terms of the MIT License.
# =====================================

"""\
This file contains classes and helper functions that are used in the generation
of `.h` and `.cpp` C++ files based on a JSON Schema. The schemas are intended
for use in a Java setting, but are adapted by this logic for use with C++ and
types that are typical for the Haiku environment.
"""

import datetime
import pathlib
from typing import Any
from enum import Enum, auto

# The possible JSON types
JSON_TYPE_STRING = "string"
JSON_TYPE_OBJECT = "object"
JSON_TYPE_ARRAY = "array"
JSON_TYPE_BOOLEAN = "boolean"
JSON_TYPE_INTEGER = "integer"
JSON_TYPE_NUMBER = "number"


# The possible C++ types
CPP_TYPE_STRING = "BString"
CPP_TYPE_ARRAY = "std::vector"
CPP_TYPE_BOOLEAN = "bool"
CPP_TYPE_INTEGER = "int64"
CPP_TYPE_NUMBER = "double"


# The possible C++ default values
CPP_DEFAULT_STRING = "\"\""
CPP_DEFAULT_BOOLEAN = "false"
CPP_DEFAULT_INTEGER = "0"
CPP_DEFAULT_NUMBER = "0.0"


class TemplateType(Enum):
    """Type of template to load."""
    HEADER = auto()
    IMPLEMENTATION = auto()


class CppClassNode:
    """An object representing a C++ class for establishing a dependency graph."""
    def __init__(self, cpp_name: str, schema: dict[str, Any], dependencies: set):
        self.cpp_name = cpp_name
        self.schema = schema
        self.dependencies = dependencies


def java_type_to_cpp_name(java_name: str) -> str:
    return java_name[java_name.rindex('.') + 1:]


def prop_name_to_cpp_name(prop_name: str) -> str:
    return prop_name[0:1].upper() + prop_name[1:]


def prop_name_to_cpp_member_name(prop_name: str) -> str:
    return "f" + prop_name_to_cpp_name(prop_name)


def prop_metadata_to_cpp_default_value(prop_metadata: dict[str, Any]) -> str:
    """Converts the property data into a C++ default for the field.

    This function will look at the metadata for the property and decide what a
    generic default value for that type might be.

    Returns: The function will return a string representation of the C++ value.
    """
    type = prop_metadata['type']

    if type == JSON_TYPE_STRING:
        return CPP_DEFAULT_STRING
    if type == JSON_TYPE_BOOLEAN:
        return CPP_DEFAULT_BOOLEAN
    if type == JSON_TYPE_INTEGER:
        return CPP_DEFAULT_INTEGER
    if type == JSON_TYPE_NUMBER:
        return CPP_DEFAULT_NUMBER
    if type == JSON_TYPE_OBJECT:
        cpp_type = prop_metadata["cpptype"]
        return f"{cpp_type}Ref()"
    if type == JSON_TYPE_ARRAY:
        items_cpptype = prop_metadata["items"]["cpptype"]
        return f"{CPP_TYPE_ARRAY}<{items_cpptype}>()"

    raise Exception(f"unknown json-schema type [{type}]")

def prop_metadata_to_cpp_type_name(prop_metadata: dict[str, Any]) -> str:
    """Introspects data metadata for a property and returns its type in C++"""
    type_ = prop_metadata['type']

    if type_ == JSON_TYPE_STRING:
        return CPP_TYPE_STRING
    if type_ == JSON_TYPE_BOOLEAN:
        return CPP_TYPE_BOOLEAN
    if type_ == JSON_TYPE_INTEGER:
        return CPP_TYPE_INTEGER
    if type_ == JSON_TYPE_NUMBER:
        return CPP_TYPE_NUMBER
    if type_ == JSON_TYPE_OBJECT:
        java_type = prop_metadata['javaType']

        if not java_type or 0 == len(java_type):
            raise Exception('missing "javaType" field')

        return java_type_to_cpp_name(java_type)

    if type_ == JSON_TYPE_ARRAY:
        items_cpptype = prop_metadata["items"]["cpptype"]
        return f"{CPP_TYPE_ARRAY}<{items_cpptype}>"

    raise Exception(f"unknown json-schema type [{type_}]")


def prop_metadata_type_is_scalar(prop_metadata: dict[str, Any]) -> bool:
    type_ = prop_metadata['type']
    return type_ == JSON_TYPE_BOOLEAN or type_ == JSON_TYPE_INTEGER or type_ == JSON_TYPE_NUMBER


def write_top_comment(f, input_filename: str, variant: str) -> None:
    f.write((
                '/*\n'
                ' * Generated %s Object\n'
                ' * source json-schema : %s\n'
                ' * generated at : %s\n'
                ' */\n'
            ) % (variant, input_filename, datetime.datetime.now().isoformat()))


def _to_cpp_class_dependency_tree(schema: dict[str, Any]) -> CppClassNode | None:
    """Builds up a dependency tree of to-be C++ classes

    The output of this is a dependency tree of C++ classes from which it is
    possible to get the order that the classes need to be written into the
    header and implementation files.
    """

    all_nodes: list[CppClassNode] = []

    def to_node(sub_schema: dict[str, Any]):
        if sub_schema["type"] == JSON_TYPE_ARRAY:
            return to_node(sub_schema["items"])
        if sub_schema["type"] == JSON_TYPE_OBJECT:
            cpp_name = sub_schema["cppname"]

            existing_class = next((c for c in all_nodes if c.cpp_name == cpp_name), None)
            if existing_class:
                return existing_class

            node = CppClassNode(cpp_name=cpp_name, schema=sub_schema, dependencies=set())
            all_nodes.append(node)

            dependencies = [to_node(property_schema) for property_schema in sub_schema["properties"].values()]
            node.dependencies = sorted([d for d in dependencies if d], key = lambda d: d.cpp_name)

            return node
        return None

    return to_node(schema)


def _to_object_sub_schemas_in_dependency_order(root_node: CppClassNode) -> list[dict[str, Any]]:
    """Returns a list of the schemas representing objects such that any dependencies appear first."""

    result: list[dict[str, Any]] = []

    def walk(node: CppClassNode):
        for d in node.dependencies:
            walk(d)
        result.append(node.schema)

    walk(root_node)

    return result


def collect_all_objects(schema: dict[str, Any]) -> list[dict[str, Any]]:
    """Walks the schema and extracts all definitions of objects.

    Returns:
        A list of schema structure which defines the class. The list will be in
        order so that latter classes are able to have dependency on earlier
        classes.
    """

    root_node = _to_cpp_class_dependency_tree(schema)

    if not root_node:
        raise RuntimeError("it was not possible to extract a tree from the schema data")

    return _to_object_sub_schemas_in_dependency_order(root_node)


def augment_schema(schema: dict[str, Any]) -> None:
    """This function will take the data from the JSON schema and will overlay any
    specific information required for rendering the templates.
    """

    def augment_property_type(prop: dict[str, Any]) -> None:
        prop_type = prop['type']

        if not prop_type or 0 == len(prop_type):
            raise Exception('missing "type" field')

        # For the object type, a `BReference` is used as the member variable
        # type but the actual wrapped type is still required for constructors.

        if prop_type == JSON_TYPE_OBJECT:
            prop["cpptyperaw"] = prop_metadata_to_cpp_type_name(prop)
            prop["cpptype"] = prop_metadata_to_cpp_type_name(prop) + "Ref"
        else:
            prop["cpptype"] = prop_metadata_to_cpp_type_name(prop)

        is_scalar_type = prop_metadata_type_is_scalar(prop)
        is_array_type = (prop_type == JSON_TYPE_ARRAY)

        prop["iscppscalartype"] = is_scalar_type
        prop["isarray"] = is_array_type
        prop["isstring"] = JSON_TYPE_STRING == prop_type
        prop["isboolean"] = JSON_TYPE_BOOLEAN == prop_type
        prop["isnumber"] = JSON_TYPE_NUMBER == prop_type
        prop["isinteger"] = JSON_TYPE_INTEGER == prop_type
        prop["isobject"] = JSON_TYPE_OBJECT == prop_type
        prop["isobjectorarray"] = prop_type in [JSON_TYPE_OBJECT, JSON_TYPE_ARRAY]
        prop["iscppnonscalarnoncollectiontype"] = not is_scalar_type and not is_array_type

    def derive_cpp_classname(obj: dict[str, Any]) -> str:
        obj_type = obj['type']

        if obj_type != JSON_TYPE_OBJECT:
            raise Exception('expecting object, but was [' + obj_type + ']')

        java_type = obj['javaType']

        return java_type_to_cpp_name(java_type)

    def augment_property(prop: dict[str, Any]) -> None:

        if JSON_TYPE_ARRAY == prop["type"]:
            array_items = prop['items']
            array_items_type = array_items["type"]

            if array_items_type == JSON_TYPE_OBJECT:
                augment_object(array_items)
            elif array_items_type == JSON_TYPE_STRING:
                augment_property(array_items)
            else:
                raise Exception(f"bad type for array items [{array_items_type}]")

        augment_property_type(prop)
        prop["cppdefaultvalue"] = prop_metadata_to_cpp_default_value(prop)

        if JSON_TYPE_OBJECT == prop["type"]:
            augment_object(prop)

    def augment_object(obj: dict[str, Any]) -> None:
        def has_any_list_properties() -> bool:
            for _, prop in obj['properties'].items():
                if prop['type'] == JSON_TYPE_ARRAY:
                    return True
            return False

        augment_property_type(obj)

        obj_cpp_classname = derive_cpp_classname(obj)
        obj["cppname"] = obj_cpp_classname
        obj["cppnameupper"] = obj_cpp_classname.upper()
        obj["hasanylistproperties"] = has_any_list_properties()

        # allows the object to know the top level object that contains it
        obj["toplevelcppname"] = top_level_cpp_name

        properties = obj['properties'].items()

        for prop_name, prop in properties:
            prop["cppname"] = prop_name_to_cpp_name(prop_name)
            prop["cppmembername"] = prop_name_to_cpp_member_name(prop_name)
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

        for i in range(len(property_array)):
            property_array[i]["cppbitmaskexpression"] = "(1 << %d)" % i

        if 0 != len(property_array):
            property_array[0]["isfirst"] = True
            property_array[len(property_array) - 1]["islast"] = True

        obj["propertyarray"] = property_array

    top_level_cpp_name = derive_cpp_classname(schema)
    augment_object(schema)


def path_to_extension_type(file_path: pathlib.Path) -> TemplateType:
    if ".h" == file_path.suffix:
        return TemplateType.HEADER
    if ".cpp" == file_path.suffix:
        return TemplateType.IMPLEMENTATION
    raise RuntimeError(f"unknown extension [{file_path.suffix}]")


def load_template(template_name: str, template_type: TemplateType) -> str:
    """Loads the template from the `./template` directory."""

    def template_type_path_component() -> str:
        if template_type == TemplateType.HEADER:
            return "header"
        if template_type == TemplateType.IMPLEMENTATION:
            return "implementation"
        raise RuntimeError(f"unknown template type [{str(template_type)}")

    script_directory = pathlib.Path(__file__).parent.resolve()
    template_path = script_directory / "template" / f"{template_name}{template_type_path_component()}.mustache"

    with open(template_path, "r") as f:
        return f.read()
