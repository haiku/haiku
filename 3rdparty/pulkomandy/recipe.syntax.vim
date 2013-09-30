" Vim syntax file
" Language: Haikuporter recipe files
" Maintainer: Adrien Destugues
" Latest Revision: 30 september 2013

if exists("b:current_syntax")
	finish
endif

syn keyword Keyword BUILD_PACKAGE_ACTIVATION_FILE DISABLE_SOURCE_PACKAGE HOMEPAGE
syn keyword Keyword MESSAGE REVISION CHECKSUM_MD5 PATCHES SOURCE_DIR SRC_FILENAME
syn keyword Keyword SRC_URI ARCHITECTURES BUILD_PREREQUIRES BUILD_REQUIRES CONFLICTS
syn keyword Keyword COPYRIGHT DESCRIPTION FRESHENS GLOBAL_WRITABLE_FILES LICENSE
syn keyword Keyword PACKAGE_GROUPS PACKAGE_USERS POST_INSTALL_SCRIPTS PROVIDES REPLACES
syn keyword Keyword REQUIRES SUMMARY SUPPLEMENTS USER_SETTING_FILES

syn keyword Function PATCH BUILD INSTALL TEST

syn region String start=/\v"/ skip=/\v\\./ end=/\v"/
