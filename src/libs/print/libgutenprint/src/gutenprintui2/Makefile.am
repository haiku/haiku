## $Id: Makefile.am,v 1.8 2008/10/24 00:37:14 rleigh Exp $
## Copyright (C) 2000 Roger Leigh
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2, or (at your option)
## any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

## Process this file with automake to produce Makefile.in.

@SET_MAKE@

include $(top_srcdir)/scripts/global.mk


## Variables

m4datadir = $(datadir)/aclocal
pkgconfigdatadir = $(libdir)/pkgconfig


## Programs

if BUILD_LIBGUTENPRINTUI2
lib_LTLIBRARIES = libgutenprintui2.la
endif

libgutenprintui2_la_SOURCES =			\
	printrcy.y				\
	printrcl.l				\
	printrc.h				\
	panel.c					\
	plist.c					\
	ui-utils.c				\
	print-image-thumbnail.c			\
	curve.c					\
	gammacurve.c				\
	typebuiltins.c				\
	gutenprintui-internal.h

libgutenprintui2_la_LIBADD = $(GUTENPRINTUI2_LIBDEPS)
# Uncommment to build an unversioned library (version in soname)
libgutenprintui2_la_CFLAGS = $(AM_CFLAGS) $(CFLAGS) $(GUTENPRINT_CFLAGS) $(GTK2_CFLAGS)
#libgutenprintui2_la_LDFLAGS = -release $(GUTENPRINT_VERSION) \
#	-rpath $(libdir) -no-undefined
# Uncomment to build a versioned library
libgutenprintui2_la_LDFLAGS = \
 -version-info  $(GUTENPRINTUI2_CURRENT_INTERFACE):$(GUTENPRINTUI2_INTERFACE_AGE):$(GUTENPRINTUI2_BINARY_AGE) \
 -rpath $(libdir) -no-undefined

AM_LFLAGS = -i
AM_YFLAGS = -d

printrcl.o: printrcy.o
plist.o: printrcy.o

## Data

if BUILD_LIBGUTENPRINTUI2
pkgconfigdata_DATA = gutenprintui2.pc
endif


## Rules

# Copied from include/gutenprintui2/Makefile.am
# This duplication needs fixing somehow
stpui_public_h_sources = \
	../../include/gutenprintui2/gutenprintui.h	\
	../../include/gutenprintui2/curve.h		\
	../../include/gutenprintui2/gammacurve.h

if MAINTAINER_MODE
typebuiltins_extra_deps = Makefile
endif

# Really need to depend upon $(stpui_public_h_sources) here...
$(srcdir)/typebuiltins.c: $(typebuiltins_extra_deps)
	( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include <gutenprintui2/gutenprintui.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(stpui_public_h_sources) ) > $@.tmp
	mv $@.tmp $@


## Clean

MAINTAINERCLEANFILES = Makefile.in typebuiltins.c typebuiltins.c.tmp printrcl.c printrcy.c printrcy.h
