#!/bin/sh


usage()
{
	cat << EOF
Usage: $0 <options>

Options - in order of intended workflow:

--help

--list			List the languages of the local set of catkeys.

--download		Download tarballs of the locally known languages.
				(The ones from --list. Ignoring all other.)

--download <lang>	Download tarball of language <lang>.

			E.g retries of 0k file.
			Or a language still unknown locally.

--unpack		Unpack all tarballs in a big pile.

--weed			Remove catkey files with less than two lines in them.

--copy			Copy catkeys from the scripts temp folder to trunk/data/calogs.

--test			Test building catalogs from catkeys files. (jam -q catalogs")

--stat			Show pre-commit subversion statistics.

--add			Add new catkey files to subversion source control. (svn add)


--diff			Show diffs for all languages. Paged. Divided by language.
--diff <lang>		Show diff for language <lang>. Paged.

--commit		Commit all languages, as separate commits.
			A standard message will be used for each.
			("Catalog update for language xx.")
			You will be asked for svn password once per language.
				
			CAVEAT! :: hta_committer currently fails to commit a langauge
			if one of the catkey files resides in a newly added folder,
			which is not part of the arguments. (Due to the split up of
			languages, by filename 'xx.catkeys'-part, in separate commits.)

(--commit <lang>)	Not implemented.

--delete		Delete tarballs and tempfolder.

--fix_fingerprints	Force expected fingerprints on catkeys that fail to build.

			Don't use this unless the fingerprints broke due to your
			editing of catalog entries, and you need the fingerprints
			to be updated. (It's a brute-force approach, including two
			full "jam clean". Caveat emptor.)


'********************************************'
'*    This script is probably not safe.     *'
'*                                          *'
'*        Use at your own peril!            *'
'********************************************'

EOF
}



subversionRootDirectory="$(pwd)"

tempDirName=hta_committer_tempfolder

tempDirectory="$subversionRootDirectory/$tempDirName"

listOfLanguagesFoundInSubversion="none found"



AssertCurrentDirIsSubversionTrunk()
{	
	for entry in data/catalogs src headers ; do
		if ! [ -e "${entry}" ] ; then
			echo "Need to be in SVN trunk."
			exit 1
		fi
	done
}


CreateTempDir()
{	
	mkdir -p "$tempDirectory"
}


PopulateLanguageList()
{
	listOfLanguagesFoundInSubversion="$( \
		find data/catalogs -name '*.catkeys' -exec basename '{}' ';' \
		| sort -u | awk '-F.' '{print$1}' | xargs echo)"
}


DownloadAllLanguageTarballs()
{
	echo "Downloading languages:" $listOfLanguagesFoundInSubversion
	for language in $listOfLanguagesFoundInSubversion ; do
		DownloadLanguageTarball "$language"
	done
}


DownloadLanguageTarball()
{
	LANGUAGE=$1
	# The package for zh_hans catalogs is actually called zh-hans
	if [ "$LANGUAGE" = "zh_hans" ] ; then
		LANGUAGE="zh-hans"
	fi
	
	cd "$tempDirectory"
	echo "Downloading language:  $LANGUAGE"

	if [ -e "$LANGUAGE.tgz" ] ; then
		rm "$LANGUAGE.tgz"
	fi	
	
	attempt=0
	local tarballURL="http://hta.polytect.org/catalogs/tarball"
	
	while [ $(wget -nv -U hta_committer -O "$LANGUAGE.tgz" "$tarballURL/$LANGUAGE" ; echo $?) -ne 0 ]; do
		if [ $attempt -eq 5 ]; then
			break
		fi
		(( attempt++ ))
		echo "Download attempt #$attempt failed. Retrying ..."
		if [ -e "$LANGUAGE" ]; then
			rm "$LANGUAGE"
		fi
	done
	if [ $attempt -ge 5 ]; then
		if [ -e "$LANGUAGE" ]; then
			rm "$LANGUAGE"
		fi
		echo "WARNING: Max download retries exceeded for language $LANGUAGE."
	fi
		
	cd "$subversionRootDirectory"
}


PopulateTarballList()
{
	cd "$tempDirectory"
	TARBALL_LIST=$(ls | grep .tgz | sort)
	echo "$TARBALL_LIST"
	cd "$subversionRootDirectory"
}


UnpackAllTarballs()
{
	cd "$tempDirectory"
	echo "Unpacking tarballs:" $TARBALL_LIST
	for tarball in $TARBALL_LIST ; do
		UnpackTarball "$tarball"
	done
	cd "$subversionRootDirectory"
}


UnpackTarball()
{
	cd "$tempDirectory"
	echo "Unpacking tarball:" $1
	tar -zxf "$1"
#	mv data "$(echo $1 | sed 's/.tgz//g')"	#in separate folders
	cd "$subversionRootDirectory"
}


DeleteEmptyCatkeyFiles()
{
	numberOfEmptyFiles=0
	
	unpackedCatkeyFiles=$(find "$tempDirectory" -name '*.catkeys' -exec echo '{}' ';')
	for file in $unpackedCatkeyFiles ; do
		LINES_IN_FILE=$(cat $file | wc | awk '{print$1}')
		if [ $LINES_IN_FILE -lt 2 ] ; then
			#echo Deleting empty file "$file"
			numberOfEmptyFiles=$(($numberOfEmptyFiles + 1))
			rm $file
		fi
	done
	echo Deleted "$numberOfEmptyFiles" empty files
}


CopyCatkeyFilesInPlace()
{
	cp -r "${tempDirectory}/data/catalogs/." "${subversionRootDirectory}/data/catalogs/"
}


TestBuildingCatalogsFromCatkeyFiles()
{
	cd "$subversionRootDirectory"
	jam catalogs
	jam catalogs
}


ShowSubversionStatistics()
{
	cd "$subversionRootDirectory"

	echo
	echo Languages: "$listOfLanguagesToCommit"
	echo
	echo Total: "$(svn stat data/catalogs | grep catkeys | wc | awk '{print$1}')"
	echo Modified: "$(svn stat data/catalogs | grep catkeys | grep ^M | wc | awk '{print$1}')"
	echo Added: "$(svn stat data/catalogs | grep catkeys | grep ^A | wc | awk '{print$1}')"
	echo
	echo Other:
	echo "$(svn stat data/catalogs | grep ^?)"
}


AddUnversionedToSubversion()
{
	cd "$subversionRootDirectory"
	svn stat data/catalogs | grep ^? | awk '{print$2}' | xargs svn add 
}


PopulateCommitLanguageList()
{
	listOfLanguagesToCommit="$(svn stat data/catalogs | grep catkeys | awk '{print$2}' \
		| awk -F / '{print$NF}' | awk '-F.' '{print$1}' | sort -u | xargs echo)"
}


DiffAllLanguages()
{
	echo Diffing languages: "$listOfLanguagesToCommit"

	for language in $listOfLanguagesToCommit ; do
		DiffLanguage "$language"
	done
}


DiffLanguage()
{
	echo Diffing language "$1"
	svn stat data/catalogs/ | grep [^A\|^M]	| grep "$1.catkeys" \
		| awk '{print$2}' | xargs svn diff | less
}


CommitAllLanguages()
{
	echo Committing languages: "$listOfLanguagesToCommit"
	
	for language in $listOfLanguagesToCommit ; do
		CommitLanguage "$language"
	done
}


CommitLanguage()
{
	echo Committing language "$1"
	svn stat data/catalogs/ | grep [^A\|^M]	| grep "$1.catkeys" \
		| awk '{print$2}' | xargs svn commit -m "Catalog update for language $1."

	echo
}


DeleteTarballs()
{
	find "$subversionRootDirectory/$tempDirName" -name '*.tgz' -exec rm '{}' ';'
}


DeleteTempDirectory()
{
	cd "$subversionRootDirectory"
	rm -rf "$tempDirectory"
}


RemoveTempDirectoryIfEmpty()
{
	rmdir --ignore-fail-on-non-empty hta_committer_tempfolder/
}


FixFingerprintsOfFailingCatalogs()
{
	# Find the catalogs that won't build.
	bad_catalogs=$( \
		echo $( \
			jam catalogs 2>&1 \
			| awk '-Fsource-catalog ' '{print$2}' \
			| awk '-F - error' '{print$1}' \
			| sort -u));

	# Find the present and the expected fingerprints.
	# Produce a script to replace present fingerprints with the expected ones.
	jam catalogs 2>&1  \
		| grep 'instead of' \
		| awk '-Fafter load ' '{print$2}' \
		| awk '-F. The catalog data' '{print$1}' \
		| sed 's/ instead of /\t/g' | sed 's/[()]//g' \
		| awk '{print "sed -i s/" $2 "/" $1 "/g"}' \
		| awk "{ print \$0\" $bad_catalogs\" }" \
		> hta_fingerprint_correction_script.sh;

	chmod u+x hta_fingerprint_correction_script.sh;
	$(hta_fingerprint_correction_script.sh);
	rm hta_fingerprint_correction_script.sh
}


if [ $# -eq 1 ] ; then
	case "$1" in
		--help)
			usage
			exit
			;;
		--list)
			AssertCurrentDirIsSubversionTrunk
			PopulateLanguageList
			echo $listOfLanguagesFoundInSubversion
			exit
			;;
		--download)
			AssertCurrentDirIsSubversionTrunk
			CreateTempDir
			PopulateLanguageList
			DownloadAllLanguageTarballs
			exit
			;;
		--unpack)
			AssertCurrentDirIsSubversionTrunk
			CreateTempDir
			PopulateTarballList
			UnpackAllTarballs
			exit
			;;
		--weed)
			AssertCurrentDirIsSubversionTrunk
			CreateTempDir
			DeleteEmptyCatkeyFiles
			exit
			;;
		--delete)
			AssertCurrentDirIsSubversionTrunk
			CreateTempDir
			DeleteTarballs
			DeleteTempDirectory
			exit
			;;
		--copy)
			AssertCurrentDirIsSubversionTrunk
			CopyCatkeyFilesInPlace
			exit
			;;
		--test)
			AssertCurrentDirIsSubversionTrunk
			TestBuildingCatalogsFromCatkeyFiles
			exit
			;;
		--stat)
			AssertCurrentDirIsSubversionTrunk
			PopulateCommitLanguageList
			ShowSubversionStatistics
			exit
			;;
		--add)
			AssertCurrentDirIsSubversionTrunk
			AddUnversionedToSubversion
			exit
			;;
		--diff)
			AssertCurrentDirIsSubversionTrunk
			PopulateCommitLanguageList
			DiffAllLanguages
			exit
			;;
		--commit)
			AssertCurrentDirIsSubversionTrunk
			PopulateCommitLanguageList
			CommitAllLanguages
			exit
			;;
		--fix_fingerprints)
			AssertCurrentDirIsSubversionTrunk
			FixFingerprintsOfFailingCatalogs
			exit
			;;
		*)
			usage
			exit 1
			;;
	esac
fi


if [ $# -eq 2 ] ; then
	case "$1" in
		--help)
			usage
			exit
			;;
		--download)
			AssertCurrentDirIsSubversionTrunk
			CreateTempDir
			DownloadLanguageTarball "$2"
			exit;
			;;
		--diff)
			AssertCurrentDirIsSubversionTrunk
			DiffLanguage "$2"
			exit
			;;
		*)
			usage
			exit 1
			;;
	esac
fi


usage
exit 1

