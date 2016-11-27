#!/bin/sh

which montage > /dev/null 2>&1 || pkgman install imagemagick

tmpf=/tmp/$$_icon_

for f in data/artwork/icons/*; do
  [ -d "$f" ] && continue
  bn="$(basename "$f")"
  translate "$f" "${tmpf}${bn}.png" 'PNG '
  echo "-label"
  echo "${bn}"
  echo "${tmpf}${bn}.png"
done | xargs -d '\n' sh -c 'montage -frame 5 -background "#336699" -geometry +4+4 -font /system/data/fonts/ttfonts/NotoSans-Regular.ttf -pointsize 9 "$@" haiku_icons.png' --

rm /tmp/$$_icon_*
