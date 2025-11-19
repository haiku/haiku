#!/bin/sh


tgzbase='fr-dvorak-bepo-keymaps-1.0rc2'
tgzname="$tgzbase.tgz"
tgzurl="http://download.tuxfamily.org/dvorak/keymaps/$tgzname"

#wget "$tgzurl"
tar zxvf "$tgzname"
./parse_linux_keymap.sh "$tgzbase"/fr-dvorak-bepo-utf8.map > bepo.keymap
keymap -o 'French (Bépo)' -c bepo.keymap
