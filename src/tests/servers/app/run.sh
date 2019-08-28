#!/bin/sh

if [ ! -f test_app_server ]; then
	echo "You need to \"TARGET_PLATFORM=libbe_test jam -q install-test-apps\" first."
	echo "Afterwards, make sure you head to the folder with the test_app_server and run the script installed there."
	echo "If you need additional help, check in the \"NOTES\" file."
	exit
fi

# launch registrar
run_test_registrar || exit

# launch app_server
test_app_server &

if [ "$#" -eq 0 ]; then
	# no argument given, don't start any apps
	exit
elif [ "$1" = "-o" ]; then
	open .
	shift
fi

sleep 1

for i in $@; do
	$i &
done
