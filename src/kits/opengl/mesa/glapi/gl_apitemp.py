#!/usr/bin/python2

# (C) Copyright IBM Corporation 2004
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Authors:
#    Ian Romanick <idr@us.ibm.com>

from xml.sax import saxutils
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces

import gl_XML
import license
import sys, getopt

class PrintGlOffsets(gl_XML.FilterGLAPISpecBase):
	name = "gl_apitemp.py (from Mesa)"

	def __init__(self):
		gl_XML.FilterGLAPISpecBase.__init__(self)
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")

	def printFunction(self, f):
		p_string = ""
		o_string = ""
		t_string = ""
		comma = ""

		for p in f.parameterIterator():
			cast = ""

			if p.is_pointer:
				t = "%p"
				cast = "(const void *) "
			elif p.p_type_string == 'GLenum':
				t = "0x%x"
			elif p.p_type_string in ['GLfloat', 'GLdouble', 'GLclampf', 'GLclampd']:
				t = "%f"
			else:
				t = "%d"

			t_string = t_string + comma + t
			p_string = p_string + comma + p.name
			o_string = o_string + comma + cast + p.name
			comma = ", "


		if f.fn_return_type != 'void':
			dispatch = "RETURN_DISPATCH"
		else:
			dispatch = "DISPATCH"

		print 'KEYWORD1 %s KEYWORD2 NAME(%s)(%s)' \
			% (f.fn_return_type, f.name, f.get_parameter_string())
		print '{'
		if p_string == "":
			print '   %s(%s, (), (F, "gl%s();\\n"));' \
				% (dispatch, f.real_name, f.name)
		else:
			print '   %s(%s, (%s), (F, "gl%s(%s);\\n", %s));' \
				% (dispatch, f.real_name, p_string, f.name, t_string, o_string)
		print '}'
		print ''
		return

	def printRealHeader(self):
		print """
/*
 * This file is a template which generates the OpenGL API entry point
 * functions.  It should be included by a .c file which first defines
 * the following macros:
 *   KEYWORD1 - usually nothing, but might be __declspec(dllexport) on Win32
 *   KEYWORD2 - usually nothing, but might be __stdcall on Win32
 *   NAME(n)  - builds the final function name (usually add "gl" prefix)
 *   DISPATCH(func, args, msg) - code to do dispatch of named function.
 *                               msg is a printf-style debug message.
 *   RETURN_DISPATCH(func, args, msg) - code to do dispatch with a return value
 *
 * Here is an example which generates the usual OpenGL functions:
 *   #define KEYWORD1
 *   #define KEYWORD2
 *   #define NAME(func)  gl##func
 *   #define DISPATCH(func, args, msg)                           \\
 *          struct _glapi_table *dispatch = CurrentDispatch;     \\
 *          (*dispatch->func) args
 *   #define RETURN DISPATCH(func, args, msg)                    \\
 *          struct _glapi_table *dispatch = CurrentDispatch;     \\
 *          return (*dispatch->func) args
 *
 */


#if defined( NAME )
#ifndef KEYWORD1
#define KEYWORD1
#endif

#ifndef KEYWORD2
#define KEYWORD2
#endif

#ifndef DISPATCH
#error DISPATCH must be defined
#endif

#ifndef RETURN_DISPATCH
#error RETURN_DISPATCH must be defined
#endif

"""
		return

    

	def printInitDispatch(self):
		print """
#endif /* defined( NAME ) */

/*
 * This is how a dispatch table can be initialized with all the functions
 * we generated above.
 */
#ifdef DISPATCH_TABLE_NAME

#ifndef TABLE_ENTRY
#error TABLE_ENTRY must be defined
#endif

static _glapi_proc DISPATCH_TABLE_NAME[] = {"""
		keys = self.functions.keys()
		keys.sort()
		for k in keys:
			if k < 0: continue

			print '   TABLE_ENTRY(%s),' % (self.functions[k].name)

		print '   /* A whole bunch of no-op functions.  These might be called'
		print '    * when someone tries to call a dynamically-registered'
		print '    * extension function without a current rendering context.'
		print '    */'
		for i in range(1, 100):
			print '   TABLE_ENTRY(Unused),'

		print '};'
		print '#endif /* DISPATCH_TABLE_NAME */'
		print ''
		return

	def printAliasedTable(self):
		print """
/*
 * This is just used to silence compiler warnings.
 * We list the functions which are not otherwise used.
 */
#ifdef UNUSED_TABLE_NAME
static _glapi_proc UNUSED_TABLE_NAME[] = {"""

		keys = self.functions.keys()
		keys.sort()
		keys.reverse();
		for k in keys:
			f = self.functions[k]
			if f.fn_offset < 0:
				print '   TABLE_ENTRY(%s),' % (f.name)

		print '};'
		print '#endif /*UNUSED_TABLE_NAME*/'
		print ''
		return

	def printRealFooter(self):
		self.printInitDispatch()
		self.printAliasedTable()
		print"""
#undef KEYWORD1
#undef KEYWORD2
#undef NAME
#undef DISPATCH
#undef RETURN_DISPATCH
#undef DISPATCH_TABLE_NAME
#undef UNUSED_TABLE_NAME
#undef TABLE_ENTRY
"""
		return

def show_usage():
	print "Usage: %s [-f input_file_name]" % sys.argv[0]
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	dh = PrintGlOffsets()

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	dh.printHeader()
	parser.parse(f)
	dh.printFooter()

