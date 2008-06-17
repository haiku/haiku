#!/bin/sh

quit application/x-vnd.Be-input_server

tagfile=/tmp/dokillinputserver
tmout=$((30))

touch $tagfile
(alert "All is fine" "Ok"; rm $tagfile) &

sleep $tmout

if [ -e $tagfile ]; then
	rm ~/config/add-ons/input_server/methods/Pen
	quit application/x-vnd.Be-input_server
	kill -9 input_server
	rm $tagfile
fi
