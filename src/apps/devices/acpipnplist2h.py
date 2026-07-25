#!/bin/python3
# Copyright 2026, Adrien Destugues <pulkomandy@pulkomandy.tk>
#
# Distributed under terms of the MIT license.
import csv
import sys


print("/*\tHaiku\t*/\n")
print("/*")
print(" This file is generated automatically. Don't edit.")
print("*/")
print("\n")
print("typedef struct { const char* VenId; const char* VenName; } idTable;")
print("idTable acpipnp_devids [] = {")

i = 0

for arg in sys.argv[1:]:
    sys.stderr.write(arg)
    with open(arg, newline='') as file:
        reader = csv.reader(file)
        for line in reader:
            key = line[1]
            value = line[0]
            if not (value.startswith("DO NOT USE -") or (value == "Company")):
                print("\t{")
                print(f"\t\t\"{key}\", \"{value}\"")
                print("\t},")
                i = i + 1

print("\t};")
print("// Use this value for loop control during searching:")
print(f"#define	ACPIPNP_DEVTABLE_LEN	{i}")
