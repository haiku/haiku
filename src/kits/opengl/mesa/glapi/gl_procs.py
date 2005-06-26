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

import license
import gl_XML
import sys, getopt

class PrintGlProcs(gl_XML.FilterGLAPISpecBase):
	name = "gl_procs.py (from Mesa)"

	def __init__(self, long_strings):
		self.long_strings = long_strings
		gl_XML.FilterGLAPISpecBase.__init__(self)
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")


	def printRealHeader(self):
		print '/* This file is only included by glapi.c and is used for'
		print ' * the GetProcAddress() function'
		print ' */'
		print ''
		print 'typedef struct {'
		print '    GLint Name_offset;'
		print '#ifdef NEED_FUNCTION_POINTER'
		print '    _glapi_proc Address;'
		print '#endif'
		print '    GLuint Offset;'
		print '} glprocs_table_t;'
		print ''
		print '#ifdef NEED_FUNCTION_POINTER'
		print '#  define NAME_FUNC_OFFSET(n,f,o) { n , (_glapi_proc) f , o }'
		print '#else'
		print '#  define NAME_FUNC_OFFSET(n,f,o) { n , o }'
		print '#endif'
		print ''
		return

	def printRealFooter(self):
		print ''
		print '#undef NAME_FUNC_OFFSET'
		return

	def printFunctionString(self, f):
		if self.long_strings:
			print '    "gl%s\\0"' % (f.name)
		else:
			print "    'g','l',",
			for c in f.name:
				print "'%s'," % (c),
			
			print "'\\0',"

	def printFunctionOffset(self, f, offset_of_name):
		print '    NAME_FUNC_OFFSET( % 5u, gl%s, _gloffset_%s ),' % (offset_of_name, f.name, f.real_name)


	def printFunctions(self):
		print ''
		if self.long_strings:
			print 'static const char gl_string_table[] ='
		else:
			print 'static const char gl_string_table[] = {'

		keys = self.functions.keys()
		keys.sort()
		for k in keys:
			if k < 0: continue
			self.printFunctionString(self.functions[k])

		keys.reverse()
		for k in keys:
			if k >= -1: continue
			self.printFunctionString(self.functions[k])

		if self.long_strings:
			print '    ;'
		else:
			print '};'

		print ''
		print 'static const glprocs_table_t static_functions[] = {'

		keys = self.functions.keys()
		keys.sort()
		base_offset = 0
		for k in keys:
			if k < 0: continue
			self.printFunctionOffset(self.functions[k], base_offset)

			# The length of the function's name, plus 2 for "gl",
			# plus 1 for the NUL.

			base_offset += len(self.functions[k].name) + 3

		keys.reverse()
		for k in keys:
			if k >= -1: continue
			self.printFunctionOffset(self.functions[k], base_offset)

			# The length of the function's name, plus 2 for "gl",
			# plus 1 for the NUL.

			base_offset += len(self.functions[k].name) + 3

		print '    NAME_FUNC_OFFSET( -1, NULL, 0 )'
		print '};'
		return


def show_usage():
	print "Usage: %s [-f input_file_name] [-m mode]" % sys.argv[0]
	print "mode can be one of:"
	print "    long  - Create code for compilers that can handle very "
	print "            long string constants. (default)"
	print "    short - Create code for compilers that can only handle "
	print "            ANSI C89 string constants."
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:")
	except Exception,e:
		show_usage()

	long_string = 1
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			if val == "short":
				long_string = 0
			elif val == "long":
				long_string = 1
			else:
				show_usage()

	dh = PrintGlProcs( long_string )

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	dh.printHeader()
	parser.parse(f)
	dh.printFooter()
