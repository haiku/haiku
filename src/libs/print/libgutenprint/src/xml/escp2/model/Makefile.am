## $Id: Makefile.am,v 1.27 2011/03/05 18:49:13 rlk Exp $
## Copyright (C) 2008 Robert Krawitz
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

pkgxmldatadir = $(pkgdatadir)/@GUTENPRINT_MAJOR_VERSION@.@GUTENPRINT_MINOR_VERSION@/xml/escp2/model

pkgxmldata_DATA =				\
	model_0.xml				\
	model_1.xml				\
	model_2.xml				\
	model_3.xml				\
	model_4.xml				\
	model_5.xml				\
	model_6.xml				\
	model_7.xml				\
	model_8.xml				\
	model_9.xml				\
	model_10.xml				\
	model_11.xml				\
	model_12.xml				\
	model_13.xml				\
	model_14.xml				\
	model_15.xml				\
	model_16.xml				\
	model_17.xml				\
	model_18.xml				\
	model_19.xml				\
	model_20.xml				\
	model_21.xml				\
	model_22.xml				\
	model_23.xml				\
	model_24.xml				\
	model_25.xml				\
	model_26.xml				\
	model_27.xml				\
	model_28.xml				\
	model_29.xml				\
	model_30.xml				\
	model_31.xml				\
	model_32.xml				\
	model_33.xml				\
	model_34.xml				\
	model_35.xml				\
	model_36.xml				\
	model_37.xml				\
	model_38.xml				\
	model_39.xml				\
	model_40.xml				\
	model_41.xml				\
	model_42.xml				\
	model_43.xml				\
	model_44.xml				\
	model_45.xml				\
	model_46.xml				\
	model_47.xml				\
	model_48.xml				\
	model_49.xml				\
	model_50.xml				\
	model_51.xml				\
	model_52.xml				\
	model_53.xml				\
	model_54.xml				\
	model_55.xml				\
	model_56.xml				\
	model_57.xml				\
	model_58.xml				\
	model_59.xml				\
	model_60.xml				\
	model_61.xml				\
	model_62.xml				\
	model_63.xml				\
	model_64.xml				\
	model_65.xml				\
	model_66.xml				\
	model_67.xml				\
	model_68.xml				\
	model_69.xml				\
	model_70.xml				\
	model_71.xml				\
	model_72.xml				\
	model_73.xml				\
	model_74.xml				\
	model_75.xml				\
	model_76.xml				\
	model_77.xml				\
	model_78.xml				\
	model_79.xml				\
	model_80.xml				\
	model_81.xml				\
	model_82.xml				\
	model_83.xml				\
	model_84.xml				\
	model_85.xml				\
	model_86.xml				\
	model_87.xml				\
	model_88.xml				\
	model_89.xml				\
	model_90.xml				\
	model_91.xml				\
	model_92.xml				\
	model_93.xml				\
	model_94.xml				\
	model_95.xml				\
	model_96.xml				\
	model_97.xml				\
	model_98.xml				\
	model_99.xml				\
	model_100.xml				\
	model_101.xml				\
	model_102.xml				\
	model_103.xml				\
	model_104.xml				\
	model_105.xml				\
	model_106.xml				\
	model_107.xml				\
	model_108.xml				\
	model_109.xml				\
	model_110.xml				\
	model_111.xml				\
	model_112.xml				\
	model_113.xml				\
	model_114.xml				\
	model_115.xml

xml-stamp: $(pkgxmldata_DATA) Makefile.am
	-rm -f $@ $@.tmp
	touch $@.tmp
	for f in $(pkgxmldata_DATA) ; do echo $$f >> $@.tmp; done
	mv $@.tmp $@

all-local: xml-stamp

dist-hook: xml-stamp
CLEANFILES = xmli18n-tmp.h xml-stamp xml-stamp.tmp

EXTRA_DIST = $(pkgxmldata_DATA)
