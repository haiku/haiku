#!python

# run from trunk/ and > errors.py

import re

errors = {}


errors_h = open("headers/os/support/Errors.h", "r")

bases = {}

for line in errors_h:

    match = re.match(r'#\s?define\s+(?P<define>\w+)\s+(?P<expr>.+)', line)
    if match:
        define = match.groupdict().get("define")
        expr = match.groupdict().get("expr")
        value = None
        #print "==== %s ==== %s" % (define, expr)
        if expr == "INT_MIN":
            value = 0x80000000
        if expr == "(-1)":
            value = -1
            expr = ""
        if expr == "((int)0)":
            value = 0
            expr = ""

        m = re.match(r"B_TO_POSIX_ERROR\((?P<expr>.*)\)", expr)
        if m:
            # strip the macro
            expr = m.group("expr")
            #print expr

        m = re.match(r"B_FROM_POSIX_ERROR\((?P<expr>.*)\)", expr)
        if m:
            # strip the macro
            expr = m.group("expr")
            #print expr

        m = re.match(r"\((?P<expr>.*)\)", expr)
        if m:
            # strip the parens
            expr = m.group("expr")
            #print expr

        m = re.match(r"(?P<expr>\w+)$", expr)
        if m:
            # strip the macro
            token = m.group("expr")
            if token in errors:
                value = errors[token]["value"]
                #print "%s -> %s = %s" % (define, token, value)

        m = re.match(r"(?P<base>.*_BASE)\s*\+\s*(?P<offset>\w+)$", expr)
        if m:
            base = m.group("base")
            if base not in bases:
                raise "error"
            b = bases[base]
            o = int(m.group("offset"), 0)
            value = b + o
            #print "value: %x + %x = %x = %d" % (b, o, value, value)

        if value is None:
            print "Value unknown for %s" % (define)
            exit(1)

        if re.match(r".*_BASE", define):
            #print "base: %s=%s" % (define, value)
            bases[define] = value

        else:
            #print "%s=%s :%s" % (define, expr, value)
            errors[define] = {"expr": expr, "value": value, "str": ""}


errors_h.close()

#print bases
#print errors
#print errors['EILSEQ']

#exit(0)

strerror_c = open("src/system/libroot/posix/string/strerror.c", "r")

defines = []

for line in strerror_c:

    match = re.match(r'\t*case (?P<define>\w+):', line)
    if match:
        define = match.groupdict().get("define")
        defines.append(define)
    match = re.match(r'\t*//\s+(?P<define>\w+):*$', line)
    if match:
        define = match.groupdict().get("define")
        defines.append(define)

    match = re.match(r'\t*return "(?P<str>.+)";', line)
    if match:
        str = match.groupdict().get("str")
        #print defines
        for d in defines:
            #print d
            if errors[d]:
                #print errors[d]#['str']
                errors[d]['str'] = str
            else:
                print "not found: %s" % (d)
                exit(1)
        defines = []
        #errors[define] = "plop"

strerror_c.close()

# Some fixup
errors['B_OK']['str'] = "No Error"
errors['B_NO_ERROR']['str'] = "No Error"
errors['B_DONT_DO_THAT']['str'] = "Don't do that!"

#print errors
print "errors = {"
for e in errors:
    print "'%s': %s," % (e, errors[e])
print "}"
