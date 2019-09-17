#!/bin/sh

# get running app signatures and filter out background apps
roster | sed -n 's/)$//;s/^ *//;/[^64] (/s/ .*(/\t/p' | while read team app; do
	echo "# team $team $app"
	c="$(hey "$team" count Window | grep result | cut -d" " -f 7)"
	let c="$c - 1"
	for w in $(seq 0 "$c"); do
		title="$(hey "$team" get Title of Window "$w" | sed -n 's/"$//;/"result"/s/.*TYPE) : "//p')"
		frame="$(hey "$team" get Frame of Window "$w" | sed -n 's/)$//;/"result"/s/.*TYPE) : BRect(//p')"
		echo "hey \"$app\" set Frame of Window \"$title\" to \"BRect($frame)\""
	done
done
