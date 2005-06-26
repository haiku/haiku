#!/usr/bin/python2

# (C) Copyright IBM Corporation 2004, 2005
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

class PrintGenericStubs(gl_XML.FilterGLAPISpecBase):
	name = "gl_x86_asm.py (from Mesa)"

	def __init__(self):
		gl_XML.FilterGLAPISpecBase.__init__(self)
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004, 2005""", "BRIAN PAUL, IBM")


	def get_stack_size(self, f):
		size = 0
		for p in f.parameterIterator():
			t = p.p_type

			if p.is_array() or t.size != 8:
				size += 4
			else:
				size += 8

		return size

	def printRealHeader(self):
		print '#include "assyntax.h"'
		print '#include "glapioffsets.h"'
		print ''
		print "/* If we build with gcc's -fvisibility=hidden flag, we'll need to change"
		print "* the symbol visibility mode to 'default'."
		print '*/'
		print '#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 303'
		print '#pragma GCC visibility push(default)'
		print '#endif'
		print ''
		print '#ifndef __WIN32__'
		print ''	
		print '#if defined(STDCALL_API)'
		print '# if defined(USE_MGL_NAMESPACE)'
		print '#  define GL_PREFIX(n,n2) GLNAME(CONCAT(mgl,n2))'
		print '# else'
		print '#  define GL_PREFIX(n,n2) GLNAME(CONCAT(gl,n2))'
		print '# endif'
		print '#else'
		print '# if defined(USE_MGL_NAMESPACE)'
		print '#  define GL_PREFIX(n,n2) GLNAME(CONCAT(mgl,n))'
		print '# else'
		print '#  define GL_PREFIX(n,n2) GLNAME(CONCAT(gl,n))'
		print '# endif'
		print '#endif'
		print ''
		print '#define GL_OFFSET(x) CODEPTR(REGOFF(4 * x, EAX))'
		print ''
		print '#if defined(GNU_ASSEMBLER) && !defined(__DJGPP__) && !defined(__MINGW32__)'
		print '#define GLOBL_FN(x) GLOBL x ; .type x, function'
		print '#else'
		print '#define GLOBL_FN(x) GLOBL x'
		print '#endif'
		print ''
		print '#if defined(PTHREADS) || defined(XTHREADS) || defined(SOLARIS_THREADS) || defined(WIN32_THREADS) || defined(BEOS_THREADS)'
		print '#  define THREADS'
		print '#endif'
		print ''
		print '#if defined(PTHREADS)'
		print '#  define GL_STUB(fn,off,fn_alt)\t\t\t\\'
		print 'ALIGNTEXT16;\t\t\t\t\t\t\\'
		print 'GLOBL_FN(GL_PREFIX(fn, fn_alt));\t\t\t\\'
		print 'GL_PREFIX(fn, fn_alt):\t\t\t\t\t\\'
		print '\tMOV_L(CONTENT(GLNAME(_glapi_DispatchTSD)), EAX) ;\t\\'
		print '\tTEST_L(EAX, EAX) ;\t\t\t\t\\'
		print '\tJE(1f) ;\t\t\t\t\t\\'
		print '\tJMP(GL_OFFSET(off)) ;\t\t\t\t\\'
		print '1:\tCALL(_x86_get_dispatch) ;\t\t\t\\'
		print '\tJMP(GL_OFFSET(off))'
		print '#elif defined(THREADS)'
		print '#  define GL_STUB(fn,off,fn_alt)\t\t\t\\'
		print 'ALIGNTEXT16;\t\t\t\t\t\t\\'
		print 'GLOBL_FN(GL_PREFIX(fn, fn_alt));\t\t\t\\'
		print 'GL_PREFIX(fn, fn_alt):\t\t\t\t\t\\'
		print '\tMOV_L(CONTENT(GLNAME(_glapi_DispatchTSD)), EAX) ;\t\\'
		print '\tTEST_L(EAX, EAX) ;\t\t\t\t\\'
		print '\tJE(1f) ;\t\t\t\t\t\\'
		print '\tJMP(GL_OFFSET(off)) ;\t\t\t\t\\'
		print '1:\tCALL(_glapi_get_dispatch) ;\t\t\t\\'
		print '\tJMP(GL_OFFSET(off))'
		print '#else /* Non-threaded version. */'
		print '#  define GL_STUB(fn,off,fn_alt)\t\t\t\\'
		print 'ALIGNTEXT16;\t\t\t\t\t\t\\'
		print 'GLOBL_FN(GL_PREFIX(fn, fn_alt));\t\t\t\\'
		print 'GL_PREFIX(fn, fn_alt):\t\t\t\t\t\\'
		print '\tMOV_L(CONTENT(GLNAME(_glapi_Dispatch)), EAX) ;\t\\'
		print '\tJMP(GL_OFFSET(off))'
		print '#endif'
		print ''
		print '#ifdef HAVE_ALIAS'
		print '#  define GL_STUB_ALIAS(fn,off,fn_alt,alias,alias_alt)\t\\'
		print '\t.globl\tGL_PREFIX(fn, fn_alt) ;\t\t\t\\'
		print '\t.set\tGL_PREFIX(fn, fn_alt), GL_PREFIX(alias, alias_alt)'
		print '#else'
		print '#  define GL_STUB_ALIAS(fn,off,fn_alt,alias,alias_alt)\t\\'
		print '    GL_STUB(fn, off, fn_alt)'
		print '#endif'
		print ''
		print 'SEG_TEXT'
		print ''
		print '#ifdef PTHREADS'
		print 'EXTERN GLNAME(_glapi_Dispatch)'
		print 'EXTERN GLNAME(_gl_DispatchTSD)'
		print 'EXTERN GLNAME(pthread_getspecific)'
		print ''
		print 'ALIGNTEXT16'
		print 'GLNAME(_x86_get_dispatch):'
		print '\tSUB_L(CONST(24), ESP)'
		print '\tPUSH_L(GLNAME(_gl_DispatchTSD))'
		print '\tCALL(GLNAME(pthread_getspecific))'
		print '\tADD_L(CONST(28), ESP)'
		print '\tRET'
		print '#elif defined(THREADS)'
		print 'EXTERN GLNAME(_glapi_get_dispatch)'
		print '#endif'
		print ''
		print '\t\tALIGNTEXT16 ; GLOBL GLNAME(gl_dispatch_functions_start)'
		print 'GLNAME(gl_dispatch_functions_start):'
		print ''
		return

	def printRealFooter(self):
		print ''
		print '#endif  /* __WIN32__ */'
		return

	def printFunction(self, f):
		stack = self.get_stack_size(f)

		alt = "%s@%u" % (f.name, stack)
		if f.fn_alias == None:
		    print '\tGL_STUB(%s, _gloffset_%s, %s)' % (f.name, f.real_name, alt)
		else:
		    alias_alt = "%s@%u" % (f.real_name, stack)
		    print '\tGL_STUB_ALIAS(%s, _gloffset_%s, %s, %s, %s)' % \
		        (f.name, f.real_name, alt, f.real_name, alias_alt)
		return

def show_usage():
	print "Usage: %s [-f input_file_name] [-m output_mode]" % sys.argv[0]
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
	mode = "generic"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "m:f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == '-m':
			mode = val
		elif arg == "-f":
			file_name = val

	if mode == "generic":
		dh = PrintGenericStubs()
	else:
		print "ERROR: Invalid mode \"%s\" specified." % mode
		show_usage()

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	dh.printHeader()
	parser.parse(f)
	dh.printFooter()
