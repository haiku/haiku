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


def printPure():
	print """#  if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#    define PURE __attribute__((pure))
#  else
#    define PURE
#  endif"""

def printFastcall():
	print """#  if defined(__i386__) && defined(__GNUC__)
#    define FASTCALL __attribute__((fastcall))
#  else
#    define FASTCALL
#  endif"""

def printVisibility(S, s):
	print """#  if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#    define %s  __attribute__((visibility("%s")))
#  else
#    define %s
#  endif""" % (S, s, S)

def printNoinline():
	print """#  if defined(__GNUC__)
#    define NOINLINE __attribute__((noinline))
#  else
#    define NOINLINE
#  endif"""


class glXItemFactory(gl_XML.glItemFactory):
	"""Factory to create GLX protocol oriented objects derived from glItem."""
    
	def create(self, context, name, attrs):
		if name == "function":
			return glXFunction(context, name, attrs)
		elif name == "enum":
			return glXEnum(context, name, attrs)
		elif name == "param":
			return glXParameter(context, name, attrs)
		else:
			return gl_XML.glItemFactory.create(self, context, name, attrs)

class glXEnumFunction:
	def __init__(self, name):
		self.name = name
		
		# "enums" is a set of lists.  The element in the set is the
		# value of the enum.  The list is the list of names for that
		# value.  For example, [0x8126] = {"POINT_SIZE_MIN",
		# "POINT_SIZE_MIN_ARB", "POINT_SIZE_MIN_EXT",
		# "POINT_SIZE_MIN_SGIS"}.

		self.enums = {}

		# "count" is indexed by count values.  Each element of count
		# is a list of index to "enums" that have that number of
		# associated data elements.  For example, [4] = 
		# {GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_EMISSION,
		# GL_AMBIENT_AND_DIFFUSE} (the enum names are used here,
		# but the actual hexadecimal values would be in the array).

		self.count = {}


	def append(self, count, value, name):
		if self.enums.has_key( value ):
			self.enums[value].append(name)
		else:
			if not self.count.has_key(count):
				self.count[count] = []

			self.enums[value] = []
			self.enums[value].append(name)
			self.count[count].append(value)


	def signature( self ):
		sig = ""
		for i in self.count:
			for e in self.count[i]:
				sig += "%04x,%u," % (e, i)
	
		return sig;


	def PrintUsingTable(self):
		"""Emit the body of the __gl*_size function using a pair
		of look-up tables and a mask.  The mask is calculated such
		that (e & mask) is unique for all the valid values of e for
		this function.  The result of (e & mask) is used as an index
		into the first look-up table.  If it matches e, then the
		same entry of the second table is returned.  Otherwise zero
		is returned.
		
		It seems like this should cause better code to be generated.
		However, on x86 at least, the resulting .o file is about 20%
		larger then the switch-statment version.  I am leaving this
		code in because the results may be different on other
		platforms (e.g., PowerPC or x86-64)."""

		return 0
		count = 0
		for a in self.enums:
			count += 1

		# Determine if there is some mask M, such that M = (2^N) - 1,
		# that will generate unique values for all of the enums.

		mask = 0
		for i in [1, 2, 3, 4, 5, 6, 7, 8]:
			mask = (1 << i) - 1

			fail = 0;
			for a in self.enums:
				for b in self.enums:
					if a != b:
						if (a & mask) == (b & mask):
							fail = 1;

			if not fail:
				break;
			else:
				mask = 0

		if (mask != 0) and (mask < (2 * count)):
			masked_enums = {}
			masked_count = {}

			for i in range(0, mask + 1):
				masked_enums[i] = "0";
				masked_count[i] = 0;

			for c in self.count:
				for e in self.count[c]:
					i = e & mask
					masked_enums[i] = '0x%04x /* %s */' % (e, self.enums[e][0])
					masked_count[i] = c


			print '    static const GLushort a[%u] = {' % (mask + 1)
			for e in masked_enums:
				print '        %s, ' % (masked_enums[e])
			print '    };'

			print '    static const GLubyte b[%u] = {' % (mask + 1)
			for c in masked_count:
				print '        %u, ' % (masked_count[c])
			print '    };'

			print '    const unsigned idx = (e & 0x%02xU);' % (mask)
			print ''
			print '    return (e == a[idx]) ? (GLint) b[idx] : 0;'
			return 1;
		else:
			return 0;

	def PrintUsingSwitch(self):
		"""Emit the body of the __gl*_size function using a 
		switch-statement."""

		print '    switch( e ) {'

		for c in self.count:
			for e in self.count[c]:
				first = 1

				# There may be multiple enums with the same
				# value.  This happens has extensions are
				# promoted from vendor-specific or EXT to
				# ARB and to the core.  Emit the first one as
				# a case label, and emit the others as
				# commented-out case labels.

				for j in self.enums[e]:
					if first:
						print '        case %s:' % (j)
						first = 0
					else:
						print '/*      case %s:*/' % (j)
					
			print '            return %u;' % (c)
					
		print '        default: return 0;'
		print '    }'


	def Print(self, name):
		print 'INTERNAL PURE FASTCALL GLint'
		print '__gl%s_size( GLenum e )' % (name)
		print '{'

		if not self.PrintUsingTable():
			self.PrintUsingSwitch()

		print '}'
		print ''



class glXEnum(gl_XML.glEnum):
	def __init__(self, context, name, attrs):
		gl_XML.glEnum.__init__(self, context, name, attrs)
		self.glx_functions = []

	def startElement(self, name, attrs):
		if name == "size":
			n = attrs.get('name', None)
			if not self.context.glx_enum_functions.has_key( n ):
				f = glXEnumFunction( n )
				self.context.glx_enum_functions[ f.name ] = f

			temp = attrs.get('count', None)
			try:
				c = int(temp)
			except Exception,e:
				raise RuntimeError('Invalid count value "%s" for enum "%s" in function "%s" when an integer was expected.' % (temp, self.name, n))

			self.context.glx_enum_functions[ n ].append( c, self.value, self.name )
		else:
			gl_XML.glEnum.startElement(self, context, name, attrs)
		return


class glXParameter(gl_XML.glParameter):
	def __init__(self, context, name, attrs):
		self.order = 1;
		gl_XML.glParameter.__init__(self, context, name, attrs);


class glXFunction(gl_XML.glFunction):
	glx_rop = 0
	glx_sop = 0
	glx_vendorpriv = 0

	# If this is set to true, it means that GLdouble parameters should be
	# written to the GLX protocol packet in the order they appear in the
	# prototype.  This is different from the "classic" ordering.  In the
	# classic ordering GLdoubles are written to the protocol packet first,
	# followed by non-doubles.  NV_vertex_program was the first extension
	# to break with this tradition.

	glx_doubles_in_order = 0

	vectorequiv = None
	handcode = 0
	ignore = 0
	can_be_large = 0

	def __init__(self, context, name, attrs):
		self.vectorequiv = attrs.get('vectorequiv', None)
		self.count_parameters = None
		self.counter = None
		self.output = None
		self.can_be_large = 0
		self.reply_always_array = 0

		gl_XML.glFunction.__init__(self, context, name, attrs)
		return

	def startElement(self, name, attrs):
		"""Process elements within a function that are specific to GLX."""

		if name == "glx":
			self.glx_rop = int(attrs.get('rop', "0"))
			self.glx_sop = int(attrs.get('sop', "0"))
			self.glx_vendorpriv = int(attrs.get('vendorpriv', "0"))

			if attrs.get('handcode', "false") == "true":
				self.handcode = 1
			else:
				self.handcode = 0

			if attrs.get('ignore', "false") == "true":
				self.ignore = 1
			else:
				self.ignore = 0

			if attrs.get('large', "false") == "true":
				self.can_be_large = 1
			else:
				self.can_be_large = 0

			if attrs.get('doubles_in_order', "false") == "true":
				self.glx_doubles_in_order = 1
			else:
				self.glx_doubles_in_order = 0

			if attrs.get('always_array', "false") == "true":
				self.reply_always_array = 1
			else:
				self.reply_always_array = 0

		else:
			gl_XML.glFunction.startElement(self, name, attrs)


	def append(self, tag_name, p):
		gl_XML.glFunction.append(self, tag_name, p)

		if p.is_variable_length_array():
			p.order = 2;
		elif not self.glx_doubles_in_order and p.p_type.size == 8:
			p.order = 0;

		if p.p_count_parameters != None:
			self.count_parameters = p.p_count_parameters
		
		if p.is_counter:
			self.counter = p.name
			
		if p.is_output:
			self.output = p

		return

	def variable_length_parameter(self):
		for param in self.fn_parameters:
			if param.is_variable_length_array():
				return param
			
		return None


	def command_payload_length(self):
		size = 0
		size_string = ""
		for p in self:
			if p.is_output: continue
			temp = p.size_string()
			try:
				s = int(temp)
				size += s
			except Exception,e:
				size_string = size_string + " + __GLX_PAD(%s)" % (temp)

		return [size, size_string]

	def command_length(self):
		[size, size_string] = self.command_payload_length()

		if self.glx_rop != 0:
			size += 4

		size = ((size + 3) & ~3)
		return "%u%s" % (size, size_string)


	def opcode_real_value(self):
		if self.glx_vendorpriv != 0:
			if self.needs_reply():
				return 17
			else:
				return 16
		else:
			return self.opcode_value()

	def opcode_value(self):
		if self.glx_rop != 0:
			return self.glx_rop
		elif self.glx_sop != 0:
			return self.glx_sop
		elif self.glx_vendorpriv != 0:
			return self.glx_vendorpriv
		else:
			return -1
	
	def opcode_rop_basename(self):
		if self.vectorequiv == None:
			return self.name
		else:
			return self.vectorequiv

	def opcode_name(self):
		if self.glx_rop != 0:
			return "X_GLrop_%s" % (self.opcode_rop_basename())
		elif self.glx_sop != 0:
			return "X_GLsop_%s" % (self.name)
		elif self.glx_vendorpriv != 0:
			return "X_GLvop_%s" % (self.name)
		else:
			return "ERROR"

	def opcode_real_name(self):
		if self.glx_vendorpriv != 0:
			if self.needs_reply():
				return "X_GLXVendorPrivateWithReply"
			else:
				return "X_GLXVendorPrivate"
		else:
			return self.opcode_name()


	def return_string(self):
		if self.fn_return_type != 'void':
			return "return retval;"
		else:
			return "return;"


	def needs_reply(self):
		return self.fn_return_type != 'void' or self.output != None


class GlxProto(gl_XML.FilterGLAPISpecBase):
	name = "glX_proto_send.py (from Mesa)"

	def __init__(self):
		gl_XML.FilterGLAPISpecBase.__init__(self)
		self.factory = glXItemFactory()
		self.glx_enum_functions = {}


	def endElement(self, name):
		if name == 'O
		glx_program was the fir.odeWdeWdeWde TO
		g nmentr	glx_progrWpeturn None


	d%O
		glxeWdeWdGLrop_%s" % ( prray = 0

_sop ! e_n):
		for pize getmEo1ry()
		self.glx_enum_funct,Lmand_pay = 0pnt(self, n,!= 0:
		sew_pay = 0pnt(self, n,!= 0:
		sew_pabX,!= 0:
		sew_pabX,!= 0:
		sew_pabX)' lf)
)e != 'void'Pin "re%glx		p.order = 0;

		if p.p_count_parame	self.output = pder =pder =.p_count_parame	self.output = 0;cNA = 0

		return [size, si2169
#!/usr/bin/			retu		g (C) Copyright IBM Corporize_st0:
		g All Rights Reserved		s	g Permispe !=selcode


grmane4

free off.ga_fu,