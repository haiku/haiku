#!/bin/sh
#
# Copyright (c) 2010-2012 Haiku, Inc.
# Distributed under the terms of the MIT License.
#
# Authors:
#		Matt Madia, mattmadia@gmail.com
#
# Synopsis:
#	Provide a mechanism for end-users to install various firmwares for wireless
#	network cards in a manner that complies with their individual licenses.
#
# Supported chipsets:
# 	Intel ipw2100
#	Intel ipw2200/2225/2915
#	Broadcom 43xx
#	Marvell 88W8335


MESSAGE="This script will install firmware for various wireless network cards.
 The Broadcom 43xx and Marvell 88W8335 require an active network connection
 to download additional files before installation. In the absence of internet
 access, only Intel's ipw2100 and ipw2200 will be installed.

 If you do not have internet access and need to install the other firmwares,
 goto https://www.haiku-os.org/docs/userguide/en/workshop-wlan.html. This page
 has instructions on which files to manually download and where to copy them
 into this OS. It also has different script that can be run on another OS and
 will prepare a zip archive for easy install. After that, re-run this script."
VIEW='View licenses'
ABORT='Abort installation'
OK='I agree to the licenses. Install firmwares.'

baseURL='https://raw.githubusercontent.com/haiku/firmware/master/wifi/'
firmwareDir=`finddir B_SYSTEM_DATA_DIRECTORY`/firmware
tempDir=`finddir B_SYSTEM_TEMP_DIRECTORY`/wifi-firmwares
driversDir=`finddir B_SYSTEM_ADDONS_DIRECTORY`/kernel/drivers
tempFirmwareDir=`finddir B_SYSTEM_TEMP_DIRECTORY`/package_me"$firmwareDir"
intelLicense='/boot/system/data/licenses/Intel (2xxx firmware)'


function DisplayAlert()
{
	local result=`alert --stop "$MESSAGE" "$VIEW" "$ABORT" "$OK"`
	case "${result}" in
		"$VIEW")
			ViewLicenses ;
			DisplayAlert ;
			;;
		"$ABORT")
			exit 0 ;;
		"$OK")
			InstallAllFirmwares ;
			exit 0 ;
			;;
	esac
}


function ViewLicenses()
{
	license="$tempDir/Wifi_Firmware_Licenses"
	cat << EOF > $license

+-----------------------------------------------------------------------------+
|                                                                             |
|   Copyright and licensing information of the various wireless firmwares     |
|                                                                             |
| Firmware for broadcom43xx is under the Copyright of Broadcom Corporation(R) |
| Firmware for marvell88w8335 is under the Copyright of Marvell Technology(R) |
| ipw2100,iprowifi2200 firmware is covered by the following Intel(R) license: |
|                                                                             |
+-----------------------------------------------------------------------------+

EOF
	cat "$intelLicense" >> $license

	open $license
}


function InstallAllFirmwares()
{
	InstallIpw2100
	InstallIprowifi2200
	InstallBroadcom43xx
	InstallMarvell88w8335
	MakeHPKG
}


function DownloadFileIfNotCached()
{
	# DownloadFileIfNotCached <url> <filename> <destination dir>
	local url=$1
	local file=$2
	local dir=$3

	mkdir -p "$dir"
	if [ ! -e $dir/$file ] ; then
		echo "Downloading $url ..."
		wget -nv -O $dir/$file $url
	fi
	result=$?
	if [ $result -gt 0 ]; then
		local error="Failed to download $url."
		local msg="As a result, ${driver}'s firmware will not be installed."
		alert --warning "$error $msg"
	fi
}


function SetFirmwarePermissions()
{
	cd ${tempFirmwareDir}/${driver}/
	for file in * ; do
		if [ "$file" != "$driver" ] && [ -f "$file" ] ; then
			chmod a=r $file
		fi
	done
}


function CleanTemporaryFiles()
{
	rm -rf "$tempDir"
	mkdir -p "$tempDir"
}


function PreFirmwareInstallation()
{
	echo "Acquiring firmware for ${driver} ..."
	mkdir -p "${tempFirmwareDir}/${driver}"
}


function PostFirmwareInstallation()
{
	SetFirmwarePermissions
	CleanTemporaryFiles
	echo "... firmware for ${driver} will be installed."
}


function InstallIpw2100()
{
	driver='iprowifi2100'
	PreFirmwareInstallation

	# Prepare firmware archive for extraction.
	local file='ipw2100-fw-1.3.tgz'
	local url="${baseURL}/intel/${file}"
	local dir="${tempFirmwareDir}/${driver}"
	cp "${firmwareDir}/${driver}/${file}" "${dir}"
	DownloadFileIfNotCached $url $file $dir

	# Extract the firmware & license file in place.
	cd "${tempFirmwareDir}/${driver}"
	gunzip < "$file" | tar xf -

	rm "${tempFirmwareDir}/${driver}/${file}"
	PostFirmwareInstallation
}


function InstallIprowifi2200()
{
	driver='iprowifi2200'
	PreFirmwareInstallation

	# Prepare firmware archive for extraction.
	local file='ipw2200-fw-3.1.tgz'
	local url="${baseURL}/intel/${file}"
	local dir="${tempFirmwareDir}/${driver}"
	cp "${firmwareDir}/${driver}/${file}" "${dir}"
	DownloadFileIfNotCached $url $file $dir

	# Extract the firmware & license file.
	cd "$tempDir"
	gunzip < "${tempFirmwareDir}/${driver}/$file" | tar xf -
	cd "${tempDir}/ipw2200-fw-3.1"
	mv LICENSE.ipw2200-fw "${tempFirmwareDir}/${driver}/"
	mv ipw2200-ibss.fw "${tempFirmwareDir}/${driver}/"
	mv ipw2200-sniffer.fw "${tempFirmwareDir}/${driver}/"
	mv ipw2200-bss.fw "${tempFirmwareDir}/${driver}/"

	rm "${tempFirmwareDir}/${driver}/${file}"
	PostFirmwareInstallation
}


function InstallBroadcom43xx()
{
	driver='broadcom43xx'
	PreFirmwareInstallation

	pkgman install -y cmd:b43_fwcutter
	returnCode=$?
	if [ $returnCode -gt 0 ] ; then
		echo "...failed. ${driver}'s firmware will not be installed."
		return $returnCode
	fi

	CutAndInstallBroadcomFirmware
	returnCode=$?
	if [ $returnCode -gt 0 ] ; then
		echo "...failed. ${driver}'s firmware will not be installed."
		return $returnCode
	fi

	PostFirmwareInstallation
}


function InstallMarvell88w8335()
{
	driver='marvell88w8335'
	PreFirmwareInstallation

	# Download firmware archive.
	local file="malo-firmware-1.4.tgz"
	local url="${baseURL}/marvell/${file}"
	local dir="${tempFirmwareDir}/${driver}"
	DownloadFileIfNotCached $url $file "$dir"
	if [ $result -gt 0 ]; then
		echo "...failed. ${driver}'s firmware will not be installed."
		return $result
	fi

	# Extract archive.
	cd "$tempDir"
	tar xf "${tempFirmwareDir}/${driver}/$file"

	# Move firmware files to destination.
	local sourceDir="${tempDir}/share/examples/malo-firmware"
	mv ${sourceDir}/malo8335-h "${tempFirmwareDir}/${driver}"
	mv ${sourceDir}/malo8335-m "${tempFirmwareDir}/${driver}"

	rm "${tempFirmwareDir}/${driver}/${file}"
	PostFirmwareInstallation
}


function CutAndInstallBroadcomFirmware()
{
	# Download firmware.
	local file="wl_apsta-3.130.20.0.o"
	local dir="${tempFirmwareDir}/${driver}"
	local url="${baseURL}/b43/${file}"
	DownloadFileIfNotCached $url $file $dir
	if [ $result -gt 0 ]; then
		return $result
	fi

	# Cut firmware in pieces.
	cp "$dir/$file" "$tempDir"
	cd "$tempDir"
	b43-fwcutter $file > /dev/null 2>&1

	# Rename the pieces.
	cd b43legacy
	for i in $(ls -1); do
		newFileName=$(echo $i | sed "s/\(.*\)\.fw$/bwi_v3_\1/g")
		mv $i $newFileName
	done
	touch bwi_v3_ucode

	# Install files.
	mv * ${tempFirmwareDir}/${driver}/

	rm 	"${tempFirmwareDir}/${driver}/$file"
	return 0
}


function makePackageInfo()
{
	cat << EOF > .PackageInfo
name			wifi_firmwares
version			2013_10_06-1
architecture	any
summary			"Firmwares for various wireless network cards"
description		"Installs firmwares for the following wireless network cards:
Broadcom 43xx, Intel ipw2100, Intel ipw2200, and Marvell 88W8335.
Firmware for broadcom43xx is under the Copyright of Broadcom Corporation(R).
Firmware for marvell88w8335 is under the Copyright of Marvell Technology(R).
ipw2100,iprowifi2200 firmware is covered by the Intel(R) license located in
/boot/system/data/licenses/Intel (2xxx firmware). The user is not granted a
license to use the package unless these terms are agreed to."

packager	"me"
vendor		"myself"
copyrights {
	"Copyright of Broadcom Corporation(R)"
	"Copyright of Intel(R) Corporation"
	"Copyright of Marvell Technology(R)"
}
licenses	"Intel (2xxx firmware)"
flags {
	"approve_license"
	"system_package"
}
provides {
	wifi_firmwares = 2013_10_06
}
requires {
}

EOF

}


function MakeHPKG()
{
	cd "$tempFirmwareDir/../../.."
	makePackageInfo
	package create -C system -i .PackageInfo wifi_firmwares-1-any.hpkg
	mv wifi_firmwares-1-any.hpkg `finddir B_SYSTEM_PACKAGES_DIRECTORY`
	rm -rf "`finddir B_SYSTEM_TEMP_DIRECTORY`/package_me"
	rm -rf "$tempDir"
}


mkdir -p "$tempDir"
mkdir -p "$tempFirmwareDir"
DisplayAlert
