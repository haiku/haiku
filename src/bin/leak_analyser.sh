#!/bin/bash

if [ ! -f "$1" ]
then
	cat << EOF

	$(basename $0) <leaksFile> [<options>] [<excludePatterns>]

	<leaksFile>

		A file containing the allocations with stack traces from
		the guarded heap output.

		To generate such a file run a program with the following
		environment variables prefixed and pipe the output to a file:

		LD_PRELOAD=libroot_debug.so MALLOC_DEBUG=ges50 program > file

		The number after the "s" is the stack trace depth. Note that
		there is an implementation defined maximum.

	--no-default-excludes

		Do not exclude known statics and globals. By default a list of
		excludes is used that removes known allocations that are never
		freed by the system libraries.

	--no-exclude-empty

		Do not exclude allocations with no stack trace. By default
		allocations without a stack trace are excluded. This should
		only happen for very early allocations where the stack trace
		setting has not yet been applied. The program should not be
		able to generate such allocations.

	<excludePatterns>

		Exclude allocations that match a regular expression. The
		expression is matched against the whole text block of the
		allocation, so can match in the header line as well as any
		stack trace lines. Note that the whole block is on a single
		line and newlines have been replaced with the caret (^)
		character.

		Multiple exclude patterns can be specified as one argument each
		and they will be ored to form the final expression.

EOF

	exit 1
fi


FILENAME="$1"
shift


DONE_PARSING_OPTIONS=
NO_DEFAULTS=
NO_EXCLUDE_EMPTY=
while [ -z "$DONE_PARSING_OPTIONS" ]
do
	case "$1" in
		--no-default-excludes)
			NO_DEFAULTS=yes
			shift
		;;
		--no-exclude-empty)
			NO_EXCLUDE_EMPTY=yes
			shift
		;;
		--*)
			echo "unrecognized option \"$1\" ignored"
			shift
		;;
		*)
			DONE_PARSING_OPTIONS=yes
		;;
	esac
done


function append_pattern {
	if [ -z "$EXCLUDE_PATTERN" ]
	then
		EXCLUDE_PATTERN="$1"
	else
		EXCLUDE_PATTERN="$EXCLUDE_PATTERN|$1"
	fi
}


EXCLUDE_PATTERN=""
if [ -z "$NO_DEFAULTS" ]
then
	declare -a DEFAULT_EXCLUDE_LIST=( \
		"<libroot.so> initialize_before " \
		"<libroot.so> __cxa_atexit " \
		"<libroot.so> BPrivate::Libroot::LocaleBackend::LoadBackend" \
		"<libbe.so> initialize_before " \
		"<libbe.so> initialize_after " \
		"<libbe.so> _control_input_server_" \
		"<libbe.so> BApplication::_InitGUIContext" \
		"<libbe.so> BApplication::_InitAppResources" \
		"<libbe.so> BResources::LoadResource" \
		"<libbe.so> BClipboard::_DownloadFromSystem" \
		"<libbe.so> BToolTipManager::_InitSingleton" \
		"<libbe.so> BPrivate::WidthBuffer::StringWidth" \
		"<libtracker.so> _init " \
		"<libtranslation.so> BTranslatorRoster::Default" \
		"Translator> " \
		"<libicu[^.]+.so.[0-9]+> icu(_[0-9]+)?::" \
	)

	for EXCLUDE in "${DEFAULT_EXCLUDE_LIST[@]}"
	do
		append_pattern "$EXCLUDE"
	done
fi


if [ -z "$NO_EXCLUDE_EMPTY" ]
then
	append_pattern "^[^^]*\^$"
fi


while [ $# -gt 0 ]
do
	append_pattern "$1"
	shift
done


ALLOCATIONS=$(cat "$FILENAME" | egrep "^allocation: |^	" | tr '\n' '^' \
	| sed 's/\^a/~a/g' | tr '~' '\n' | sed 's/$/^/' | c++filt)

if [ ! -z "$EXCLUDE_PATTERN" ]
then
	ALLOCATIONS=$(echo "$ALLOCATIONS" | egrep -v "$EXCLUDE_PATTERN")
fi

if [ -z "$ALLOCATIONS" ]
then
	COUNT=0
else
	COUNT=$(echo "$ALLOCATIONS" | wc -l)
fi


echo "$ALLOCATIONS^total leaks: $COUNT" | tr '^' '\n' | less
