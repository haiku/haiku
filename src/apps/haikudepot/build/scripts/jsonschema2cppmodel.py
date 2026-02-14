#!/usr/bin/python

# =====================================
# Copyright 2017-2026, Andrew Lindesay
# Distributed under the terms of the MIT License.
# =====================================

"""\
This is a command line tool which is able to take a JSON Schema file as input
and will generate the `.h` and `.cpp` C++ source files for model classes that
represent the schema. The classes will appear in the output file in the correct
order according to their relationship in the schema.
"""

import argparse
import json
import pathlib
from pathlib import Path

import jsonschemacommon
import ustache


def main():
    parser = argparse.ArgumentParser(
        description="Convert JSON schema to Haiku C++ Models")

    parser.add_argument(
        "--jsonschemafile",
        dest = "jsonschemafile",
        required=True,
        help="The input filename containing the JSON schema")

    parser.add_argument(
        "--outputdirectory",
        dest = "outputdirectory",
        required=True,
        help="The output filename where the C++ header and implementation file should be written")

    args = parser.parse_args()
    schema_path = Path(args.jsonschemafile).absolute()

    if not schema_path.is_file():
        raise RuntimeError(f"The input file [{schema_path}] does not exist")

    output_directory_path = Path(args.outputdirectory).absolute()
    output_directory_path.mkdir(parents=True, exist_ok=True)

    with open(schema_path) as input_file:
        schema = json.load(input_file)

    jsonschemacommon.augment_schema(schema)
    object_schemas = jsonschemacommon.collect_all_objects(schema)

    def output_to_path(output_path: pathlib.Path):
        template_type = jsonschemacommon.path_to_extension_type(output_path)
        template = jsonschemacommon.load_template("model", template_type)
        template_model = {
            "objects": object_schemas,
            "outputfilename": output_path.name,
            "outputheaderfile": output_path.stem + ".h",
            "outputfilestemupper": output_path.stem.upper()
        }

        with open(output_path, 'w') as out_file:
            out_file.write(ustache.render(
                template,
                template_model,
                escape= lambda x: x))

    cpp_name = schema["cppname"]
    output_to_path(output_directory_path / f"{cpp_name}Model.h")
    output_to_path(output_directory_path / f"{cpp_name}Model.cpp")


if __name__ == "__main__":
    main()

