# This Makefile has two purposes:
# 1. Starting the compilation of Vim for Unix.
# 2. Creating the various distribution files.


# 1. Using this Makefile without an argument will compile Vim for Unix.
#
# "make install" is also possible.
#
# NOTE: If this doesn't work properly, first change directory to "src" and use
# the Makefile there:
#	cd src
#	make [arguments]
# Noticed on AIX systems when using this Makefile: Trying to run "cproto" or
# something else after Vim has been compiled.  Don't know why...
# Noticed on OS/390 Unix: Restarts configure.
#
# The first (default) target is "first".  This will result in running
# "make first", so that the target from "src/auto/config.mk" is picked
# up properly when config didn't run yet.  Doing "make all" before configure
# has run can result in compiling with $(CC) empty.

first:
	@echo "Starting make in the src directory."
	@echo "If there are problems, cd to the src directory and run make there"
	cd src && $(MAKE) $@

# Some make programs use the last target for the $@ default; put the other
# targets separately to always let $@ expand to "first" by default.
all install uninstall tools config configure proto depend lint tags types test testclean clean distclean:
	@echo "Starting make in the src directory."
	@echo "If there are problems, cd to the src directory and run make there"
	cd src && $(MAKE) $@


# 2. Create the various distributions:
#
# TARGET	PRODUCES		CONTAINS
# unixall	vim-#.#.tar.bz2		Runtime files and Sources for Unix
#    unixrt	vim-#.#-rt[12].tar.gz	Runtime files for Unix
#    unixsrc	vim-#.#-src[12].tar.gz	Sources for Unix
#
# extra		vim-#.#-extra.tar.gz	Extra source and runtime files
# lang		vim-#.#-lang.tar.gz	multi-language files
#
# html		vim##html.zip		HTML docs
#
# amisrc	vim##src.tgz		sources for Amiga
# amirt		vim##rt.tgz		runtime for Amiga
# amibin	vim##bin.tgz		binary for Amiga
#
# dossrc	vim##src.zip		sources for MS-DOS
# dosrt		vim##rt.zip		runtime for MS-DOS
# dosbin	vim##d16.zip		binary for MS-DOS 16 bits
#		vim##d32.zip		binary for MS-DOS 32 bits
#		vim##w32.zip		binary for Win32
#		gvim##.zip		binary for GUI Win32
#		gvim##ole.zip		OLE exe for Win32 GUI
#		gvim##_s.zip		exe for Win32s GUI
# doslang	vim##lang.zip		language files for Win32
#
# os2bin	vim##os2.zip		binary for OS/2
#					(use RT from dosrt)
#
# farsi		farsi##.zip		Farsi fonts
#
#    All output files are created in the "dist" directory.  Existing files are
#    overwritten!
#    To do all this you need the unixrt, unixsrc, extra and lang archives, and
#    compiled binaries.
#    Before creating an archive first delete all backup files, *.orig, etc.

MAJOR = 6
MINOR = 3

# Uncomment this line if the Win32s version is to be included.
#DOSBIN_S =  dosbin_s

# CHECKLIST for creating a new version:
#
# - Update Vim version number.  For a test version in: src/version.h, Contents,
#   MAJOR/MINOR above, VIMRTDIR and VERSION in src/Makefile, README*.txt,
#   runtime/doc/*.txt and nsis/gvim.nsi.  For a minor/major version:
#   src/GvimExt/GvimExt.reg, src/vim16.def.
# - Correct included_patches[] in src/version.c.
# - Compile Vim with GTK, Perl, Python, TCL, Ruby, Cscope and "huge" features.
# - With these features: "make proto" (requires cproto and Motif installed;
#   ignore warnings for missing include files, fix problems for syntax errors).
# - With these features: "make depend" (works best with gcc).
# - "make lint" and check the output (ignore GTK warnings).
# - Enable the efence library in "src/Makefile" and run "make test".  May
#   require disabling Python to avoid trouble with threads.
# - Check for missing entries in runtime/makemenu.vim (with checkmenu script).
# - Check for missing options in runtime/optwin.vim et al. (with check.vim).
# - Do "make menu" to update the runtime/synmenu.vim file.
# - Add remarks for changes to runtime/doc/version6.txt.
# - In runtime/doc run "make" and "make html" to check for errors.
# - Check if src/Makefile and src/feature.h don't contain any personal
#   preferences or the GTK, Perl, etc. mentioned above.
# - Check that runtime/doc/help.txt doesn't contain entries in "LOCAL
#   ADDITIONS".
# - Check file protections to be "644" for text and "755" for executables (run
#   the "check" script).
# - Check compiling on Amiga, MS-DOS and MS-Windows.
# - Delete all *~, *.sw?, *.orig, *.rej files
# - "make unixall", "make extra", "make lang", "make html"
#
# Amiga:
# - "make amisrc", move the archive to the Amiga and compile:
#   "make -f Make_manx.mak" (will use "big" features by default).
# - Run the tests: "make -f Make_manx.mak test"
# - Place the executables Vim and Xxd in this directory (set the executable
#   flag).
# - "make amirt", "make amibin".
#
# PC:
# - "make dossrc" and "make dosrt".  Unpack the archives on a PC.
# 16 bit DOS version:
# - Set environment for compiling with Borland C++ 3.1.
# - "bmake -f Make_bc3.mak BOR=E:\borlandc" (compiling xxd might fail, in that
#   case set environment for compiling with Borland C++ 4.0 and do
#   "make -f make_bc3.mak BOR=E:\BC4 xxd/xxd.exe").
# - "make test" and check the output.
# - Rename the executables to "vimd16.exe", "xxdd16.exe", "installd16.exe" and
#   "uninstald16.exe".
# 32 bit DOS version:
# - Set environment for compiling with DJGPP; "gmake -f Make_djg.mak".
# - "rm testdir/*.out", "make -f Make_djg.mak test" and check the output.
# - Rename the executables to "vimd32.exe", "xxdd32.exe", "installd32.exe" and
#   "uninstald32.exe".
# Win32 console version:
# - Set environment for Visual C++ 5.0: "vcvars32"
# - "nmake -f Make_mvc.mak"
# - "rm testdir/*.out", "nmake -f Make_mvc.mak test" and check the output.
# - Rename the executables to "vimw32.exe", "xxdw32.exe".
# - When building the Win32s version later, delete vimrun.exe, install.exe and
#   uninstal.exe.  Otherwise rename executables to installw32.exe and
#   uninstalw32.exe.
# Win32 GUI version:
# - "nmake -f Make_mvc.mak GUI=yes.
# - move "gvim.exe" to here (otherwise the OLE version will overwrite it).
# - Delete vimrun.exe, install.exe and uninstall.exe.
# - Copy "GvimExt/gvimext.dll" to here.
# Win32 GUI version with OLE, PERL, TCL, PYTHON and dynamic IME:
# - Run src/bigvim.bat ("nmake -f Make_mvc.mak GUI=yes OLE=yes IME=yes ...)
# - Rename "gvim.exe" to "gvim_ole.exe".
# - Delete install.exe and uninstall.exe.
# - If building the Win32s version delete vimrun.exe.
# Win32s GUI version:
# - Set environment for Visual C++ 4.1 (requires a new console window)
# - "vcvars32" (use the path for VC 4.1 e:\msdev\bin)
# - "nmake -f Make_mvc.mak GUI=yes INTL=no clean" (use the path for VC 4.1)
# - "nmake -f Make_mvc.mak GUI=yes INTL=no" (use the path for VC 4.1)
# - Rename "gvim.exe" to "gvim_w32s.exe".
# - Rename "install.exe" to "installw32.exe"
# - Rename "uninstal.exe" to "uninstalw32.exe"
# - The produced uninstalw32.exe and vimrun.exe are used.
# Create the archives:
# - Copy all the "*.exe" files to where this Makefile is.
# - "make dosbin".
# - Run make on Unix to update the ".mo" files.
# - "make doslang".
# NSIS self installing exe:
# - Unpack the doslang archive on the PC.
# - Make sure gvim_ole.exe, vimd32.exe, vimw32.exe, installw32.exe,
#   uninstalw32.exe and xxdw32.exe have been build as mentioned above.
# - put gvimext.dll in src/GvimExt and VisVim.dll in src/VisVim (get them
#   from a binary archive or build them)
# - make sure there is a diff.exe two levels up
# - go to ../nsis and do "makensis gvim.nsi".
# - Copy gvim##.exe to the dist directory.
#
# OS/2:
# - Unpack the Unix "src", "extra" and "rt" archives.
# - "make -f Make_os2.mak".
# - Rename the executables to vimos2.exe, xxdos2.exe and teeos2.exe and copy
#   them to here.
# - "make os2bin".

VIMVER	= vim-$(MAJOR).$(MINOR)
VERSION = $(MAJOR)$(MINOR)
VDOT	= $(MAJOR).$(MINOR)
VIMRTDIR = vim$(VERSION)

# Vim used for conversion from "unix" to "dos"
VIM	= vim

# source files for all source archives
SRC_ALL1 =	\
		src/README.txt \
		src/arabic.c \
		src/arabic.h \
		src/ascii.h \
		src/buffer.c \
		src/charset.c \
		src/diff.c \
		src/digraph.c \
		src/edit.c \
		src/eval.c \
		src/ex_cmds.c \
		src/ex_cmds.h \
		src/ex_cmds2.c \
		src/ex_docmd.c \
		src/ex_eval.c \
		src/ex_getln.c \
		src/farsi.c \
		src/farsi.h \
		src/feature.h \
		src/fileio.c \
		src/fold.c \
		src/getchar.c \
		src/globals.h \
		src/gui.c \
		src/gui.h \
		src/gui_beval.c \
		src/gui_beval.h \
		src/keymap.h \
		src/macros.h \
		src/main.c \
		src/mark.c \
		src/mbyte.c \
		src/memfile.c \
		src/memline.c \
		src/menu.c \
		src/message.c \
		src/misc1.c \
		src/misc2.c \
		src/move.c \
		src/mysign \
		src/nbdebug.c \
		src/nbdebug.h \
		src/netbeans.c \
		src/normal.c \
		src/ops.c \
		src/option.c \
		src/option.h \
		src/quickfix.c \
		src/regexp.c \
		src/regexp.h \
		src/screen.c \
		src/search.c \
		src/structs.h \
		src/syntax.c \
		src/tag.c \
		src/term.c \
		src/term.h \
		src/termlib.c \
		src/ui.c \
		src/undo.c \
		src/version.c \
		src/version.h \
		src/vim.h \
		src/window.c \
		src/xxd/xxd.c \

SRC_ALL2 =	\
		src/main.aap \
		src/testdir/main.aap \
		src/testdir/*.in \
		src/testdir/*.ok \
		src/testdir/test49.vim \
		src/proto.h \
		src/proto/buffer.pro \
		src/proto/charset.pro \
		src/proto/diff.pro \
		src/proto/digraph.pro \
		src/proto/edit.pro \
		src/proto/eval.pro \
		src/proto/ex_cmds.pro \
		src/proto/ex_cmds2.pro \
		src/proto/ex_docmd.pro \
		src/proto/ex_eval.pro \
		src/proto/ex_getln.pro \
		src/proto/fileio.pro \
		src/proto/fold.pro \
		src/proto/getchar.pro \
		src/proto/gui.pro \
		src/proto/gui_beval.pro \
		src/proto/main.pro \
		src/proto/mark.pro \
		src/proto/mbyte.pro \
		src/proto/memfile.pro \
		src/proto/memline.pro \
		src/proto/menu.pro \
		src/proto/message.pro \
		src/proto/misc1.pro \
		src/proto/misc2.pro \
		src/proto/move.pro \
		src/proto/netbeans.pro \
		src/proto/normal.pro \
		src/proto/ops.pro \
		src/proto/option.pro \
		src/proto/quickfix.pro \
		src/proto/regexp.pro \
		src/proto/screen.pro \
		src/proto/search.pro \
		src/proto/syntax.pro \
		src/proto/tag.pro \
		src/proto/term.pro \
		src/proto/termlib.pro \
		src/proto/ui.pro \
		src/proto/undo.pro \
		src/proto/version.pro \
		src/proto/window.pro \


# source files for Unix only
SRC_UNIX =	\
		Makefile \
		README_src.txt \
		configure \
		pixmaps/*.xpm \
		pixmaps/gen-inline-pixbufs.sh \
		pixmaps/stock_icons.h \
		src/INSTALL \
		src/Makefile \
		src/auto/configure \
		src/config.aap.in \
		src/config.h.in \
		src/config.mk.dist \
		src/config.mk.in \
		src/configure \
		src/configure.in \
		src/gui_at_fs.c \
		src/gui_at_sb.c \
		src/gui_at_sb.h \
		src/gui_athena.c \
		src/gui_gtk.c \
		src/gui_gtk_f.c \
		src/gui_gtk_f.h \
		src/gui_gtk_x11.c \
		src/gui_motif.c \
		src/gui_x11.c \
		src/hangulin.c \
		src/if_xcmdsrv.c \
		src/integration.c \
		src/integration.h \
		src/link.sh \
		src/mkinstalldirs \
		src/os_unix.c \
		src/os_unix.h \
		src/os_unixx.h \
		src/osdef.sh \
		src/osdef1.h.in \
		src/osdef2.h.in \
		src/pathdef.sh \
		src/proto/gui_athena.pro \
		src/proto/gui_gtk.pro \
		src/proto/gui_gtk_x11.pro \
		src/proto/gui_motif.pro \
		src/proto/gui_x11.pro \
		src/proto/hangulin.pro \
		src/proto/if_xcmdsrv.pro \
		src/proto/os_unix.pro \
		src/proto/pty.pro \
		src/proto/workshop.pro \
		src/pty.c \
		src/testdir/Makefile \
		src/testdir/unix.vim \
		src/toolcheck \
		src/vim_icon.xbm \
		src/vim_mask.xbm \
		src/vimtutor \
		src/which.sh \
		src/workshop.c \
		src/workshop.h \
		src/wsdebug.c \
		src/wsdebug.h \
		src/xxd/Makefile \

# source files for both DOS and Unix
SRC_DOS_UNIX =	\
		src/if_cscope.c \
		src/if_cscope.h \
		src/if_perl.xs \
		src/if_perlsfio.c \
		src/if_python.c \
		src/if_ruby.c \
		src/if_tcl.c \
		src/proto/if_cscope.pro \
		src/proto/if_perl.pro \
		src/proto/if_perlsfio.pro \
		src/proto/if_python.pro \
		src/proto/if_ruby.pro \
		src/proto/if_tcl.pro \
		src/typemap \

# source files for DOS (also in the extra archive)
SRC_DOS =	\
		src/GvimExt \
		README_srcdos.txt \
		src/INSTALLpc.txt \
		src/Make_bc3.mak \
		src/Make_bc5.mak \
		src/Make_cyg.mak \
		src/Make_djg.mak \
		src/Make_ivc.mak \
		src/Make_dvc.mak \
		src/Make_ming.mak \
		src/Make_mvc.mak \
		src/Make_w16.mak \
		src/bigvim.bat \
		src/dimm.idl \
		src/dlldata.c \
		src/dosinst.c \
		src/dosinst.h \
		src/glbl_ime.cpp \
		src/glbl_ime.h \
		src/gui_w16.c \
		src/gui_w32.c \
		src/gui_w48.c \
		src/guiw16rc.h \
		src/gui_w32_rc.h \
		src/if_ole.cpp \
		src/if_ole.h \
		src/if_ole.idl \
		src/iid_ole.c \
		src/os_dos.h \
		src/os_msdos.c \
		src/os_msdos.h \
		src/os_w32dll.c \
		src/os_w32exe.c \
		src/os_win16.c \
		src/os_win32.c \
		src/os_mswin.c \
		src/os_win16.h \
		src/os_win32.h \
		src/proto/gui_w16.pro \
		src/proto/gui_w32.pro \
		src/proto/if_ole.pro \
		src/proto/os_msdos.pro \
		src/proto/os_win16.pro \
		src/proto/os_win32.pro \
		src/proto/os_mswin.pro \
		src/testdir/Make_dos.mak \
		src/testdir/dos.vim \
		src/uninstal.c \
		src/vim.def \
		src/vim.rc \
		src/gvim.exe.mnf \
		src/vim16.def \
		src/vim16.rc \
		src/vimrun.c \
		src/vimtbar.h \
		src/xpm_w32.c \
		src/xpm_w32.h \
		src/xxd/Make_bc3.mak \
		src/xxd/Make_bc5.mak \
		src/xxd/Make_cyg.mak \
		src/xxd/Make_djg.mak \
		src/xxd/Make_mvc.mak \
		nsis/gvim.nsi \
		nsis/README.txt \
		uninstal.txt \
		src/VisVim/Commands.cpp \
		src/VisVim/Commands.h \
		src/VisVim/DSAddIn.cpp \
		src/VisVim/DSAddIn.h \
		src/VisVim/OleAut.cpp \
		src/VisVim/OleAut.h \
		src/VisVim/README_VisVim.txt \
		src/VisVim/Reg.cpp \
		src/VisVim/Register.bat \
		src/VisVim/Resource.h \
		src/VisVim/StdAfx.cpp \
		src/VisVim/StdAfx.h \
		src/VisVim/UnRegist.bat \
		src/VisVim/VisVim.cpp \
		src/VisVim/VisVim.def \
		src/VisVim/VisVim.mak \
		src/VisVim/VisVim.h \
		src/VisVim/VisVim.odl \
		src/VisVim/VisVim.rc \
		src/VisVim/VsReadMe.txt \

# source files for DOS without CR/LF translation (also in the extra archive)
SRC_DOS_BIN =	\
		src/VisVim/Res \
		src/tearoff.bmp \
		src/tools.bmp \
		src/tools16.bmp \
		src/vim*.ico \
		src/vim.tlb \
		src/vimtbar.lib \
		src/vimtbar.dll \
		nsis/icons \

# source files for Amiga, DOS, etc. (also in the extra archive)
SRC_AMI_DOS =	\

# source files for Amiga (also in the extra archive)
SRC_AMI =	\
		README_amisrc.txt \
		README_amisrc.txt.info \
		src.info \
		src/INSTALLami.txt \
		src/Make_agui.mak \
		src/Make_aros.mak \
		src/Make_dice.mak \
		src/Make_manx.mak \
		src/Make_morph.mak \
		src/Make_sas.mak \
		src/gui_amiga.c \
		src/gui_amiga.h \
		src/os_amiga.c \
		src/os_amiga.h \
		src/proto/gui_amiga.pro \
		src/proto/os_amiga.pro \
		src/testdir/Make_amiga.mak \
		src/testdir/amiga.vim \
		src/xxd/Make_amiga.mak \

# source files for the Mac (also in the extra archive)
SRC_MAC =	\
		src/INSTALLmac.txt \
		src/Make_mpw.mak \
		src/dehqx.py \
		src/gui_mac.c \
		src/gui_mac.icns \
		src/gui_mac.r \
		src/os_mac* \
		src/proto/gui_mac.pro \
		src/proto/os_mac.pro \

# source files for VMS (in the extra archive)
SRC_VMS =	\
		src/INSTALLvms.txt \
		src/Make_vms.mms \
		src/gui_gtk_vms.h \
		src/os_vms.c \
		src/os_vms_conf.h \
		src/os_vms_mms.c \
		src/proto/os_vms.pro \
		src/testdir/Make_vms.mms \
		src/testdir/vms.vim \
		src/xxd/Make_vms.mms \
		vimtutor.com \

# source files for OS/2 (in the extra archive)
SRC_OS2 =	\
		src/Make_os2.mak \
		src/os_os2_cfg.h \
		src/testdir/Make_os2.mak \
		src/testdir/todos.vim \
		src/testdir/os2.vim \
		src/xxd/Make_os2.mak \

# source files for QNX (in the extra archive)
SRC_QNX =	\
		src/os_qnx.c \
		src/os_qnx.h \
		src/gui_photon.c \
		src/proto/gui_photon.pro \
		src/proto/os_qnx.pro \


# source files for the extra archive (all sources that are not for Unix)
SRC_EXTRA =	\
		$(SRC_AMI) \
		$(SRC_AMI_DOS) \
		$(SRC_DOS) \
		$(SRC_DOS_BIN) \
		$(SRC_MAC) \
		$(SRC_OS2) \
		$(SRC_QNX) \
		$(SRC_VMS) \
		README_os390.txt \
		src/Make_mint.mak \
		src/Make_ro.mak \
		src/gui_beos.cc \
		src/gui_beos.h \
		src/gui_riscos.c \
		src/gui_riscos.h \
		src/if_sniff.c \
		src/if_sniff.h \
		src/infplist.xml \
		src/link.390 \
		src/os_beos.c \
		src/os_beos.h \
		src/os_beos.rsrc \
		src/os_mint.h \
		src/os_riscos.c \
		src/os_riscos.h \
		src/proto/gui_beos.pro \
		src/proto/gui_riscos.pro \
		src/proto/os_beos.pro \
		src/proto/os_riscos.pro \
		src/os_vms_fix.com \
		src/toolbar.phi \

# runtime files for all distributions
RT_ALL =	\
		README.txt \
		runtime/bugreport.vim \
		runtime/doc/*.awk \
		runtime/doc/*.pl \
		runtime/doc/*.txt \
		runtime/doc/Makefile \
		runtime/doc/doctags.c \
		runtime/doc/vim.1 \
		runtime/doc/evim.1 \
		runtime/doc/vimdiff.1 \
		runtime/doc/vimtutor.1 \
		runtime/doc/xxd.1 \
		runtime/ftoff.vim \
		runtime/gvimrc_example.vim \
		runtime/macros/README.txt \
		runtime/macros/dvorak \
		runtime/macros/hanoi/click.me \
		runtime/macros/hanoi/hanoi.vim \
		runtime/macros/hanoi/poster \
		runtime/macros/justify.vim \
		runtime/macros/less.sh \
		runtime/macros/less.vim \
		runtime/macros/life/click.me \
		runtime/macros/life/life.vim \
		runtime/macros/matchit.vim \
		runtime/macros/matchit.txt \
		runtime/macros/maze/README.txt \
		runtime/macros/maze/[mM]akefile \
		runtime/macros/maze/main.aap \
		runtime/macros/maze/maze.c \
		runtime/macros/maze/maze_5.78 \
		runtime/macros/maze/maze_mac \
		runtime/macros/maze/mazeansi.c \
		runtime/macros/maze/mazeclean.c \
		runtime/macros/maze/poster \
		runtime/macros/shellmenu.vim \
		runtime/macros/swapmous.vim \
		runtime/macros/urm/README.txt \
		runtime/macros/urm/examples \
		runtime/macros/urm/urm \
		runtime/macros/urm/urm.vim \
		runtime/mswin.vim \
		runtime/evim.vim \
		runtime/optwin.vim \
		runtime/ftplugin.vim \
		runtime/ftplugof.vim \
		runtime/indent.vim \
		runtime/indoff.vim \
		runtime/termcap \
		runtime/tools/README.txt \
		runtime/tools/[a-z]*[a-z0-9] \
		runtime/tutor/README.txt \
		runtime/tutor/tutor \
		runtime/tutor/tutor.vim \
		runtime/vimrc_example.vim \

# runtime files for all distributions without CR-NL translation
RT_ALL_BIN =	\
		runtime/doc/tags \
		runtime/print/*.ps \

# runtime script files
RT_SCRIPTS =	\
		runtime/filetype.vim \
		runtime/scripts.vim \
		runtime/menu.vim \
		runtime/delmenu.vim \
		runtime/synmenu.vim \
		runtime/makemenu.vim \
		runtime/colors/*.vim \
		runtime/colors/README.txt \
		runtime/compiler/*.vim \
		runtime/compiler/README.txt \
		runtime/indent/*.vim \
		runtime/indent/README.txt \
		runtime/ftplugin/*.vim \
		runtime/ftplugin/README.txt \
		runtime/plugin/*.vim \
		runtime/plugin/README.txt \
		runtime/syntax/*.vim \
		runtime/syntax/README.txt \

# Unix runtime
RT_UNIX =	\
		README_unix.txt \
		runtime/vim16x16.png \
		runtime/vim16x16.xpm \
		runtime/vim32x32.png \
		runtime/vim32x32.xpm \
		runtime/vim48x48.png \
		runtime/vim48x48.xpm \

# Unix and DOS runtime without CR-LF translation
RT_UNIX_DOS_BIN =	\
		runtime/vim16x16.gif \
		runtime/vim32x32.gif \
		runtime/vim48x48.gif \

# runtime not for unix or extra
RT_NO_UNIX =	\

# runtime for Amiga (also in the extra archive)
RT_AMI_DOS =	\
		runtime/doc/vim.man \
		runtime/doc/vimdiff.man \
		runtime/doc/vimtutor.man \
		runtime/doc/xxd.man \

# DOS runtime (also in the extra archive)
RT_DOS =	\
		README_dos.txt \
		runtime/rgb.txt \
		vimtutor.bat \

# DOS runtime without CR-LF translation (also in the extra archive)
RT_DOS_BIN =	\
		runtime/vimlogo.cdr \
		runtime/vimlogo.eps \
		runtime/vimlogo.gif \
		runtime/vimlogo.pdf \

# Amiga runtime (also in the extra archive)
RT_AMI =	\
		README.txt.info \
		README_ami.txt \
		README_ami.txt.info \
		libs/arp.library \
		runtime/doc.info \
		runtime/doc/*.info \
		runtime/icons \
		runtime/icons.info \
		runtime/macros.info \
		runtime/macros/*.info \
		runtime/macros/hanoi/*.info \
		runtime/macros/life/*.info \
		runtime/macros/maze/*.info \
		runtime/macros/urm/*.info \
		runtime/tools.info \
		runtime/tutor.info \
		runtime/tutor/*.info \

# runtime files in extra archive
RT_EXTRA =	\
		$(RT_AMI) \
		$(RT_AMI_DOS) \
		$(RT_DOS) \
		$(RT_DOS_BIN) \
		README_mac.txt \

# included in all Amiga archives
ROOT_AMI =	\
		Contents \
		Contents.info \
		runtime.info \
		vimdir.info \

# root files for the extra archive
ROOT_EXTRA =	\
		$(ROOT_AMI) \

# files for Amiga small binary (also in extra archive)
BIN_AMI =	\
		README_amibin.txt \
		README_amibin.txt.info \
		Vim.info \
		Xxd.info \

# files for DOS binary (also in extra archive)
BIN_DOS =	\
		README_bindos.txt \
		uninstal.txt \

# files for Win32 OLE binary (also in extra archive)
BIN_OLE =	\
		README_ole.txt \

# files for Win32s binary (also in extra archive)
BIN_W32S =	\
		README_w32s.txt \

# files for VMS binary (also in extra archive)
BIN_VMS =	\
		README_vms.txt \

# files for OS/2 binary (also in extra archive)
BIN_OS2 =	\
		README_os2.txt \

# binary files for extra archive
BIN_EXTRA =	\
		$(BIN_AMI) \
		$(BIN_DOS) \
		$(BIN_OLE) \
		$(BIN_W32S) \
		$(BIN_VMS) \
		$(BIN_OS2) \

# all files for extra archive
EXTRA =		\
		$(BIN_EXTRA) \
		$(ROOT_EXTRA) \
		$(RT_EXTRA) \
		$(SRC_EXTRA) \
		README_extra.txt \
		src/VisVim/VisVim.dll \
		farsi \
		runtime/vimlogo.xpm \
		src/swis.s \
		src/tee/Makefile* \
		src/tee/tee.c \
		csdpmi4b.zip \
		emx.dll \
		emxlibcs.dll \

# generic language files
LANG_GEN = \
		README_lang.txt \
		runtime/lang/README.txt \
		runtime/lang/menu_*.vim \
		runtime/keymap/README.txt \
		runtime/keymap/*.vim \
		runtime/tutor/README.*.txt \
		runtime/tutor/Makefile \
		runtime/tutor/tutor.?? \
		runtime/tutor/tutor.gr.* \
		runtime/tutor/tutor.ja.* \
		runtime/tutor/tutor.ko.* \
		runtime/tutor/tutor.pl.* \
		runtime/tutor/tutor.ru.* \
		runtime/tutor/tutor.zh.* \

# all files for lang archive
LANG_SRC = \
		src/po/README.txt \
		src/po/README_mingw.txt \
		src/po/README_mvc.txt \
		src/po/cleanup.vim \
		src/po/Makefile \
		src/po/Make_ming.mak \
		src/po/Make_mvc.mak \
		src/po/sjiscorr.c \
		src/po/*.po \

# the language files for the Win32 lang archive
LANG_DOS = \
		src/po/*.mo \

# All output is put in the "dist" directory.
dist:
	mkdir dist

# Clean up some files to avoid they are included.
prepare:
	if test -f runtime/doc/uganda.nsis.txt; then \
		rm runtime/doc/uganda.nsis.txt; fi

# For the zip files we need to create a file with the comment line
dist/comment:
	mkdir dist/comment

COMMENT_RT = comment/$(VERSION)-rt
COMMENT_RT1 = comment/$(VERSION)-rt1
COMMENT_RT2 = comment/$(VERSION)-rt2
COMMENT_D16 = comment/$(VERSION)-bin-d16
COMMENT_D32 = comment/$(VERSION)-bin-d32
COMMENT_W32 = comment/$(VERSION)-bin-w32
COMMENT_GVIM = comment/$(VERSION)-bin-gvim
COMMENT_OLE = comment/$(VERSION)-bin-ole
COMMENT_W32S = comment/$(VERSION)-bin-w32s
COMMENT_SRC = comment/$(VERSION)-src
COMMENT_OS2 = comment/$(VERSION)-bin-os2
COMMENT_HTML = comment/$(VERSION)-html
COMMENT_FARSI = comment/$(VERSION)-farsi
COMMENT_LANG = comment/$(VERSION)-lang

dist/$(COMMENT_RT): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) runtime files for MS-DOS and MS-Windows" > dist/$(COMMENT_RT)

dist/$(COMMENT_RT1): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) runtime files (PART 1) for MS-DOS and MS-Windows" > dist/$(COMMENT_RT1)

dist/$(COMMENT_RT2): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) runtime files (PART 2) for MS-DOS and MS-Windows" > dist/$(COMMENT_RT2)

dist/$(COMMENT_D16): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) binaries for MS-DOS 16 bit real mode" > dist/$(COMMENT_D16)

dist/$(COMMENT_D32): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) binaries for MS-DOS 32 bit protected mode" > dist/$(COMMENT_D32)

dist/$(COMMENT_W32): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) binaries for MS-Windows NT/95" > dist/$(COMMENT_W32)

dist/$(COMMENT_GVIM): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) GUI binaries for MS-Windows NT/95" > dist/$(COMMENT_GVIM)

dist/$(COMMENT_OLE): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) MS-Windows GUI binaries with OLE support" > dist/$(COMMENT_OLE)

dist/$(COMMENT_W32S): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) GUI binaries for MS-Windows 3.1/3.11" > dist/$(COMMENT_W32S)

dist/$(COMMENT_SRC): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) sources for MS-DOS and MS-Windows" > dist/$(COMMENT_SRC)

dist/$(COMMENT_OS2): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) binaries + runtime files for OS/2" > dist/$(COMMENT_OS2)

dist/$(COMMENT_HTML): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) documentation in HTML" > dist/$(COMMENT_HTML)

dist/$(COMMENT_FARSI): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) Farsi language files" > dist/$(COMMENT_FARSI)

dist/$(COMMENT_LANG): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) MS-Windows language files" > dist/$(COMMENT_LANG)

unixrt: dist prepare
	-rm -f dist/$(VIMVER)-rt1.tar.gz
	-rm -f dist/$(VIMVER)-rt2.tar.gz
# first runtime file
	-rm -rf dist/$(VIMRTDIR)
	mkdir dist/$(VIMRTDIR)
	tar cf - \
		$(RT_ALL) \
		$(RT_ALL_BIN) \
		$(RT_UNIX) \
		$(RT_UNIX_DOS_BIN) \
		| (cd dist/$(VIMRTDIR); tar xf -)
	cd dist && tar cf $(VIMVER)-rt1.tar $(VIMRTDIR)
	gzip -9 dist/$(VIMVER)-rt1.tar
# second runtime file (script and language files)
	-rm -rf dist/$(VIMRTDIR)
	mkdir dist/$(VIMRTDIR)
	tar cf - \
		$(RT_SCRIPTS) \
		$(LANG_GEN) \
		| (cd dist/$(VIMRTDIR); tar xf -)
	cd dist && tar cf $(VIMVER)-rt2.tar $(VIMRTDIR)
	gzip -9 dist/$(VIMVER)-rt2.tar

unixsrc: dist prepare
	-rm -f dist/$(VIMVER)-src1.tar.gz
	-rm -f dist/$(VIMVER)-src2.tar.gz
# first source file
	-rm -rf dist/$(VIMRTDIR)
	mkdir dist/$(VIMRTDIR)
	tar cf - \
		$(SRC_ALL1) \
		| (cd dist/$(VIMRTDIR); tar xf -)
	cd dist && tar cf $(VIMVER)-src1.tar $(VIMRTDIR)
	gzip -9 dist/$(VIMVER)-src1.tar
# second source file
	-rm -rf dist/$(VIMRTDIR)
	mkdir dist/$(VIMRTDIR)
	tar cf - \
		$(SRC_ALL2) \
		$(SRC_UNIX) \
		$(SRC_DOS_UNIX) \
		| (cd dist/$(VIMRTDIR); tar xf -)
# Need to use a "distclean" config.mk file
	cp -f src/config.mk.dist dist/$(VIMRTDIR)/src/auto/config.mk
# Create an empty config.h file, make dependencies require it
	touch dist/$(VIMRTDIR)/src/auto/config.h
# Make sure configure is newer than config.mk to force it to be generated
	touch dist/$(VIMRTDIR)/src/configure
	cd dist && tar cf $(VIMVER)-src2.tar $(VIMRTDIR)
	gzip -9 dist/$(VIMVER)-src2.tar

unixall: dist unixsrc unixrt
	-rm -f dist/$(VIMVER).tar.bz2
	-rm -rf dist/$(VIMRTDIR)
	mkdir dist/$(VIMRTDIR)
	cd dist && tar xfz $(VIMVER)-src1.tar.gz
	cd dist && tar xfz $(VIMVER)-src2.tar.gz
	cd dist && tar xfz $(VIMVER)-rt1.tar.gz
	cd dist && tar xfz $(VIMVER)-rt2.tar.gz
# Create an empty config.h file, make dependencies require it
	touch dist/$(VIMRTDIR)/src/auto/config.h
# Make sure configure is newer than config.mk to force it to be generated
	touch dist/$(VIMRTDIR)/src/configure
	cd dist && tar cf $(VIMVER).tar $(VIMRTDIR)
	bzip2 dist/$(VIMVER).tar

extra: dist prepare
	-rm -f dist/$(VIMVER)-extra.tar.gz
	-rm -rf dist/$(VIMRTDIR)
	mkdir dist/$(VIMRTDIR)
	tar cf - \
		$(EXTRA) \
		| (cd dist/$(VIMRTDIR); tar xf -)
	cd dist && tar cf $(VIMVER)-extra.tar $(VIMRTDIR)
	gzip -9 dist/$(VIMVER)-extra.tar

lang: dist prepare
	-rm -f dist/$(VIMVER)-lang.tar.gz
	-rm -rf dist/$(VIMRTDIR)
	mkdir dist/$(VIMRTDIR)
	tar cf - \
		$(LANG_SRC) \
		| (cd dist/$(VIMRTDIR); tar xf -)
# Make sure ja.sjis.po is newer than ja.po to avoid it being regenerated.
# Same for cs.cp1250.po, pl.cp1250.po and sk.cp1250.po.
	touch dist/$(VIMRTDIR)/src/po/ja.sjis.po
	touch dist/$(VIMRTDIR)/src/po/cs.cp1250.po
	touch dist/$(VIMRTDIR)/src/po/pl.cp1250.po
	touch dist/$(VIMRTDIR)/src/po/sk.cp1250.po
	touch dist/$(VIMRTDIR)/src/po/zh_CN.cp936.po
	touch dist/$(VIMRTDIR)/src/po/ru.cp1251.po
	cd dist && tar cf $(VIMVER)-lang.tar $(VIMRTDIR)
	gzip -9 dist/$(VIMVER)-lang.tar

amirt: dist prepare
	-rm -f dist/vim$(VERSION)rt.tar.gz
	-rm -rf dist/Vim
	mkdir dist/Vim
	mkdir dist/Vim/$(VIMRTDIR)
	tar cf - \
		$(ROOT_AMI) \
		$(RT_ALL) \
		$(RT_ALL_BIN) \
		$(RT_SCRIPTS) \
		$(RT_AMI) \
		$(RT_NO_UNIX) \
		$(RT_AMI_DOS) \
		| (cd dist/Vim/$(VIMRTDIR); tar xf -)
	mv dist/Vim/$(VIMRTDIR)/vimdir.info dist/Vim.info
	mv dist/Vim/$(VIMRTDIR)/runtime.info dist/Vim/$(VIMRTDIR).info
	mv dist/Vim/$(VIMRTDIR)/runtime/* dist/Vim/$(VIMRTDIR)
	rmdir dist/Vim/$(VIMRTDIR)/runtime
	cd dist && tar cf vim$(VERSION)rt.tar Vim Vim.info
	gzip -9 dist/vim$(VERSION)rt.tar
	mv dist/vim$(VERSION)rt.tar.gz dist/vim$(VERSION)rt.tgz

amibin: dist prepare
	-rm -f dist/vim$(VERSION)bin.tar.gz
	-rm -rf dist/Vim
	mkdir dist/Vim
	mkdir dist/Vim/$(VIMRTDIR)
	tar cf - \
		$(ROOT_AMI) \
		$(BIN_AMI) \
		Vim \
		Xxd \
		| (cd dist/Vim/$(VIMRTDIR); tar xf -)
	mv dist/Vim/$(VIMRTDIR)/vimdir.info dist/Vim.info
	mv dist/Vim/$(VIMRTDIR)/runtime.info dist/Vim/$(VIMRTDIR).info
	cd dist && tar cf vim$(VERSION)bin.tar Vim Vim.info
	gzip -9 dist/vim$(VERSION)bin.tar
	mv dist/vim$(VERSION)bin.tar.gz dist/vim$(VERSION)bin.tgz

amisrc: dist prepare
	-rm -f dist/vim$(VERSION)src.tar.gz
	-rm -rf dist/Vim
	mkdir dist/Vim
	mkdir dist/Vim/$(VIMRTDIR)
	tar cf - \
		$(ROOT_AMI) \
		$(SRC_ALL1) \
		$(SRC_ALL2) \
		$(SRC_AMI) \
		$(SRC_AMI_DOS) \
		| (cd dist/Vim/$(VIMRTDIR); tar xf -)
	mv dist/Vim/$(VIMRTDIR)/vimdir.info dist/Vim.info
	mv dist/Vim/$(VIMRTDIR)/runtime.info dist/Vim/$(VIMRTDIR).info
	cd dist && tar cf vim$(VERSION)src.tar Vim Vim.info
	gzip -9 dist/vim$(VERSION)src.tar
	mv dist/vim$(VERSION)src.tar.gz dist/vim$(VERSION)src.tgz

no_title.vim: Makefile
	echo "set notitle noicon nocp nomodeline viminfo=" >no_title.vim

dosrt: dist dist/$(COMMENT_RT) dosrt_unix2dos
	-rm -rf dist/vim$(VERSION)rt.zip
	cd dist && zip -9 -rD -z vim$(VERSION)rt.zip vim <$(COMMENT_RT)

dosrt_unix2dos: dist prepare no_title.vim
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(RT_ALL) \
		$(RT_SCRIPTS) \
		$(RT_DOS) \
		$(RT_NO_UNIX) \
		$(RT_AMI_DOS) \
		$(LANG_GEN) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	tar cf - \
		$(RT_UNIX_DOS_BIN) \
		$(RT_ALL_BIN) \
		$(RT_DOS_BIN) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	mv dist/vim/$(VIMRTDIR)/runtime/* dist/vim/$(VIMRTDIR)
	rmdir dist/vim/$(VIMRTDIR)/runtime


# Convert runtime files from Unix fileformat to dos fileformat.
# Used before uploading.  Don't delete the AAPDIR/sign files!
runtime_unix2dos: dosrt_unix2dos
	-rm -rf `find runtime/dos -type f -print | sed -e /AAPDIR/d`
	cd dist/vim/$(VIMRTDIR); tar cf - * \
		| (cd ../../../runtime/dos; tar xf -)

dosbin: prepare dosbin_gvim dosbin_w32 dosbin_d32 dosbin_d16 dosbin_ole $(DOSBIN_S)

# make Win32 gvim
dosbin_gvim: dist no_title.vim dist/$(COMMENT_GVIM)
	-rm -rf dist/gvim$(VERSION).zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	cp gvim.exe dist/vim/$(VIMRTDIR)/gvim.exe
	cp xxdw32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp vimrun.exe dist/vim/$(VIMRTDIR)/vimrun.exe
	cp installw32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstalw32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cp gvimext.dll dist/vim/$(VIMRTDIR)/gvimext.dll
	cd dist && zip -9 -rD -z gvim$(VERSION).zip vim <$(COMMENT_GVIM)

# make Win32 console
dosbin_w32: dist no_title.vim dist/$(COMMENT_W32)
	-rm -rf dist/vim$(VERSION)w32.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	cp vimw32.exe dist/vim/$(VIMRTDIR)/vim.exe
	cp xxdw32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp installw32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstalw32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cd dist && zip -9 -rD -z vim$(VERSION)w32.zip vim <$(COMMENT_W32)

# make 32bit DOS
dosbin_d32: dist no_title.vim dist/$(COMMENT_D32)
	-rm -rf dist/vim$(VERSION)d32.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	cp vimd32.exe dist/vim/$(VIMRTDIR)/vim.exe
	cp xxdd32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp installd32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstald32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cp csdpmi4b.zip dist/vim/$(VIMRTDIR)
	cd dist && zip -9 -rD -z vim$(VERSION)d32.zip vim <$(COMMENT_D32)

# make 16bit DOS
dosbin_d16: dist no_title.vim dist/$(COMMENT_D16)
	-rm -rf dist/vim$(VERSION)d16.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	cp vimd16.exe dist/vim/$(VIMRTDIR)/vim.exe
	cp xxdd16.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp installd16.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstald16.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cd dist && zip -9 -rD -z vim$(VERSION)d16.zip vim <$(COMMENT_D16)

# make Win32 gvim with OLE
dosbin_ole: dist no_title.vim dist/$(COMMENT_OLE)
	-rm -rf dist/gvim$(VERSION)ole.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	cp gvim_ole.exe dist/vim/$(VIMRTDIR)/gvim.exe
	cp xxdw32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp vimrun.exe dist/vim/$(VIMRTDIR)/vimrun.exe
	cp installw32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstalw32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cp gvimext.dll dist/vim/$(VIMRTDIR)/gvimext.dll
	cp README_ole.txt dist/vim/$(VIMRTDIR)
	cp src/VisVim/VisVim.dll dist/vim/$(VIMRTDIR)/VisVim.dll
	cp src/VisVim/README_VisVim.txt dist/vim/$(VIMRTDIR)
	cd dist && zip -9 -rD -z gvim$(VERSION)ole.zip vim <$(COMMENT_OLE)

# make Win32s gvim
dosbin_s: dist no_title.vim dist/$(COMMENT_W32S)
	-rm -rf dist/gvim$(VERSION)_s.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	cp gvim_w32s.exe dist/vim/$(VIMRTDIR)/gvim.exe
	cp xxdd32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp README_w32s.txt dist/vim/$(VIMRTDIR)
	cp installw32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstalw32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cd dist && zip -9 -rD -z gvim$(VERSION)_s.zip vim <$(COMMENT_W32S)

# make Win32 lang archive
doslang: dist prepare no_title.vim dist/$(COMMENT_LANG)
	-rm -rf dist/vim$(VERSION)lang.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	mkdir dist/vim/$(VIMRTDIR)/lang
	cd src && MAKEMO=yes $(MAKE) languages
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
# Add the message translations.  Trick: skip ja.mo and use ja.sjis.mo instead.
# Same for cs.mo / cs.cp1250.mo, pl.mo / pl.cp1250.mo, sk.mo / sk.cp1250.mo,
# zh_CN.mo / zh_CN.cp936.mo and ru.mo / ru.cp1251.mo.
	for i in $(LANG_DOS); do \
	      if test "$$i" != "src/po/ja.mo" -a "$$i" != "src/po/pl.mo" -a "$$i" != "src/po/cs.mo" -a "$$i" != "src/po/sk.mo" -a "$$i" != "src/po/zh_CN.mo" -a "$$i" != "src/po/ru.mo"; then \
		n=`echo $$i | sed -e "s+src/po/\([-a-zA-Z0-9_]*\(.UTF-8\)*\)\(.sjis\)*\(.cp1250\)*\(.cp1251\)*\(.cp936\)*.mo+\1+"`; \
		mkdir dist/vim/$(VIMRTDIR)/lang/$$n; \
		mkdir dist/vim/$(VIMRTDIR)/lang/$$n/LC_MESSAGES; \
		cp $$i dist/vim/$(VIMRTDIR)/lang/$$n/LC_MESSAGES/vim.mo; \
	      fi \
	    done
	cp libintl.dll dist/vim/$(VIMRTDIR)/
	cd dist && zip -9 -rD -z vim$(VERSION)lang.zip vim <$(COMMENT_LANG)

# MS-DOS sources
dossrc: dist no_title.vim dist/$(COMMENT_SRC) runtime/doc/uganda.nsis.txt
	-rm -rf dist/vim$(VERSION)src.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(SRC_ALL1) \
		$(SRC_ALL2) \
		$(SRC_DOS) \
		$(SRC_AMI_DOS) \
		$(SRC_DOS_UNIX) \
		runtime/doc/uganda.nsis.txt \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	mv dist/vim/$(VIMRTDIR)/runtime/* dist/vim/$(VIMRTDIR)
	rmdir dist/vim/$(VIMRTDIR)/runtime
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	tar cf - \
		$(SRC_DOS_BIN) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	cd dist && zip -9 -rD -z vim$(VERSION)src.zip vim <$(COMMENT_SRC)

runtime/doc/uganda.nsis.txt: runtime/doc/uganda.txt
	cd runtime/doc && $(MAKE) uganda.nsis.txt

os2bin: dist no_title.vim dist/$(COMMENT_OS2)
	-rm -rf dist/vim$(VERSION)os2.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_OS2) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -u no_title.vim -c ":set tx|wq" {} \;
	cp vimos2.exe dist/vim/$(VIMRTDIR)/vim.exe
	cp xxdos2.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp teeos2.exe dist/vim/$(VIMRTDIR)/tee.exe
	cp emx.dll emxlibcs.dll dist/vim/$(VIMRTDIR)
	cd dist && zip -9 -rD -z vim$(VERSION)os2.zip vim <$(COMMENT_OS2)

html: dist dist/$(COMMENT_HTML)
	-rm -rf dist/vim$(VERSION)html.zip
	cd runtime/doc && zip -9 -z ../../dist/vim$(VERSION)html.zip *.html <../../dist/$(COMMENT_HTML)

farsi: dist dist/$(COMMENT_FARSI)
	-rm -f dist/farsi$(VERSION).zip
	zip -9 -rD -z dist/farsi$(VERSION).zip farsi < dist/$(COMMENT_FARSI)
