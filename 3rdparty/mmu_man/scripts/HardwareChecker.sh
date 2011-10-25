#!/bin/sh
#	HardwareChecker.sh for Haiku
#
#	Copyright 2011, François Revol <revol@free.fr>.
#
#	Distributed under the MIT License
#
#	Created: 2011-10-25
#


netcat=netcat
report_site=haikuware.con
report_cgi=http://haikuware.com/hwreport.php

do_notify ()
{
	p="$1"
	m="$2"
	shift
	shift
	notify --type progress \
		--messageID hwck_$$ \
		--icon /system/apps/Devices \
		--app HardwareChecker \
		--title "progress:" --progress "$p" "$m" "$@"
	
	
}

start_fake_httpd ()
{
	report_port=8989
	report_file="$(finddir B_DESKTOP_DIRECTORY)/hwchecker_report_$$.txt"
	report_ack="<html><head></head><body><h1>Done! You can close this window now.</h1></body></html>"
	report_cgi=http://127.0.0.1:$report_port/hwreport
	(
		# force a previous isntance to close
		$netcat 127.0.0.1 8989 < /dev/null > /dev/null
		echo "listening on port $report_port"
		#
		(echo -e "HTTP/1.1 100 Continue\r\n\r\n"; echo -e "HTTP/1.1 200 OK\r\nDate: $(date)\r\nContent-Type: text/html\r\nContent-Length: ${#report_ack}\r\n\r\n$report_ack") | $netcat -q 1 -l -p $report_port > "$report_file"
		
		# make sure we have something
		if [ -s "$report_file" ]; then
			open "$report_file"
			sleep 1
			alert "A file named $(basename $report_file) has been created on your desktop. You can copy this file to an external drive to submit it with another operating system." "Ok"
		else
			rm "$report_file"
		fi
	) &
}

detect_network ()
{
	ping -c 1 "$report_site"
	if [ "$?" -gt 0 ]; then
		alert --stop "Cannot contact the hardware report site ($report_site).
You can continue anyway and generate a local file to submit later on, or try to configure networking." "Cancel" "Configure Network" "Continue"
		case "$?" in
		0)
			exit 0
			;;
		1)
			/system/preferences/Network
			detect_network
			;;
		2)
			start_fake_httpd
			;;
		*)
			exit 1
			;;
		esac
	fi
}

check_pci ()
{
	echo "<h2>PCI devices</h2><dl>"
	echo "<div><i>List ot detected PCI devices. This does not indicate that every probed device is supported by a driver.</i></div><br />"
	devn=0
	bus="pci"
	vendor=''
	device=''
	true;
	listdev | while read line; do
	
	case "$line" in
	device*)
		case "$vendor" in
		"")
			desc="${line/device /}"
			echo "<dt><b>$desc</b></dt>"
			;;
		*)
			devicestr=${line#*:}
			device="${line%:*}"
			device="${device#device }"
			echo "<dd>"
			echo "<div>$vendor:$device <i>$vendorstr:$devicestr</i></div>"
			descline="$vendor:$device \"$vendorstr\" \"$devicestr\" $desc"
			echo "<div>Identification: <input type='text' id='$bus${devn}_desc' name='$bus${devn}_desc' value='$descline' readonly='readonly' size='80' /></div>"

			echo "<div>"
			echo "<table border='0'>"
			echo "<tr><td>"
			echo "Status: "
			echo "</td><td>"
			echo "<input type='radio' name='$bus${devn}_status' id='$bus${devn}_status_ok' value='ok' /><label for='$bus${devn}_status_ok' class='status_ok'>Working</label>"
			echo "</td></tr><tr><td></td><td>"
			#echo "<br />"
			echo "<input type='radio' name='$bus${devn}_status' id='$bus${devn}_status_ko' value='ko' /><label for='$bus${devn}_status_ko' class='status_ko'>Not working</label>"
			echo "</td></tr><tr><td></td><td>"
			#echo "<br />"
			echo "<input type='radio' name='$bus${devn}_status' id='$bus${devn}_status_unkn' value='unkn' checked='checked' /><label for='$bus${devn}_status_unkn' class='status_unkn'>Unknown</label>"
			echo "</td></tr>"
			echo "</table>"
			echo "</div>"

			echo "<div>"
			echo "Is it an add-in card (not part of the motherboard) ? "
			echo "<input type='checkbox' name='$bus${devn}_addin' id='$bus${devn}_addin' /><label for='$bus${devn}_addin'>Yes</label>"
			echo "</div>"

			echo "<div>"
			echo "Comment: "
			echo "<input type='text' name='$bus${devn}_comment' id='$bus${devn}_comment' placeholder='bug, missing feature...' size='30' />"
			echo "</div>"
			
			echo "<br />"

			echo "</dd>"

			vendor=''
			devn=$(($devn+1))
			;;
		esac
		;;
	vendor*)
		vendorstr=${line#*:}
		vendor="${line%:*}"
		vendor="${vendor#vendor }"
		;;
	*)
		;;
	esac
	done
}

check_usb ()
{
	echo "<h2>USB devices</h2><dl>"
	echo "<div><i>List ot detected USB devices. This does not indicate that every probed device is supported by a driver.</i></div><br />"
	devn=0
	bus="usb"
	listusb | while read vpid dev desc; do
		echo "<dt><b>$desc</b></dt>"
		echo "<dd>"
		echo "<div>Identification: <input type='text' id='$bus${devn}_desc' name='$bus${devn}_desc' value='$vpid $dev $desc' readonly='readonly' size='80' /></div>"
		if [ "$vpid" != "0000:0000" ]; then
			enabled=1
			id=""

			echo "<div>"
			echo "<table border='0'>"
			echo "<tr><td>"
			echo "Status: "
			echo "</td><td>"
			echo "<input type='radio' name='$bus${devn}_status' id='$bus${devn}_status_ok' value='ok' /><label for='$bus${devn}_status_ok' class='status_ok'>Working</label>"
			echo "</td></tr><tr><td></td><td>"
			#echo "<br />"
			echo "<input type='radio' name='$bus${devn}_status' id='$bus${devn}_status_ko' value='ko' /><label for='$bus${devn}_status_ko' class='status_ko'>Not working</label>"
			echo "</td></tr><tr><td></td><td>"
			#echo "<br />"
			echo "<input type='radio' name='$bus${devn}_status' id='$bus${devn}_status_unkn' value='unkn' checked='checked' /><label for='$bus${devn}_status_unkn' class='status_unkn'>Unknown</label>"
			echo "</td></tr>"
			echo "</table>"
			echo "</div>"

			echo "<div>"
			echo "Is it an external device (not part of the motherboard) ? "
			echo "<input type='checkbox' name='$bus${devn}_addin' id='$bus${devn}_addin' name='$bus${devn}_addin' /><label for='$bus${devn}_addin'>Yes</label>"
			echo "</div>"

			echo "<div>"
			echo "Comment: "
			echo "<input type='text' name='$bus${devn}_comment' id='$bus${devn}_comment' placeholder='bug, missing feature...' size='30' />"
			echo "</div>"
		else
			echo "<div><i>(virtual device)</i></div>"
		fi

		echo "<br />"
		echo "</dd>"
		devn=$(($devn+1))
	done
	echo "</dl>"
}

check_machine ()
{
	echo "<h2>Machine</h2>"
	echo "Vendor: <input type='text' name='machine_vendor' id='machine_vendor' placeholder='Lenovo,HP,Asus...' />"
	echo "<br />"
	echo "Model: <input type='text' name='machine_model' id='machine_model' placeholder='T510,l4500r...' />"
	echo "<br />"
	echo "Specification page: <input type='url' name='machine_url' id='machine_url' placeholder='url of the model page on the vendor website' />"
	echo "<br />"
	echo "Comments: <br />"
	echo "<textarea style='font-family: monospace' rows='10' cols='40' name='machine_comments' id='machine_comments' placeholder='specific options...'></textarea>"
	echo "<br />"
}

check_haiku ()
{
	echo "<h2>Haiku</h2>"
	uname_r="$(uname -r)"
	uname_v="$(uname -v)"
	echo "Release: <input type='text' name='uname_r' id='uname_r' value='$uname_r' readonly='readonly' size='6' />"
	echo "<br />"
	echo "Version: <input type='text' name='uname_v' id='uname_v' value='$uname_v' readonly='readonly' size='30' />"
	echo "<br />"
	echo "Comments: <br />"
	echo "<textarea style='font-family: monospace' rows='4' cols='40' name='haiku_comments' id='haiku_comments' placeholder='Custom build, gcc4...' ></textarea>"
	echo "<br />"
}

check_utils ()
{
	echo "<h2>Utilities output</h2>"
	echo "<i>The output of some system utilities gives precious informations on the processor model and other stuff...</i>"

	echo "<h3><tt>sysinfo</tt></h3>"
	echo "<i>(system info)</i><br />"
	echo "<textarea style='font-family: monospace' rows='10' cols='80' name='sysinfo_output' id='sysinfo_output' readonly='readonly'>"
	sysinfo
	echo "</textarea>"

	echo "<h3><tt>listimage 1</tt></h3>"
	echo "<i>(list of loaded kernel drivers)</i><br />"
	echo "<textarea style='font-family: monospace' rows='10' cols='80' name='listimage_1_output' id='listimage_1_output' readonly='readonly'>"
	listimage 1
	echo "</textarea>"

	echo "<h3><tt>ifconfig</tt></h3>"
	echo "<i>(list of network interfaces)</i><br />"
	echo "<textarea style='font-family: monospace' rows='10' cols='80' name='ifconfig_output' id='ifconfig_output' readonly='readonly'>"
	ifconfig
	echo "</textarea>"

	echo "<h3><tt>installoptionalpackage -l</tt></h3>"
	echo "<i>(list of installed packaged)</i><br />"
	echo "<textarea style='font-family: monospace' rows='10' cols='80' name='installoptpkg_l_output' id='installoptpkg_l_output' readonly='readonly'>"
	installoptionalpackage -l
	echo "</textarea>"

	echo "<br />"
}

check_syslog ()
{
	echo "<h2>System log</h2>"
	echo "<div><i>Part of the system boot log that could help developer understand why some devices are not recognized...</i></div>"
	echo "<textarea style='font-family: monospace' rows='10' cols='80' name='syslog' id='syslog' readonly='readonly'>"
	cat /var/log/syslog
	echo "</textarea>"
	
}

check_sender ()
{
	echo "<h2>Sender info (optional)</h2>"
	echo "Name: <input type='text' name='sender_name' id='sender_name' placeholder='Your name' />"
	echo "<br />"
	echo "Mail: <input type='email' name='sender_name' id='sender_name' placeholder='contact email' />"
	echo "<br />"
	echo "Other comments: <br />"
	echo "<textarea style='font-family: monospace' rows='4' cols='40' name='sender_comments' id='sender_comments' placeholder='IRC nickname on freenode...' ></textarea>"
	echo "<br />"
}

check_all ()
{
	echo "<html>"
	echo "<head>"
	echo '<meta http-equiv="content-type" content="text/html; charset=utf-8">'
	echo "<title>Hardware report</title>"
	#echo '<link rel="stylesheet" type="text/css" href="http://svn.haiku-os.org/haiku/haiku/trunk/docs/welcome/Haiku-doc.css">'
	
	echo "<style type='text/css'>"
	echo ".status_ok { color: #008000 }"
	echo ".status_ko { color: #800000 }"
	echo ".status_unkn { color: #000080 }"
	echo "</style>"
	echo "</head>"
	echo "<body>"
	echo "<div id='content'>"
	echo "<form method='POST' action='$report_cgi'>"
	
	do_notify 0.1 "Checking for PCI hardware..."
	check_pci
	
	do_notify 0.3 "Checking for USB hardware..."
	check_usb
	
	do_notify 0.5 "Checking for Haiku version..." 
	check_haiku

	do_notify 0.6 "Checking for utility outputs..." 
	check_utils

	do_notify 0.8 "Dumping syslog output..." 
	check_syslog

	check_machine
	check_sender

	do_notify 1.0 "Done!" --timeout 3
	
	echo "<div><i>Note: this form will only send data that is visible on this page.</i></div>"
	
	echo "<input type='submit' name='submit' value='submit'>"
	
	echo "</form>"
	echo "</div>"
	echo "</body>"
	echo "</html>"
}

tf=/tmp/hw_checker_$$.html

do_notify 0.0 "Checking for network..."
detect_network

check_all > "$tf"

open "$tf"

