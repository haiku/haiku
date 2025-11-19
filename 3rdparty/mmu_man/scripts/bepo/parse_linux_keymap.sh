#!/bin/sh

# path to a keysymdef.h
KEYSYM_PATHS=(/usr/include/X11/ /zeta/apps/X11R6.4/include/X11/ ./)
KEYSYMDEF=keysymdef.h

# maps Linux (X11) keycodes to BeOS keycodes
L2B=(
	0x00	0x00	0x12	0x13	0x14	0x15	0x16	0x17
	0x18	0x19	0x1a	0x1b	0x1c	0x1d	0x00	0x00
	#16
	0x27	0x28	0x29	0x2a	0x2b	0x2c	0x2d	0x2e
	0x2f	0x30	0x31	0x32	0x00	0x00	0x3c	0x3d
	#32
	0x3e	0x3f	0x40	0x41	0x42	0x43	0x44	0x45
	0x46	0x11	0x00	0x33	0x4c	0x4d	0x4e	0x4f
	#48
	0x50	0x51	0x52	0x53	0x54	0x55	0x00	0x00
	0x00	0x5e	0x00	0x00	0x00	0x00	0x00	0x00
	#64
	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
	#72
	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
	#80
	0x00	0x00	0x00	0x00	0x00	0x00	0x69	0x00
	#88
	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
	#96
	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
	0x00	0x00	0x00	0x00	0x00	0x00	0x00	0x00
	#128
)

stop () {
	echo "$@"
	exit 1
}

warn () {
	echo "warning: " "$@" >&2
}

remap_key_l2b () {
	echo "$((${L2B[$1]}))"
}

unicode_to_utf8 () {
	uni="${1}"
	if [ "${uni:0:2}" = "U+" ]; then
		uni="${uni:2}"
	fi
	if [ "${uni:0:2}" = "0x" ]; then
		uni="${uni:2}"
	fi
	if [ "${#uni}" = "3" ]; then
		uni="0${uni}"
	fi
	case "$uni" in
	[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F])
		#utf8="$(echo -en "\x${uni:0:2}\x${uni:2}" | iconv -f UCS-2BE -t UTF-8 | od -A n -t x1 | sed 's/ /\\\x/g')"
		utf8="$(echo -en "\x${uni:0:2}\x${uni:2}" | iconv -f UCS-2BE -t UTF-8 | od -A n -t x1 | sed 's/ //g')"
		#utf8="$uni"
		if [ -z "$uni" ]; then
			warn "empty utf-8 for '$uni'"
		fi
		;;
	*)
		warn "unhandled unicode '$uni'"
		utf8="00"
		;;
	esac
	echo "0x$utf8"
}

broken_unicode_to_utf8 () {
	uni="${1}"
	if [ "${lk:0:2}" = "U+" ]; then
		uni="${lk:2}"
	fi
	case "$uni" in
	[0-7][0-9a-f])
		utf8="$uni"
		;;
	00[0-7][0-9a-f])
		utf8="${uni:2:2}"
		;;
#	01[0-7][0-9a-f])
#		utf8="c5$(printf '' $((0x80${uni:2:2})))"
#		;;
	20[8-9a-f][0-9a-f])
		utf8="e282${uni:2:2}"
		;;
	*)
		warn "unhandled unicode '$uni'"
		utf8="00"
		;;
	esac
	echo "0x$utf8"
}

convert_keyval () {
	lk="$1"
	if [ "${lk:0:1}" = "+" ]; then
		warn "unhandled +"
		lk="${lk:1}"
	fi
	warn "lk: $lk"
	case "$lk" in
	[0-9a-zA-Z])
		echo "'$lk'"
		;;
	U+*)
		echo "$(unicode_to_utf8 $lk)"
		;;
	space)
		echo "' '"
		;;
	underscore)
		echo "'_'"
		;;
	'')
		echo "''"
		;;
	*)
		keysymhex="$(grep "XK_$lk[	 ]" "$keysymdef" | awk '{print $3}')"
		warn "found: '$keysymhex'"
		if [ -n "$keysymhex" ]; then
			echo "$(unicode_to_utf8 $keysymhex)"
			return
		fi
		case "$lk" in
		one)
			echo "'1'";;
		two)
			echo "'2'";;
		three)
			echo "'3'";;
		four)
			echo "'4'";;
		five)
			echo "'5'";;
		six)
			echo "'6'";;
		seven)
			echo "'7'";;
		eight)
			echo "'8'";;
		nine)
			echo "'9'";;
		zero)
			echo "'0'";;
		*)
			warn "unknown: '$lk'"
			;;
		esac
		echo "''"
		;;
	esac
}

parse_lkeymap () {
	while read line; do
		if [ "${line:0:1}" = "#" ]; then
			continue
		fi
		keyspec="${line%=*}"
		keyvals="${line#*=}"
		if [ "${keyspec}" = "${keyvals}" ]; then
			#warn "skipping $line"
			continue
		fi
		mods=0
		for k in $keyspec; do
			#echo -n "-$k-"
			case "$k" in
				Shift|shift)
					mods=$((mods | 0x1))
					;;
				Control|control)
					mods=$((mods | 0x4))
					;;
				Alt|alt)
					mods=$((mods | 0x2))
					#warn "unhandled modifier"
					;;
				AltGr|altgr)
					mods=$((mods | 0x40))
					;;
				plain)
					mods=0
					continue;;
				keycode)
					continue;;
				*)
					case "${k:0:1}" in
					[0-9]*)
						keycode="${k/[^0-9]}"
						;;
					*)
						stop "unknown: $k"
						;;
					esac
					;;
			esac
		done
		if [ $((mods & 0x2)) -gt 0 ]; then
			#echo "skipping Alt line: $line"
			continue
		fi
		hkeycode="$(remap_key_l2b "$keycode")"
		#echo "$hkeycode"
		# handle values
		kv=($keyvals)
		hkv=('' '' '' '')
		if [ -z "${kv[1]}" ]; then
			case "${kv[0]}" in 
			[a-z])
				#echo "plop-${kv[0]}-"
				kv[1]="$(echo "${kv[0]}" | tr '[a-z]' '[A-Z]')"
				#echo "-${#kv[@]}-"
				;;
			*)
				;;
			esac
		fi
		for n in $(seq 0 3); do
			#if [ -z "${kv[$n]}" ]; then
			#	continue;
			#fi
			hkv[$n]="$(convert_keyval "${kv[$n]}")"
			if [ "${hkv[$n]}" = "" ]; then
				hkv[$n]="''"
			fi
		done
		#echo "-$mods-"
		case "$mods" in
			0)
				eval "hkmap_${hkeycode}[0]=\"${hkv[0]}\""	# n
				eval "hkmap_${hkeycode}[1]=\"${hkv[1]}\""	# s
				#eval "hkmap_${hkeycode}[2]=\"${hkv[2]}\""	# c
				eval "hkmap_${hkeycode}[3]=\"${hkv[2]}\""	# o
				eval "hkmap_${hkeycode}[4]=\"${hkv[3]}\""	# os
				eval "hkmap_${hkeycode}[5]=\"${hkv[1]}\""	# C
				eval "hkmap_${hkeycode}[6]=\"${hkv[0]}\""	# Cs
				eval "hkmap_${hkeycode}[7]=\"${hkv[2]}\""	# Co
				eval "hkmap_${hkeycode}[8]=\"${hkv[3]}\""	# Cos
				;;
			64)	#Option(AltGr)
				eval "hkmap_${hkeycode}[3]=\"${hkv[0]}\""
				;;
			65)	#Shift+Option(AltGr)
				eval "hkmap_${hkeycode}[4]=\"${hkv[0]}\""
				;;
			*)
				warn "unhandled modifiers $mods"
				;;
		esac
		#for i in $(seq 0 3); do
		#	if [ -z "${kv[$i]}" ]; then
		#		break
		#	fi
		#	
		#	#echo "[$i]='${kv[$i]}'"
		#done
		#echo ":$keycode:"
		#echo "$keyspec===$keyvals"
	done
	gen_hkeymap
}

prefill_hkeymap () {
	for n in $(seq 0 127); do
		val="''"
		val2=""
		case $n in
			1)
				val="0x1b";;
			[2-9]|1[0-6])
				val="0x10";;
			$((0x1e)))
				val="0x08";;
			$((0x1f)))
				val="0x05";;
			$((0x20)))
				val="0x01";;
			$((0x21)))
				val="0x0b";;
			$((0x23)))
				val="'/'";;
			$((0x24)))
				val="'*'";;
			$((0x25)))
				val="'-'";;
			$((0x26)))
				val="0x09";;
			$((0x34)))
				val="0x7f";;
			$((0x35)))
				val="0x04";;
			$((0x36)))
				val="0x0c";;
			$((0x37)))
				val="0x01";val2="'7'";;
			$((0x38)))
				val="0x1e";val2="'8'";;
			$((0x39)))
				val="0x0b";val2="'9'";;
			$((0x3a)))
				val="'+'";;
			$((0x47)))
				val="0x0a";;
			$((0x48)))
				val="0x1c";val2="'4'";;
			$((0x49)))
				val="''";val2="'5'";;
			$((0x4a)))
				val="0x1d";val2="'6'";;
			$((0x57)))
				val="0x1e";;
			$((0x58)))
				val="0x01";val2="'1'";;
			$((0x59)))
				val="0x1e";val2="'2'";;
			$((0x5a)))
				val="0x0b";val2="'3'";;
			$((0x5b)))
				val="0x0a";;
			$((0x61)))
				val="0x1c";;
			$((0x62)))
				val="0x1f";;
			$((0x63)))
				val="0x1d";;
			$((0x64)))
				val="0x05";val2="'0'";;
			$((0x65)))
				val="0x7f";val2="'.'";;
#			$((0x66)))
#				val="''";;
			*)
				;;
		esac
		if [ -z "$val2" ]; then
			val2="$val"
		fi
		eval "hkmap_$n=(\"$val\" \"$val2\" \"$val\" \"$val\" \"$val2\" \"$val\" \"$val2\" \"$val\" \"$val2\")"
		
		#eval v="(\${hkmap_$n[*]})"
		#echo "${v[7]}"
	done
}


gen_hkeymap () {
	echo "#!/bin/keymap -l
Version = 3
CapsLock = 0x3b
ScrollLock = 0x0f
NumLock = 0x22
LShift = 0x4b
RShift = 0x56
LCommand = 0x5d
RCommand = 0x00
LControl = 0x5c
RControl = 0x60
LOption = 0x66
ROption = 0x5f
Menu = 0x68
#
# Lock settings
# To set NumLock, do the following:
#   LockSettings = NumLock
#
# To set everything, do the following:
#   LockSettings = CapsLock NumLock ScrollLock
#
LockSettings = 

# Legend:
#   n = Normal
#   s = Shift
#   c = Control
#   C = CapsLock
#   o = Option
# Key      n        s        c        o        os       C        Cs       Co       Cos     
"

	for n in $(seq 0 127); do
		printf "Key 0x%02x = " $n
		eval v="(\${hkmap_$n[*]})"
		for n in $(seq 0 8); do
			printf "%-10s" "${v[$n]}"
		done
		echo ""
	done

echo "
Acute ' '       = 0xc2b4   
Acute 'A'       = 0xc381   
Acute 'E'       = 0xc389   
Acute 'I'       = 0xc38d   
Acute 'O'       = 0xc393   
Acute 'U'       = 0xc39a   
Acute 'Y'       = 0xc39d   
Acute 'a'       = 0xc3a1   
Acute 'e'       = 0xc3a9   
Acute 'i'       = 0xc3ad   
Acute 'o'       = 0xc3b3   
Acute 'u'       = 0xc3ba   
Acute 'y'       = 0xc3bd   
AcuteTab = Option Option-Shift CapsLock-Option CapsLock-Option-Shift 
Grave ' '       = '\`'      
Grave 'A'       = 0xc380   
Grave 'E'       = 0xc388   
Grave 'I'       = 0xc38c   
Grave 'O'       = 0xc392   
Grave 'U'       = 0xc399   
Grave 'a'       = 0xc3a0   
Grave 'e'       = 0xc3a8   
Grave 'i'       = 0xc3ac   
Grave 'o'       = 0xc3b2   
Grave 'u'       = 0xc3b9   
GraveTab = Option Option-Shift CapsLock-Option CapsLock-Option-Shift 
Circumflex ' '       = '^'      
Circumflex 'A'       = 0xc382   
Circumflex 'E'       = 0xc38a   
Circumflex 'I'       = 0xc38e   
Circumflex 'O'       = 0xc394   
Circumflex 'U'       = 0xc39b   
Circumflex 'a'       = 0xc3a2   
Circumflex 'e'       = 0xc3aa   
Circumflex 'i'       = 0xc3ae   
Circumflex 'o'       = 0xc3b4   
Circumflex 'u'       = 0xc3bb   
CircumflexTab = Normal Shift CapsLock CapsLock-Shift 
Diaeresis ' '       = 0xc2a8   
Diaeresis 'A'       = 0xc384   
Diaeresis 'E'       = 0xc38b   
Diaeresis 'I'       = 0xc38f   
Diaeresis 'O'       = 0xc396   
Diaeresis 'U'       = 0xc39c   
Diaeresis 'Y'       = 0xc5b8   
Diaeresis 'a'       = 0xc3a4   
Diaeresis 'e'       = 0xc3ab   
Diaeresis 'i'       = 0xc3af   
Diaeresis 'o'       = 0xc3b6   
Diaeresis 'u'       = 0xc3bc   
Diaeresis 'y'       = 0xc3bf   
DiaeresisTab = Normal Shift CapsLock CapsLock-Shift 
Tilde ' '       = '~'      
Tilde 'A'       = 0xc383   
Tilde 'O'       = 0xc395   
Tilde 'N'       = 0xc391   
Tilde 'a'       = 0xc3a3   
Tilde 'o'       = 0xc3b5   
Tilde 'n'       = 0xc3b1   
TildeTab = Option Option-Shift CapsLock-Option CapsLock-Option-Shift 
"

}

# find keysymdef.h
for d in "${KEYSYM_PATHS[@]}"; do
	if [ -e "$d$KEYSYMDEF" ]; then
		keysymdef="$d$KEYSYMDEF"
		break;
	fi
done

if [ -z "$keysymdef" ]; then
	stop "Cannot find a keysymdef.h"
fi


prefill_hkeymap

parse_lkeymap < "$1"
