<?php

/*
 * haiku.php - an online Haiku demo using qemu and vnc.
 *
 * Copyright 2007-2011, Francois Revol, revol@free.fr.
 * Distributed under the terms of the MIT License.
 */

// parts inspired by the Free Live OS Zoo
// http://www.oszoo.org/wiki/index.php/Free_Live_OS_Zoo


// include local configuration that possibly overrides defines below.
if (file_exists('haiku.conf.php'))
	include('haiku.conf.php');


// name of the page
define("PAGE_TITLE", "Haiku Online Demo");


// relative path to the vnc java applet jar
// you must *copy* (apache doesn't seem to like symlinks) it there.

// on debian, apt-get install vnc-java will put them in
// /usr/share/vnc-java
//define("VNCJAVA_PATH", "vnc-java");
//define("VNCJAR", "vncviewer.jar");
//define("VNCCLASS", "vncviewer.class");

// to use the tightvnc applet instead (supports > 8bpp):
// on debian, apt-get install tightvnc-java will put them in
// /usr/share/tightvnc-java
// else you can get it from http://www.tightvnc.com/download-old.php :
// wget http://www.tightvnc.com/download/1.3.10/tightvnc-1.3.10_javabin.zip
// (you will have to move the VncViewer.jar file around)
define("VNCJAVA_PATH", "tightvnc-java");
define("VNCJAR", "VncViewer.jar");
define("VNCCLASS", "VncViewer.class");

// do not show applet controls
define("VNC_HIDE_CONTROLS", true);

// generate and use (plain text) passwords
// NOT IMPLEMENTED
//define("VNC_USE_PASS", true);

// maximum count of qemu instances.
define("MAX_QEMUS", 2);

// size of the java applet, must match the default resolution of the image.
//define("APPLET_WIDTH", "800");
//define("APPLET_HEIGHT", "600");
define("APPLET_WIDTH", "1024");
define("APPLET_HEIGHT", "768");
// vnc protocol base port.
define("VNCPORTBASE", 5900);

// base port for audio streams
//define("AUDIOPORTBASE", 8080);
define("AUDIOPORTBASE", (VNCPORTBASE + MAX_QEMUS));

// if audio is enabled
define("AUDIOENABLED", false);

// base port for serial output
//define("SERIALPORTBASE", 9000);
define("SERIALPORTBASE", (VNCPORTBASE + MAX_QEMUS * 2));

// timeout before the demo session is killed, as argument to /bin/sleep
define("SESSION_TIMEOUT", "20m");

// path to qemu binary
define("QEMU_BASE", "/usr/local");
define("QEMU_BIN", QEMU_BASE . "/bin/qemu");
define("QEMU_KEYMAPS", QEMU_BASE . "/share/qemu/keymaps");
// default arguments: no network, emulate tablet, readonly image file.
define("QEMU_ARGS", ""
	."-daemonize " /* detach from stdin */
	."-localtime " /* not UTC */
	."-name '" . addslashes(PAGE_TITLE) . "' "
	."-monitor null " /* disable the monitor */
	."-serial none "
	."-parallel none "
	."-net none "
	."-usbdevice wacom-tablet "
	//."-vga vmware "
	."-snapshot ");

// absolute path to the image.
define("QEMU_IMAGE_PATH", "/home/revol/haiku.image");
// BAD: let's one download the image
//define("QEMU_IMAGE_PATH", dirname($_SERVER['SCRIPT_FILENAME']) . "/haiku.image");

// max number of cpus for the VM, no more than 8
define("QEMU_MAX_CPUS", 1);

// qemu 0.8.2 needs "", qemu 0.9.1 needs ":"
define("QEMU_VNC_PREFIX", ":");

// name of session and pid files in /tmp
define("QEMU_SESSFILE_TMPL", "qemu-haiku-session-");
define("QEMU_PIDFILE_TMPL", "qemu-haiku-pid-");
define("QEMU_LOGFILE_TMPL", "qemu-haiku-log-");
// name of session variable holding the qemu slot; not yet used correctly
define("QEMU_IDX_VAR", "QEMU_HAIKU_SESSION_VAR");


// uncomment if you want to pass your Sonix webcam device through
// migth need to update VID:PID
// doesnt really work yet
//define("QEMU_USB_PASSTHROUGH", "-usbdevice host:0c45:6005");


define("BGCOLOR", "#336698");




$vnckeymap = "en-us";

$cpucount = 1;

// statics
//$count = $_SESSION['compteur'];
//$count = $GLOBALS['compteur'];
$closing = 0;
$do_kill = 0;
$do_run = 0;

function out($str)
{
	echo "<div class=\"haiku_online_out\">$str</div>\n";
	ob_flush();
	flush();
}

function dbg($str)
{
	echo "<div class=\"haiku_online_debug\">$str</div>\n";
	ob_flush();
	flush();
}

function err($str)
{
	echo "<div class=\"haiku_online_error\">$str</div>\n";
	ob_flush();
	flush();
}

function make_qemu_sessionfile_name($idx)
{
	return "/tmp/" . QEMU_SESSFILE_TMPL . $idx;
}

function make_qemu_pidfile_name($idx)
{
	return "/tmp/" . QEMU_PIDFILE_TMPL . $idx;
}

function make_qemu_logfile_name($idx)
{
	return "/tmp/" . QEMU_LOGFILE_TMPL . $idx;
}

function find_qemu_slot()
{
	for ($idx = 0; $idx < MAX_QEMUS; $idx++) {
		$pidfile = make_qemu_pidfile_name($idx);
		$sessfile = make_qemu_sessionfile_name($idx);
		dbg("checking \"$pidfile\", \"$sessfile\"...");
		if (!file_exists($pidfile) && !file_exists($sessfile)) {
			file_put_contents($sessfile, session_id());
			$sid = file_get_contents($sessfile);
			if ($sid != session_id())
				continue;
			$_SESSION[QEMU_IDX_VAR] = $idx;
			return $idx;
		}
	}
	return -1;
}

function total_qemu_slots()
{
	return MAX_QEMUS;
}


function available_qemu_slots()
{
	$count = 0;
	for ($idx = 0; $idx < MAX_QEMUS; $idx++) {
		$pidfile = make_qemu_pidfile_name($idx);
		$sessfile = make_qemu_sessionfile_name($idx);
		//dbg("checking \"$pidfile\", \"$sessfile\"...");
		if (!file_exists($pidfile) && !file_exists($sessfile))
			$count++;
	}
	return $count;
}

function qemu_slot()
{
	return $_SESSION[QEMU_IDX_VAR];
}

function audio_port()
{
	return AUDIOPORTBASE + qemu_slot();
}

function vnc_display()
{
	return qemu_slot();
}

function vnc_addr()
{
	return $_SERVER['HTTP_HOST'];
}

function vnc_port()
{
	return VNCPORTBASE + vnc_display();
}

function vnc_addr_display()
{
	return vnc_addr() . ":" . vnc_display();
}

function vnc_url()
{
	return "vnc://" . vnc_addr_display();
}

function is_my_session_valid()
{
	if (!isset($_SESSION[QEMU_IDX_VAR]))
		return 0;
	$idx = $_SESSION[QEMU_IDX_VAR];
	$sessfile = make_qemu_sessionfile_name($idx);
	if (!file_exists($sessfile))
		return 0;
	$qemusession=file_get_contents($sessfile);
	// has expired
	if ($qemusession != session_id()) {
		return 0;
	}
	return 1;
}


function list_keymaps()
{
	$bads = array('.', '..', 'common', 'modifiers');
	$keymaps = scandir(QEMU_KEYMAPS);
	foreach ($keymaps as $key => $map) {
		if (in_array($map, $bads))
			unset($keymaps[$key]);
	}
	return $keymaps;
}


function in_keymaps($keymap)
{
	$keymaps = list_keymaps();

	if ($keymap == "")
		return false;
	if (in_array($keymap, $keymaps))
		return true;

	return false;
}


function probe_keymap()
{
	global $vnckeymap;
	if (is_string($_GET['keymap']) && in_keymaps($_GET['keymap']))
	{
		$vnckeymap = $_GET['keymap'];
		dbg("Overriden keymap '" . $vnckeymap . "' in arguments.");
		return;
	}
	// if the browser advertised a prefered lang...
	if (!isset($_SERVER["HTTP_ACCEPT_LANGUAGE"]))
		return;
	$langs = $_SERVER["HTTP_ACCEPT_LANGUAGE"];
	$langs = ereg_replace(";q=[^,]*", "", $langs);
	$langs = str_replace(" ", "", $langs);
	$langs = split(",", $langs);
	//print_r($langs);
	//print_r($keymaps);
	foreach($langs as $lang)
	{
		if (!in_keymaps($lang))
			$lang = ereg_replace("-.*", "", $lang);
		if (in_keymaps($lang))
		{
			$vnckeymap = $lang;
			dbg("Detected keymap '" . $vnckeymap .
			    "' from browser headers.");
			return;
		}
	}
}


function probe_options_form()
{
	global $cpucount;
	$cpucount = 1;
	if (isset($_GET['cpucount']))
		$cpucount = (int)$_GET['cpucount'];
	$cpucount = max(min($cpucount, QEMU_MAX_CPUS), 1);
	//dbg("cpucount $cpucount");
}


function output_options_form()
{
	global $vnckeymap;
	$idx = qemu_slot();
	echo "<form method=\"get\" action=\"" . $_SERVER['PHP_SELF'] . "\">";
	echo "<table border=\"0\" class=\"haiku_online_form\">\n";

	$keymaps = list_keymaps();
	echo "<tr>\n<td align=\"right\">\n";
	echo "Select your keymap:";
	echo "</td>\n<td>\n";
	echo "<select name=\"keymap\">";
	foreach ($keymaps as $keymap) {
		echo "<option value=\"$keymap\" ";
		if ($keymap == $vnckeymap)
			echo "selected=\"selected\" ";
		echo ">$keymap</option>";
		//echo "<option name=\"keymap\" ";
		//echo "value=\"$keymap\">" . locale_get_display_name($keymap);
		//echo "</option>";
	}
	echo "</select>";
	echo "</td>\n</tr>\n";

	
	$modes = array("1024x768"/*, "800x600"*/);
	echo "<tr ";
	if (count($modes) < 2)
		echo "class=\"haiku_online_disabled\"";
	echo ">\n";
	echo "<td align=\"right\">\n";
	echo "Select display size:";
	echo "</td>\n<td>\n";
	echo "<select name=\"videomode\" ";
	if (count($modes) < 2)
		echo "disabled=\"disabled\"";
	echo ">";
	foreach ($modes as $mode) {
		echo "<option value=\"$mode\" ";
		if ($mode == $videomode)
			echo "selected=\"selected\" ";
		echo ">$mode</option>";
	}
	echo "</select>";
	echo "</td>\n</tr>\n";


	echo "<tr ";
	if (QEMU_MAX_CPUS < 2)
		echo "class=\"haiku_online_disabled\"";
	echo ">\n";
	echo "<td align=\"right\">\n";
	echo "Select cpu count:";
	echo "</td>\n<td>\n";
	echo "<select name=\"cpucount\" ";
	if (QEMU_MAX_CPUS < 2)
		echo "disabled=\"disabled\"";
	echo ">";
	for ($ncpu = 1; $ncpu <= QEMU_MAX_CPUS; $ncpu++) {
		echo "<option value=\"$ncpu\" ";
		if ($ncpu == 1)
			echo "selected=\"selected\" ";
		echo ">$ncpu</option>";
	}
	echo "</select>";
	echo "</td>\n</tr>\n";

	
	echo "<tr ";
	if (!AUDIOENABLED)
		echo "class=\"haiku_online_disabled\"";
	echo ">\n";
	echo "<td align=\"right\">\n";
	echo "Check to enable sound:";
	echo "</td>\n<td>\n";
	echo "<input type=\"checkbox\" name=\"sound\" id=\"sound_cb\" ";
	echo "value=\"1\" ";
	if (AUDIOENABLED) {
		//echo "checked=\"checked\" /";
	} else
		echo "disabled=\"disabled\" /";
	echo "><label for=\"sound_cb\">Sound</label>";
	echo "</td>\n</tr>\n";

	$enable_serial = 1;
	echo "<tr ";
	if (!$enable_serial)
		echo "class=\"haiku_online_disabled\"";
	echo ">\n";
	echo "<td align=\"right\">\n";
	echo "Check to enable serial output:";
	echo "</td>\n<td>\n";
	echo "<input type=\"checkbox\" name=\"serial\" id=\"serial_cb\" ";
	echo "value=\"1\" "/*"disabled "*/;
	if ($enable_serial) {
		//echo "checked ";
	}
	echo "/><label for=\"serial_cb\">Serial</label>";
	echo "</td>\n</tr>\n";

	if (defined("QEMU_USB_PASSTHROUGH")) {

		$enable_webcam = 1;
		echo "<tr ";
		if (!$enable_webcam)
			echo "class=\"haiku_online_disabled\"";
		echo ">\n";
		echo "<td align=\"right\">\n";
		echo "Check to enable webcam:";
		echo "</td>\n<td>\n";
		echo "<input type=\"checkbox\" name=\"webcam\" id=\"webcam_cb\" ";
		echo "value=\"1\" "/*"disabled "*/;
		if ($enable_webcam) {
			//echo "checked ";
		}
		echo "/><label for=\"webcam_cb\">Webcam</label>";
		echo "</td>\n</tr>\n";
	}	
	/*
	echo "<tr>\n<td align=\"right\">\n";
	//out("Click here to enable sound:");
	echo "</td>\n<td>\n";
	echo "</td>\n</tr>\n";

	echo "<tr>\n<td align=\"right\">\n";
	//out("Click here to enable sound:");
	echo "</td>\n<td>\n";
	echo "</td>\n</tr>\n";
	*/

	echo "<tr>\n<td align=\"right\">\n";
	echo "Click here to start the session:";
	echo "</td>\n<td>\n";
	echo "<input type=\"submit\" name=\"run\" ";
	echo "value=\"Start!\" />";
	echo "</td>\n</tr>\n";

	echo "</table>\n";
	echo "</form>\n";
	out("NOTE: You will need a Java-enabled browser to display the VNC " .
	    "Applet used by this demo. " .
	    "You can however use instead an external <a " .
	    "href=\"http://fr.wikipedia.org/wiki/Virtual_Network_Computing\"" .
	    ">VNC viewer</a>.");
	ob_flush();
	flush();
}

function output_kill_form()
{
	echo "<form method=\"get\" action=\"" . $_SERVER['PHP_SELF'] . "\">";
	echo "<table border=\"0\" class=\"haiku_online_form\">\n";
	echo "<tr>\n";
	echo "<td>\n";
	echo "Click here to kill the session:";
	echo "</td>\n";
	echo "<td>\n";
	echo "<input type=\"submit\" name=\"kill\" ";
	echo "value=\"Terminate\"/>";
	echo "</td>\n";
	echo "</tr>\n";
	echo "</table>\n";
	echo "</form>\n";
	ob_flush();
	flush();
}


function start_qemu()
{
	global $vnckeymap;
	global $cpucount;
	$idx = find_qemu_slot();
	if ($idx < 0) {
		err("No available qemu slot, please try later.");
		return $idx;
	}
	$pidfile = make_qemu_pidfile_name($idx);
	$logfile = make_qemu_logfile_name($idx);
	$cmd = '';
	if (isset($_GET['sound'])) {
		$cmd .= "QEMU_AUDIO_DRV=twolame ";
		//$cmd .= "QEMU_TWOLAME_SAMPLES=" . 4096 . " ";
		$cmd .= "QEMU_TWOLAME_PORT=" . audio_port() . " ";
	}
	$cmd .= QEMU_BIN . " " . QEMU_ARGS;
	if ($cpucount > 1)
		$cmd .= " -smp " . $cpucount;
	if (isset($_GET['sound'])) {
		$cmd .= " -soundhw hda";
	}
	if (isset($_GET['serial'])) {
		$cmd .= " -serial telnet::";
		$cmd .= (SERIALPORTBASE + qemu_slot());
		$cmd .= ",server,nowait,nodelay";
	}
	if (isset($_GET['webcam']) && defined("QEMU_USB_PASSTHROUGH")) {
		$cmd .= " " . QEMU_USB_PASSTHROUGH;
	}
	$cmd .= " -k " . $vnckeymap .
		" -vnc " . QEMU_VNC_PREFIX . vnc_display() .
		" -pidfile " . $pidfile .
		" " . QEMU_IMAGE_PATH;
	//$cmd .= " || echo $? && echo done )";
	// redirect output to log file
	//$cmd .= " >$logfile 2>&1";

	if (file_exists($pidfile))
		unlink($pidfile);
	dbg("Starting <tt>" . $cmd . "</tt>...");

	$descriptorspec = array(
	//       0 => array("pipe", "r"),   // stdin
	//       1 => array("pipe", "w"),  // stdout
	//       2 => array("pipe", "w")   // stderr
	);
	//$cmd="/bin/ls";
	//passthru($cmd, $ret);
	//dbg("ret=$ret");
	$cmd .= " &";
	$process = proc_open($cmd, $descriptorspec, $pipes);
	sleep(1);
	proc_close($process);

	dbg("Started QEMU.");
	$sessfile = make_qemu_sessionfile_name($idx);
	$cmd = "(PID=`cat " . $pidfile . "`; " .
	  "sleep " . SESSION_TIMEOUT . "; " .
	  "kill -9 \$PID && " .
	  "rm " . $pidfile . " " . $sessfile . ") &";

	$process = proc_open($cmd, $descriptorspec, $wkpipes);
	sleep(1);
	proc_close($process);

	dbg("Started timed kill.");
	dbg("Ready for a " . SESSION_TIMEOUT . " session.");
}

function stop_qemu()
{
	$qemuidx = qemu_slot();
	$pidfile = make_qemu_pidfile_name($qemuidx);
	if (file_exists($pidfile)) {
		$pid = file_get_contents($pidfile);
		//out("PID:" . $pid);
		system("/bin/kill -TERM " . $pid);
		unlink($pidfile);
	}
	$sessionfile = make_qemu_sessionfile_name($qemuidx);
	if (file_exists($sessionfile)) {
		unlink($sessionfile);
	}
	unset($_SESSION[QEMU_IDX_VAR]);

	out("reloading...");
	sleep(1);
	echo "<script>\n";
	echo "<!--\n";
	echo "window.location = \"" . $_SERVER['PHP_SELF'] . "\";\n";
	echo "//--></script>\n";
	out("Click <a href=\"" . $_SERVER['PHP_SELF'] .
	    "\">here</a> to reload the page.");
}

function output_vnc_info()
{
	out("You can use an external VNC client at " .
	    "<a href=\"vnc://" . vnc_addr_display() . "\">" .
	    "vnc://" . vnc_addr_display() . "</a> " .
	    "or open <a href=\"" . $_SERVER['PHP_SELF'] . "?getfile=vncinfo&slot=" . vnc_display() . "\">this file</a>, " .
	    "or enter <tt>" . vnc_addr_display() . "</tt> in your " .
	    "<a href=\"http://fr.wikipedia.org/wiki/Virtual_Network_" .
	    "Computing\"" .
	    ">VNC viewer</a>.");
	//echo "<br />\n";
}

function output_vnc_info_file()
{
	if (!is_my_session_valid())
		die("Bad request");

	header("Pragma: public");
	header("Expires: 0");
	header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
	header("Content-type: application/x-vnc");
	header('Content-Disposition: attachment; filename="onlinedemo.vnc"'); 

	echo "[connection]\n";
	echo "host=" . vnc_addr() . "\n";
	echo "port=" . vnc_display() . "\n";
	if (defined('VNC_USE_PASS') && VNC_USE_PASS)
		echo "password=" . $_SESSION['VNC_PASS'] . "\n";
	//echo "[options]\n";
	// cf. http://www.realvnc.com/pipermail/vnc-list/1999-December/011086.html
	// cf. http://www.tek-tips.com/viewthread.cfm?qid=1173303&page=1
	//echo "\n";
}

function output_audio_player_code($external_only=false)
{
	if (!isset($_GET['sound']))
		return;

	$port = audio_port();
	$url = "http://" . $_SERVER['HTTP_HOST'] . ":$port/";
	$icy = "icy://" . $_SERVER['HTTP_HOST'] . ":$port/";
	$use_html5 = true;

	if (!$external_only) {
		if ($use_html5) {
			echo "<audio autoplay=\"autoplay\" autobuffer=\"autobuffer\" controls=\"controls\">";
			echo "<source src=\"" . $url . "\" type=\"audio/mpeg\" />";
		}
		if (!$use_html5) {
			echo "<object type=\"audio/mpeg\" width=\"300\" height=\"50\">";
			echo "<param name=\"src\" value=\"" . $url . "\" />";
			echo "<param name=\"controller\" value=\"true\" />";
			echo "<param name=\"controls\" value=\"controlPanel\" />";
			echo "<param name=\"autoplay\" value=\"true\" />";
			echo "<param name=\"autostart\" value=\"1\" />";

			echo "<embed src=\"$url\" type=\"audio/mpeg\" ";
			echo "autoplay=\"true\" width=\"300\" height=\"50\" ";
			echo "controller=\"true\" align=\"right\" hidden=\"false\"></embed>";

			echo "</object>";
		}
		if ($use_html5) {
			echo "</audio>";
		}
	}
	out("You can use an external audio play at " .
	    "<a href=\"$url\">$url</a> or <a href=\"$icy\">$icy</a>, or use one of the playlists: " .
	    "<a href=\"" . $_SERVER['PHP_SELF'] . "?getfile=audiom3u\">[M3U]</a> " .
	    "<a href=\"" . $_SERVER['PHP_SELF'] . "?getfile=audiopls\">[PLS]</a>");
}

function output_audio_player_file_m3u()
{
	if (!is_my_session_valid())
		die("Bad request");

	header("Pragma: public");
	header("Expires: 0");
	header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
	header("Content-type: audio/x-mpegurl");
	//header("Content-type: text/plain");
	header('Content-Disposition: attachment; filename="onlinedemo.m3u"'); 

	$port = audio_port();
	$url = "http://" . $_SERVER['HTTP_HOST'] . ":$port/";

	// cf. http://hanna.pyxidis.org/tech/m3u.html
	echo "#EXTM3U\n";
	echo "#EXTINF:0," . PAGE_TITLE . "\n";
	echo "$url\n";
	//echo "\n";
}

function output_audio_player_file_pls()
{
	if (!is_my_session_valid())
		die("Bad request");

	header("Pragma: public");
	header("Expires: 0");
	header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
	header("Content-type: audio/x-scpls");
	//header("Content-type: text/plain");
	header('Content-Disposition: attachment; filename="onlinedemo.pls"');

	$port = audio_port();
	$url = "http://" . $_SERVER['HTTP_HOST'] . ":$port/";

	echo "[playlist]\n";
	echo "numberofentries=1\n";
	echo "File1=$url\n";
	echo "Title1=" . PAGE_TITLE . "\n";
	echo "Length1=-1\n";
	echo "version=2\n";
	//echo "\n";
}

function output_applet_code($external_only=false)
{
	$w = APPLET_WIDTH;
	$h = APPLET_HEIGHT;
	$port = vnc_port();
	$vncjpath = VNCJAVA_PATH;
	$jar = VNCJAR;
	$class = VNCCLASS;
	if ($external_only)
		return;
	if (!VNC_HIDE_CONTROLS)
		$h += 32;
	echo "<a name=\"haiku_online_applet\"></a>";
	echo "<center>";
	echo "<applet code=$class codebase=\"$vncjpath/\" ";
	echo "archive=\"$vncjpath/$jar\" width=$w height=$h ";
	echo "bgcolor=\"#336698\">\n";
	//not needed
	//echo "<param name=\"HOST\" value=\"$HTTP_HOST\">\n";
	echo "<param name=\"PORT\" value=\"$port\">\n";
	$pass = '';
	if (defined('VNC_USE_PASS') && VNC_USE_PASS)
		$pass = $_SESSION['VNC_PASS'];
	echo "<param name=\"PASSWORD\" value=\"" . $pass . "\">\n";
	if (VNC_HIDE_CONTROLS)
		echo "<param name=\"Show controls\" value=\"No\">\n";
	//echo "<param name=\"share desktop\" value=\"no\" />";
	echo "<param name=\"background-color\" value=\"#336698\">\n";
	echo "<param name=\"foreground-color\" value=\"#ffffff\">\n";
	//echo "<param name=\"background\" value=\"#336698\">\n";
	//echo "<param name=\"foreground\" value=\"#ffffff\">\n";
	echo "There should be a java applet here... ";
	echo "make sure you have a JVM and it's enabled!<br />\n";
	echo "If you do not have Java you can use an external VNC ";
	echo "client as described above.\n";

	echo "</applet>\n";
	echo "</center>";
	ob_flush();
	flush();
	// scroll to the top of the applet
	echo "<script>\n";
	echo "<!--\n";
	echo "scrollToAnchor(\"haiku_online_applet\");";
	echo "//--></script>\n";
	ob_flush();
	flush();
}

function output_serial_output_code($external_only=false)
{
	if (!isset($_GET['serial']))
		return;
	
	$url = "telnet://" . $_SERVER['HTTP_HOST'] . ":";
	$url .= (SERIALPORTBASE + qemu_slot()) . "/";
	out("You can get serial output at <a href=\"$url\">$url</a>");
	return;
	
	// not really http...
	$url = "http://" . $_SERVER['HTTP_HOST'] . ":";
	$url .= (SERIALPORTBASE + qemu_slot()) . "/";
	echo "<center>";
	echo "<iframe src=\"$url/\" type=\"text/plain\" width=\"100%\" ";
	echo "height=\"200\"></iframe>";
	echo "</center>";
  
}


session_start();

// parse args

// output redirections...
if (isset($_GET['getfile'])) {
	switch ($_GET['getfile']) {
	case "vncinfo":
		output_vnc_info_file();
		break;
	case "audiom3u":
		output_audio_player_file_m3u();
		break;
	case "audiopls":
		output_audio_player_file_pls();
		break;
	default:
		die("Bad request");
	}
	die();
}

if (isset($_GET['close']))
	$closing = 1;

if (isset($_GET['kill']))
	$do_kill = 1;

if (isset($_GET['run']))
	$do_run = 1;

if (isset($_GET['frame'])) {}


//echo "do_run: " . $do_run . "<br>\n";
//echo "do_kill: " . $do_kill . "<br>\n";

?>
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<meta name="robots" content="noindex, nofollow, noarchive" />
<title><?php echo PAGE_TITLE; ?></title>
<link rel="shortcut icon" href="http://www.haiku-os.org/sites/haiku-os.org/themes/shijin/favicon.ico" type="image/x-icon" />
<style type="text/css">
<!--
 /* basic style */
body { background-color: <?php echo BGCOLOR; ?>; }
a:link { color:orange; }
a:visited { color:darkorange; }
a:hover { color:pink; }
.haiku_online_form { color: white; }
.haiku_online_disabled { color: grey; }
.haiku_online_out { color: white; }
.haiku_online_debug { color: orange; }
.haiku_online_error { color: red; font-weight: bold; }
.haiku_online_applet { background-color: <?php echo BGCOLOR; ?>; }
-->
</style>
<script type="text/javascript">
function onPageUnload() {
	//window.open("<?php echo $_SERVER["SCRIPT_NAME"] . "?close"; ?>", "closing", "width=100,height=30,location=no,menubar=no,toolbar=no,scrollbars=no");
}

function scrollToAnchor(anchor) {
  var a = document.anchors[anchor];
  if (a) {
    if (a.scrollIntoView)
      a.scrollIntoView(true);
    else if (a.focus)
      a.focus();
  } else
    window.location.hash = anchor;
}
</script>
</head>
<?php


if ($closing == 1)
	echo "<body>";
else
	echo "<body onunload=\"onPageUnload();\">";


out("<div style=\"text-align:right;\">Available displays: " .
    available_qemu_slots() . "/" . total_qemu_slots() .
    "</div>");


probe_keymap();
probe_options_form();

dbg("Checking if session is running...");

$qemuidx = -1;

if (is_my_session_valid()) {
	dbg("Session running.");
	$qemuidx = qemu_slot();
	if ($do_kill) {
		dbg("closing...");
		stop_qemu();
	}
} else if (!$do_kill && $do_run) {
	dbg("Need to start qemu.");

	$qemuidx = start_qemu();
	//out("Waiting for vnc server...");
	//sleep(5);
}


if ($qemuidx >= 0 && !$do_kill) {
	output_kill_form();
	output_serial_output_code();
	output_audio_player_code();
	output_vnc_info();
	out("Waiting for vnc server...");
	sleep(1);
	output_applet_code();
} else {
	output_options_form();
}

//phpinfo();

?>

</body>
</html>
