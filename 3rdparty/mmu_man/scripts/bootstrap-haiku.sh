#!/bin/sh

cd "$(dirname "$0")"

DESTVOL=/haiku
LOOPVOL=/loopimg

warn () {
	alert --warning "$*" "Ok"
}

error () {
	alert --stop "$*" "Ok"
	exit 1
}

log () {
	echo "$*"
}


if [ ! -d "$DESTVOL" ]; then
	error "$DESTVOL not mounted"
	exit 1
fi


find_current_revision () {
	log "Getting url of latest revision..."
	tf=/tmp/haiku-files.org_raw_$$
	wget -O $tf http://haiku-files.org/raw/ >/dev/null 2>&1 || error "wget error"
	url="$(grep -m1 'http:.*nightly-.*gcc2hybrid-raw.zip' "$tf" | sed 's/.*href="//;s/".*//')"
	test -n "$url" || error "cannot find latest build"
	file="${url##*/}"
	rm "$tf"
}


download_current_revision () {
	#if [ ! -e "$file" ]; then
	log "Downloading latest revision: $file"
	wget -c "$url" || error "cannot download latest revision"
	#fi
}


unzip_current_revision () {
	if [ ! -e "haiku-nightly.image" ]; then
		log "Unziping..."
		unzip "$file"
	fi
}


install_current_revision () {
	mkdir -p "$LOOPVOL"
	sync
	mount -ro "$PWD/haiku-nightly.image" "$LOOPVOL" || error "mount"
	sync
	
	copyattr -r -d "$LOOPVOL"/* "$DESTVOL/"
	sync
	unmount "$LOOPVOL"
}


makebootable_install () {
	log "Making the partition bootable..."
	makebootable "$DESTVOL/"
}


customize_install () {
	log "Copying files..."
	copyattr -r -d files/* "$DESTVOL/"
}



find_current_revision
download_current_revision
#file=haiku-nightly-r37641-x86gcc2hybrid-raw.zip
unzip_current_revision
install_current_revision
makebootable_install
customize_install
