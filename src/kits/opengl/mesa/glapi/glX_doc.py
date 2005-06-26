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
import glX_XML
import license
import sys, getopt


class glXDocItemFactory(glX_XML.glXItemFactory):
	"""Factory to create GLX protocol documentation oriented objects derived from glItem."""
    
	def create(self, context, name, attrs):
		if name == "param":
			return glXDocParameter(context, name, attrs)
		else:
			return glX_XML.glXItemFactory.create(self, context, name, attrs)

class glXDocParameter(gl_XML.glParameter):
	def __init__(self, context, name, attrs):
		self.order = 1;
		gl_XML.glParameter.__init__(self, context, name, attrs);


	def packet_type(self):
		"""Get the type string for the packet header
		
		GLX protocol documentation uses type names like CARD32,
		FLOAT64, LISTofCARD8, and ENUM.  This function converts the
		type of the parameter to one of these names."""

		list_of = ""
		if self.is_array():
			list_of = "LISTof"

		if self.p_type.glx_name == "":
			type_name = "CARD8"
		else:
			type_name = self.p_type.glx_name

		return "%s%s" % (list_of, type_name)

	def packet_size(self):
		p = None
		s = self.size()
		if s == 0:
			a_prod = "n"
			b_prod = self.p_type.size

			if self.p_count_parameters == None and self.counter != None:
				a_prod = self.counter
			elif (self.p_count_parameters != None and self.counter == None) or (self.is_output):
				pass
			elif self.p_count_parameters != None and self.counter != None:
				b_prod = self.counter
			else:
				raise RuntimeError("Parameter '%s' to function '%s' has size 0." % (self.name, self.context.name))

			ss = "%s*%s" % (a_prod, b_prod)

			return [ss, p]
		else:
			if s % 4 != 0:
				p = "p"

			return [str(s), p]

class PrintGlxProtoText(glX_XML.GlxProto):
	def __init__(self):
		glX_XML.GlxProto.__init__(self)
		self.factory = glXDocItemFactory()
		self.last_category = ""
		self.license = ""

	def printHeader(self):
		return

	def body_size(self, f):
		# At some point, refactor this function and
		# glXFunction::command_payload_length.

		size = 0;
		size_str = ""
		pad_str = ""
		plus = ""
		for p in f.parameterIterator(1, 2):
			[s, pad] = p.packet_size()
			try: 
				size += int(s)
			except Exception,e:
				size_str += "%s%s" % (plus, s)
				plus = "+"

			if pad != None:
				pad_str = pad

		return [size, size_str, pad_str]

	def print_render_header(self, f):
		[size, size_str, pad_str] = self.body_size(f)
		size += 4;

		if size_str == "":
			s = "%u" % ((size + 3) & ~3)
		elif pad_str != "":
			s = "%u+%s+%s" % (size, size_str, pad_str)
		else:
			s = "%u+%s" % (size, size_str)

		print '            2        %-15s rendering command length' % (s)
		print '            2        %-4u            rendering command opcode' % (f.glx_rop)
		return


	def print_single_header(self, f):
		[size, size_str, pad_str] = self.body_size(f)
		size = ((size + 3) / 4) + 2;

		if f.glx_vendorpriv != 0:
			size += 1

		print '            1        CARD8           opcode (X assigned)'
		print '            1        %-4u            GLX opcode (%s)' % (f.opcode_real_value(), f.opcode_real_name())

		if size_str == "":
			s = "%u" % (size)
		elif pad_str != "":
			s = "%u+((%s+%s)/4)" % (size, size_str, pad_str)
		else:
			s = "%u+((%s)/4)" % (size, size_str)

		print '            2        %-15s request length' % (s)

		if f.glx_vendorpriv != 0:
			print '            4        %-4u            vendor specific opcode' % (f.opcode_value())
			
		print '            4        GLX_CONTEXT_TAG context tag'

		return
		
	def print_reply(self, f):
		print '          =>'
		print '            1        1               reply'
		print '            1                        unused'
		print '            2        CARD16          sequence number'

		if f.output == None:
			print '            4        0               reply length'
		elif f.reply_always_array:
			print '            4        m               reply length'
		else:
			print '            4        m               reply length, m = (n == 1 ? 0 : n)'


		unused = 24
		if f.fn_return_type != 'void':
			print '            4        %-15s return value' % (f.fn_return_type)
			unused -= 4
		elif f.output != None:
			print '            4                        unused'
			unused -= 4

		if f.output != None:
			print '            4        CARD32          n'
			unused -= 4

		if f.output != None:
			if not f.reply_always_array:
				print ''
				print '            if (n = 1) this follows:'
				print ''
				print '            4        CARD32          %s' % (f.output.name)
				print '            %-2u                       unused' % (unused - 4)
				print ''
				print '            otherwise this follows:'
				print ''

			print '            %-2u                       unused' % (unused)
			p = f.output
			[s, pad] = p.packet_size()
			print '            %-8s %-15s %s' % (s, p.packet_type(), p.name)
			if pad != None:
				try:
					bytes = int(s)
					bytes = 4 - (bytes & 3)
					print '            %-8u %-15s unused' % (bytes, "")
				except Exception,e:
					print '            %-8s %-15s unused, %s=pad(%s)' % (pad, "", pad, s)
		else:
			print '            %-2u                       unused' % (unused)


	def print_body(self, f):
		for p in f.parameterIterator(1, 2):
			[s, pad] = p.packet_size()
			print '            %-8s %-15s %s' % (s, p.packet_type(), p.name)
			if pad != None:
				try:
					bytes = int(s)
					bytes = 4 - (bytes & 3)
					print '            %-8u %-15s unused' % (bytes, "")
				except Exception,e:
					print '            %-8s %-15s unused, %s=pad(%s)' % (pad, "", pad, s)

	def printFunction(self, f):
		# At some point this should be expanded to support pixel
		# functions, but I'm not going to lose any sleep over it now.

		if f.fn_offset < 0 or f.handcode or f.ignore or f.vectorequiv or f.image:
			return

		print '        %s' % (f.name)

		if f.glx_rop != 0:
			self.print_render_header(f)
		else:
			self.print_single_header(f)
		
		self.print_body(f)

		if f.needs_reply():
			self.print_reply(f)

		print ''
		return


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	dh = PrintGlxProtoText()

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	dh.printHeader()
	parser.parse(f)
	dh.printFooter()
