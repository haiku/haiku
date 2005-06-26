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

import re

class glItem:
	"""Generic class on which all other API entity types are based."""

	def __init__(self, tag_name, name, context):
		self.name = name
		self.category = context.get_category_define()
		self.context = context
		self.tag_name = tag_name
		
		context.append(tag_name, self)
		return
	
	def startElement(self, name, attrs):
		"""Generic startElement handler.
		
		The startElement handler is called for all elements except
		the one that starts the object.  For a foo element, the
		XML "<foo><bar/></foo>" would cause the startElement handler
		to be called once, but the endElement handler would be called
		twice."""
		return

	def endElement(self, name):
		"""Generic endElement handler.

		Generic endElement handler.    Returns 1 if the tag containing
		the object is complete.  Otherwise 0 is returned.  All
		derived class endElement handlers should call this method.  If
		the name of the ending tag is the same as the tag that
		started this object, the object is assumed to be complete.
		
		This fails if a tag can contain another tag with the same
		name.  The XML "<foo><foo/><bar/></foo>" would fail.  The
		object would end before the bar tag was processed.
		
		The endElement handler is called for every end element
		associated with an object, even the element that started the
		object.  See the description of startElement an example."""

		if name == self.tag_name:
			return 1
		else:
			return 0

	def get_category_define(self):
		return self.category


class glEnum( glItem ):
	"""Subclass of glItem for representing GL enumerants.
	
	This class is not complete, and is not really used yet."""

	def __init__(self, context, name, attrs):
		self.value = int(attrs.get('value', "0x0000"), 0)
		self.functions = {}

		enum_name = "GL_" + attrs.get('name', None)
		glItem.__init__(self, name, enum_name, context)

	def startElement(self, name, attrs):
		if name == "size":
			name = attrs.get('name', None)
			count = int(attrs.get('count', "0"), 0)
			self.functions[name] = count

		return


class glType( glItem ):
	"""Subclass of glItem for representing GL types."""

	def __init__(self, context, name, attrs):
		self.size = int(attrs.get('size', "0"))
		self.glx_name = attrs.get('glx_name', "")

		type_name = "GL" + attrs.get('name', None)
		glItem.__init__(self, name, type_name, context)


class glParameter( glItem ):
	"""Parameter of a glFunction."""
	p_type = None
	p_type_string = ""
	p_count = 0
	p_count_parameters = None
	counter = None
	is_output = 0
	is_counter = 0
	is_pointer = 0

	def __init__(self, context, name, attrs):
		p_name = attrs.get('name', None)
		self.p_type_string = attrs.get('type', None)
		self.p_count_parameters = attrs.get('variable_param', None)

		self.p_type = context.context.find_type(self.p_type_string)
		if self.p_type == None:
			raise RuntimeError("Unknown type '%s' in function '%s'." % (self.p_type_string, context.name))


		# The count tag can be either a numeric string or the name of
		# a variable.  If it is the name of a variable, the int(c)
		# statement will throw an exception, and the except block will
		# take over.

		c = attrs.get('count', "0")
		try: 
			self.p_count = int(c)
			self.counter = None
		except Exception,e:
			self.p_count = 0
			self.counter = c

		if attrs.get('counter', "false") == "true":
			self.is_counter = 1
		else:
			self.is_counter = 0

		if attrs.get('output', "false") == "true":
			self.is_output = 1
		else:
			self.is_output = 0

			
		# Pixel data has special parameters.

		self.width      = attrs.get('img_width',  None)
		self.height     = attrs.get('img_height', None)
		self.depth      = attrs.get('img_depth',  None)
		self.extent     = attrs.get('img_extent', None)

		self.img_xoff   = attrs.get('img_xoff',   None)
		self.img_yoff   = attrs.get('img_yoff',   None)
		self.img_zoff   = attrs.get('img_zoff',   None)
		self.img_woff   = attrs.get('img_woff',   None)

		self.img_format = attrs.get('img_format', None)
		self.img_type   = attrs.get('img_type',   None)
		self.img_target = attrs.get('img_target', None)

		pad = attrs.get('img_pad_dimensions', "false")
		if pad == "true":
			self.img_pad_dimensions = 1
		else:
			self.img_pad_dimensions = 0


		null_flag = attrs.get('img_null_flag', "false")
		if null_flag == "true":
			self.img_null_flag = 1
		else:
			self.img_null_flag = 0

		send_null = attrs.get('img_send_null', "false")
		if send_null == "true":
			self.img_send_null = 1
		else:
			self.img_send_null = 0



		if self.p_count > 0 or self.counter or self.p_count_parameters:
			has_count = 1
		else:
			has_count = 0


		# If there is a * anywhere in the parameter's type, then it
		# is a pointer.

		if re.compile("[*]").search(self.p_type_string):
			# We could do some other validation here.  For
			# example, an output parameter should not be const,
			# but every non-output parameter should.

			self.is_pointer = 1;
		else:
			# If a parameter is not a pointer, then there cannot
			# be an associated count (either fixed size or
			# variable) and the parameter cannot be an output.

			if has_count or self.is_output:
				raise RuntimeError("Non-pointer type has count or is output.")
			self.is_pointer = 0;

		glItem.__init__(self, name, p_name, context)
		return


	def is_variable_length_array(self):
		"""Determine if a parameter is a variable length array.
		
		A parameter is considered to be a variable length array if
		its size depends on the value of another parameter that is
		an enumerant.  The params parameter to glTexEnviv is an
		example of a variable length array parameter.  Arrays whose
		size depends on a count variable, such as the lists parameter
		to glCallLists, are not variable length arrays in this
		sense."""

		return self.p_count_parameters or self.counter or self.width


	def is_array(self):
		return self.is_pointer


	def count_string(self):
		"""Return a string representing the number of items
		
		Returns a string representing the number of items in a
		parameter.  For scalar types this will always be "1".  For
		vector types, it will depend on whether or not it is a
		fixed length vector (like the parameter of glVertex3fv),
		a counted length (like the vector parameter of
		glDeleteTextures), or a general variable length vector."""

		if self.is_array():
			if self.p_count_parameters != None:
				return "compsize"
			elif self.counter != None:
				return self.counter
			else:
				return str(self.p_count)
		else:
			return "1"


	def size(self):
		if self.p_count_parameters or self.counter or self.width or self.is_output:
			return 0
		elif self.p_count == 0:
			return self.p_type.size
		else:
			return self.p_type.size * self.p_count

	def size_string(self):
		s = self.size()
		if s == 0:
			a_prod = "compsize"
			b_prod = self.p_type.size

			if self.p_count_parameters == None and self.counter != None:
				a_prod = self.counter
			elif self.p_count_parameters != None and self.counter == None:
				pass
			elif self.p_count_parameters != None and self.counter != None:
				b_prod = self.counter
			elif self.width:
				return "compsize"
			else:
				raise RuntimeError("Parameter '%s' to function '%s' has size 0." % (self.name, self.context.name))

			return "(%s * %s)" % (a_prod, b_prod)
		else:
			return str(s)


class glParameterIterator:
	"""Class to iterate over a list of glParameters.
	
	Objects of this class are returned by the parameterIterator method of
	the glFunction class.  They are used to iterate over the list of
	parameters to the function."""

	def __init__(self, data):
		self.data = data
		self.index = 0

	def __iter__(self):
		return self

	def next(self):
		if self.index == len( self.data ):
			raise StopIteration
		i = self.index
		self.index += 1
		return self.data[i]


class glFunction( glItem ):
	real_name = ""
	fn_alias = None
	fn_offset = -1
	fn_return_type = "void"
	fn_parameters = []

	def __init__(self, context, name, attrs):
		self.fn_alias = attrs.get('alias', None)
		self.fn_parameters = []
		self.image = None

		temp = attrs.get('offset', None)
		if temp == None or temp == "?":
			self.fn_offset = -1
		else:
			self.fn_offset = int(temp)

		fn_name = attrs.get('name', None)
		if self.fn_alias != None:
			self.real_name = self.fn_alias
		else:
			self.real_name = fn_name

		glItem.__init__(self, name, fn_name, context)
		return


	def parameterIterator(self):
		return glParameterIterator(self.fn_parameters)


	def startElement(self, name, attrs):
		if name == "param":
			try:
				self.context.factory.create(self, name, attrs)
			except RuntimeError:
				print "Error with parameter '%s' in function '%s'." \
					% (attrs.get('name','(unknown)'), self.name)
				raise
		elif name == "return":
			self.set_return_type(attrs.get('type', None))


	def append(self, tag_name, p):
		if tag_name != "param":
			raise RuntimeError("Trying to append '%s' to parameter list of function '%s'." % (tag_name, self.name))

		if p.width:
			self.image = p

		self.fn_parameters.append(p)


	def set_return_type(self, t):
		self.fn_return_type = t


	def get_parameter_string(self):
		arg_string = ""
		comma = ""
		for p in glFunction.parameterIterator(self):
			arg_string = arg_string + comma + p.p_type_string + " " + p.name
			comma = ", "

		if arg_string == "":
			arg_string = "void"

		return arg_string


class glItemFactory:
	"""Factory to create objects derived from glItem."""
    
	def create(self, context, name, attrs):
		if name == "function":
			return glFunction(context, name, attrs)
		elif name == "type":
			return glType(context, name, attrs)
		elif name == "enum":
			return glEnum(context, name, attrs)
		elif name == "param":
			return glParameter(context, name, attrs)
		else:
			return None


class FilterGLAPISpecBase(saxutils.XMLFilterBase):
	name = "a"
	license = "The license for this file is unspecified."
	functions = {}
	next_alias = -2
	types = {}
	xref = {}
	current_object = None
	factory = None
	current_category = ""

	def __init__(self):
		saxutils.XMLFilterBase.__init__(self)
		self.functions = {}
		self.types = {}
		self.xref = {}
		self.factory = glItemFactory()


	def find_type(self,type_name):
		for t in self.types:
			if re.compile(t).search(type_name):
				return self.types[t]
		print "Unable to find base type matching \"%s\"." % (type_name)
		return None


	def find_function(self,function_name):
		index = self.xref[function_name]
		return self.functions[index]


	def printFunctions(self):
		keys = self.functions.keys()
		keys.sort()
		prevk = -1
		for k in keys:
			if k < 0: continue

			if self.functions[k].fn_alias == None:
				if k != prevk + 1:
					#print 'Missing offset %d' % (prevk)
					pass
			prevk = int(k)
			self.printFunction(self.functions[k])

		keys.reverse()
		for k in keys:
			if self.functions[k].fn_alias != None:
				self.printFunction(self.functions[k])

		return


	def printHeader(self):
		"""Print the header associated with all files and call the printRealHeader method."""

		print '/* DO NOT EDIT - This file generated automatically by %s script */' \
			% (self.name)
		print ''
		print '/*'
		print ' * ' + self.license.replace('\n', '\n * ')
		print ' */'
		print ''
		self.printRealHeader();
		return


	def printFooter(self):
		"""Print the header associated with all files and call the printRealFooter method."""

		self.printFunctions()
		self.printRealFooter()


	def get_category_define(self):
		"""Convert the category name to the #define that would be found in glext.h"""

		if re.compile("[1-9][0-9]*[.][0-9]+").match(self.current_category):
			s = self.current_category
			return "GL_VERSION_" + s.replace(".", "_")
		else:
			return self.current_category


	def append(self, object_type, obj):
		if object_type == "function":
			# If the function is not an alias and has a negative
			# offset, then we do not need to track it.  These are
			# functions that don't have an assigned offset

			if obj.fn_offset >= 0 or obj.fn_alias != None:
				if obj.fn_offset >= 0:
					index = obj.fn_offset
				else:
					index = self.next_alias
					self.next_alias -= 1

				self.functions[index] = obj
				self.xref[obj.name] = index
		elif object_type == "type":
			self.types[obj.name] = obj

		return


	def startElement(self, name, attrs):
		"""Start a new element in the XML stream.
		
		Starts a new element.  There are three types of elements that
		are specially handled by this function.  When a "category"
		element is encountered, the name of the category is saved.
		If an element is encountered and no API object is
		in-progress, a new object is created using the API factory.
		Any future elements, until that API object is closed, are
		passed to the current objects startElement method.
	
		This paradigm was chosen becuase it allows subclasses of the
		basic API types (i.e., glFunction, glEnum, etc.) to handle
		additional XML data, GLX protocol information,  that the base
		classes do not know about."""

		if self.current_object != None:
			self.current_object.startElement(name, attrs)
		elif name == "category":
			self.current_category = attrs.get('name', "")
		else:
			self.current_object = self.factory.create(self, name, attrs)
		return


	def endElement(self, name):
		if self.current_object != None:
			if self.current_object.endElement(name):
				self.current_object = None
		return


	def printFunction(self,offset):
		"""Print a single function.

		In the base class, this function is empty.  All derived
		classes should over-ride this function."""
		return
    

	def printRealHeader(self):
		"""Print the "real" header for the created file.

		In the base class, this function is empty.  All derived
		classes should over-ride this function."""
		return


	def printRealFooter(self):
		"""Print the "real" footer for the created file.

		In the base class, this function is empty.  All derived
		classes should over-ride this function."""
		return
