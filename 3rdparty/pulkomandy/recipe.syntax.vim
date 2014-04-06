" Vim syntax file
" Language: Haikuporter recipe files
" Maintainer: Adrien Destugues
" Latest Revision: 30 september 2013

if exists("b:current_syntax")
	finish
endif

syn keyword Keyword BUILD_PACKAGE_ACTIVATION_FILE DISABLE_SOURCE_PACKAGE HOMEPAGE
syn keyword Keyword MESSAGE REVISION CHECKSUM_MD5 PATCHES SOURCE_DIR SRC_FILENAME
syn keyword Keyword CHECKSUM_SIZE CHECKSUM_RMD160 CHECKSUM_SHA512 SECONDARY_ARCHITECTURES
syn keyword Keyword SRC_URI ARCHITECTURES BUILD_PREREQUIRES BUILD_REQUIRES CONFLICTS
syn keyword Keyword COPYRIGHT DESCRIPTION FRESHENS GLOBAL_WRITABLE_FILES LICENSE
syn keyword Keyword PACKAGE_GROUPS PACKAGE_USERS POST_INSTALL_SCRIPTS PROVIDES REPLACES
syn keyword Keyword REQUIRES SUMMARY SUPPLEMENTS USER_SETTING_FILES

syn keyword Function PATCH BUILD INSTALL TEST

syn keyword Define addOnsDir appsDir binDir buildArchitecture configureDirArgs
syn keyword Define dataDir dataRootDir debugInfoDir developDir developDocDir
syn keyword Define developLibDir docDir documentationDir fontsDir haikuVersion
syn keyword Define includeDir infodir installDestDir isCrossRepository jobArgs
syn keyword Define jobs libDir libExecDir localStateDir manDir oldIncludeDir
syn keyword Define portBaseDir portDir portFullVersion portName 
syn keyword Define portPackageLinksdir portRevision portRevisionedName
syn keyword Define portVersion portVersionedName postInstallDir preferencesDir
syn keyword Define prefix relativeAddOnsDir relativeAppsDir relativeBinDir
syn keyword Define relativeDataDir relativeDataRootDir relativeDebugInfoDir
syn keyword Define relativeDevelopDir relativeDevelopDocDir
syn keyword Define relativeDevelopLibDir relativeDocDir relativeDocumentationDir
syn keyword Define relativeFontsDir relativeIncludeDir relativeInfoDir
syn keyword Define relativeLibDir relativeLibExecDir relativeLocalStateDir
syn keyword Define relativeManDir relativeOldIncludeDir relativePostInstallDir
syn keyword Define relativePreferencesDir relativeSbinDir relativeSettingsDir
syn keyword Define relativeSharedStateDir sbinDir settingsDir sharedStateDir
syn keyword Define sourceDir targetArchitecture buildMachineTriple
syn keyword Define buildMachineTripleAsName crossSysrootDir targetMachineTriple
syn keyword Define targetMachineTripleAsName secondaryArchSuffix

syn keyword Function addAppDeskbarSymlink addPreferencesDeskbarSymlink
syn keyword Function defineDebugInfoPackage extractDebugInfo
syn keyword Function fixDevelopLibDirReferences fixPkgconfig packageEntries
syn keyword Function prepareInstalledDevelLib prepareInstalledDevelLibs
syn keyword Function runConfigure

syn keyword Type cmd devel lib app add_on

syn region String start=/\v"/ skip=/\v\\./ end=/\v"/ contains=Define,Type
syn match Comment '#.*$'
