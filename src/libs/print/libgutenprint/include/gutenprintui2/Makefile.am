## $Id: Makefile.am,v 1.4 2008/10/24 00:37:14 rleigh Exp $
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


pkgincludedir = $(includedir)/gutenprintui2


## Data

stpui_public_h_sources =	\
	gutenprintui.h		\
	curve.h			\
	gammacurve.h

stpui_built_public_sources = \
	typebuiltins.h

if BUILD_LIBGUTENPRINTUI2
pkginclude_HEADERS = \
	$(stpui_public_h_sources) \
	$(stpui_built_public_sources)
endif

## Rules

if MAINTAINER_MODE
typebuiltins_extra_deps = Makefile
endif

$(srcdir)/typebuiltins.h: $(stpui_public_h_sources) $(typebuiltins_extra_deps)
	( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef GUTENPRINTUI_TYPE_BUILTINS_H\n#define GUTENPRINTUI_TYPE_BUILTINS_H\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define STPUI_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* GUTENPRINTUI_TYPE_BUILTINS_H */" \
		$(stpui_public_h_sources) ) > $@.tmp
	mv $@.tmp $@


## Clean

MAINTAINERCLEANFILES = Makefile.in typebuiltins.h typebuiltins.h.tmp
