#!/bin/sh

unescape_url () {
    echo -e "$(echo $*|sed 's/%/\\\x/g')"
}




dumpview () {

#    echo "$2= $1 of Window $WIN ="
#    echo "${2}APP=$APP"
#    echo "${2}WIN=$WIN"
#echo hey "$APP" count View of $1 Window $WIN

    local CNT="$(hey "$APP" count View of $1 Window $WIN | grep result | cut -d" " -f 7)"

    if [ -z "$CNT" ]; then
#	echo "NULL"
	return 0
    fi

#    echo "${2}CNT=$CNT="

    if [ "$CNT" -lt 1 ]; then
	return 0
    fi

    local C=0

    while [ $C -lt $CNT ]; do
	local INAME="$(hey "$APP" get InternalName of View $C of $1 Window $WIN | grep result | sed 's/.* : //')"
	echo "$2:: View $C/$CNT ($INAME) ( View $C of ${1}Window $WIN )"
#	hey "$APP" get View $C of $1 Window $WIN | awk " { print \"  $2\" \$0 } "
#	hey "$APP" getsuites of View $C of $1 Window $WIN | grep Label >/dev/null

#	if hey "$APP" getsuites of View $C of $1 Window $WIN | grep Label >/dev/null; then
#	    echo hey "$APP" getsuites of View $C of $1 Window $WIN | grep Label
#	    hey "$APP" get Label of View $C of $1 Window $WIN | grep result | awk " { print \"  $2\" \$0 } "
#	fi

#	if hey "$APP" getsuites of View $C of $1 Window $WIN | grep Text >/dev/null; then
#	    echo hey "$APP" getsuites of View $C of $1 Window $WIN | grep Text
#	    hey "$APP" get Text of View $C of $1 Window $WIN | grep result | awk " { print \"  $2\" \$0 } "
#	fi

	if hey "$APP" getsuites of View $C of $1 Window $WIN | grep Value >/dev/null; then
	    echo hey "$APP" getsuites of View $C of $1 Window $WIN | grep Value
	    hey "$APP" get Value of View $C of $1 Window $WIN | grep result | awk " { print \"  $2\" \$0 } "
	fi

#hey "$APP" getsuites of View $C of $1 Window $WIN | awk " { print \"  $2\" \$0 } "

	dumpview "View $C of $1" "  $2"
	let C="$C + 1"
    done

    echo "$2<"
}

if [ "$#" -lt 2 ]; then
echo "$0 App Window"
exit 1
fi


APP="$1"
WIN="$2"

dumpview 

