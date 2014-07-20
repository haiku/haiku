#!/bin/bash

# for each package in /system/packages/ check which repository it comes from

# actually reverse video
#bold=`tput smso`
#offbold=`tput rmso`

# show only not found
onf=0
if [ "x$1" == "x-n" ]; then
onf=1
fi

cd /system/packages/

repos=""
for r in /system/settings/package-repositories/*; do
	repos="$repos ${r##*/}"
	u=`sed '/^url=/s/url=//g;q' "$r"`
	urls="$urls $u"
done
reponames=($repos)
repourls=($urls)

for p in *.hpkg; do
	#echo "$p"
	i=0
	found=0
	while [ $i -lt ${#reponames[@]} ]; do
		#echo "Checking repo ${reponames[$i]}..."
		#echo "${repourls[$i]}"
		if wget -q --spider "${repourls[$i]}/packages/$p" ; then
			[ "$onf" == 1 ] || echo "$p in ${reponames[$i]}";
			found=1
			break
		fi
		let i=i+1
	done
	if [ $found != 1 ]; then
		echo "${bold}$p NOT FOUND${offbold}"
	fi
done
