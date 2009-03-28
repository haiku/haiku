<?php

/*
 * haiku.php - an online Haiku demo using qemu and vnc.
 * Copyright 2007, Francois Revol, revol@free.fr.
 */

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
define("VNCJAVA_PATH", "tightvnc-java");
define("VNCJAR", "VncViewer.jar");
define("VNCCLASS", "VncViewer.class");

// maximum count of qemu instances.
define("MAX_QEMUS", 8);

// size of the java applet, must match the default resolution of the image.
//define("APPLET_WIDTH", "800");
//define("APPLET_HEIGHT", "600");
define("APPLET_WIDTH", "1024");
define("APPLET_HEIGHT", "768");
// vnc protocol base port.
define("VNCPORTBASE", 5900);

// timeout before the demo session is killed, as argument to /bin/sleep
define("SESSION_TIMEOUT", "20m");

// path to qemu binary
define("QEMU_BASE", "/usr/local");
define("QEMU_BIN", QEMU_BASE . "/bin/qemu");
define("QEMU_KEYMAPS", QEMU_BASE . "/share/qemu/keymaps");
// default arguments: no network, emulate tablet, readonly image file.
define("QEMU_ARGS", ""
	."-monitor /dev/null "
	."-serial none "
	."-parallel none "
	." -net none "
	."-usbdevice wacom-tablet "
	."-vga vmware "
	."-snapshot");
// absolute path to the image.
define("QEMU_IMAGE_PATH","/home/revol/haiku/trunk/generated.x86/haiku.image");
// qemu 0.8.2 needs "", qemu 0.9.1 needs ":"
define("QEMU_VNC_PREFIX", ":");

// name of session and pid files in /tmp
define("QEMU_SESSFILE_TMPL", "qemu-haiku-session-");
define("QEMU_PIDFILE_TMPL", "qemu-haiku-pid-");
// name of session variable holding the qemu slot; not yet used correctly
define("QEMU_IDX_VAR", "QEMU_HAIKU_SESSION_VAR");

define("BGCOLOR", "#336698");

$vnckeymap = "en-us";

// statics
$count = $_SESSION['compteur'];
//$count = $GLOBALS['compteur'];
$closing = 0;
$do_kill = 0;
$do_run = 0;

// parse args
if (isset($_GET['close']))
	$closing = 1;

if (isset($_GET['kill']))
	$do_kill = 1;

if (isset($_GET['run']))
	$do_run = 1;

if (isset($_GET['frame'])) {}

session_start();

//echo "do_run: " . $do_run . "<br>\n";
//echo "do_kill: " . $do_kill . "<br>\n";

?>
<html>
<head>
<meta name="robots" content="noindex, nofollow, noarchive">
<title>Haiku Online Demo</title>
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
</head>
<script>
function onPageUnload() {
	//window.open("<?php echo $_SERVER["SCRIPT_NAME"] . "?close"; ?>", "closing", "width=100,height=30,location=no,menubar=no,toolbar=no,scrollbars=no");
}
</script>
<?php


if ($closing == 1)
	echo "<body>";
else
	echo "<body onunload=\"onPageUnload();\">";

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

function vnc_display()
{
	return qemu_slot();
}

function vnc_port()
{
	return VNCPORTBASE + vnc_display();
}

function vnc_addr_display()
{
	return $_SERVER['SERVER_ADDR'] . ":" . vnc_display();
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
		if (in_keymaps($lang))
		{
			$vnckeymap = $lang;
			dbg("Detected keymap '" . $vnckeymap . "' from browser headers.");
			return;
		}
	}
}


function output_options_form()
{
	global $vnckeymap;
	$idx = qemu_slot();
	echo "<form method=\"get\" action=\"" . $_SERVER['PHP_SELF'] . "\">";
	echo "<table border=\"0\" class=\"haiku_online_form\">\n";
	echo "<tr>\n";
	echo "<td align=\"right\">\n";
	echo "Select your keymap:";
	echo "</td>\n";
	echo "<td>\n";
	echo "<select name=\"keymap\">";
	$keymaps = list_keymaps();
	foreach ($keymaps as $keymap) {
		echo "<option name=\"keymap\" value=\"$keymap\" ";
		if ($keymap == $vnckeymap)
			echo "selected ";
		echo ">$keymap</option>";
		//echo "<option name=\"keymap\" ";
		//echo "value=\"$keymap\">" . locale_get_display_name($keymap);
		//echo "</option>";
	}
	echo "</select>";
	echo "</td>\n";
	echo "</tr>\n";

	$modes = array("1024x768"/*, "800x600"*/);
	echo "<tr ";
	if (count($modes) < 2)
		echo "class=\"haiku_online_disabled\"";
	echo ">\n";
	echo "<td align=\"right\">\n";
	echo "Select display size:";
	echo "</td>\n";
	echo "<td>\n";
	echo "<select name=\"videomode\" ";
	if (count($modes) < 2)
		echo "disabled";
	echo ">";
	
	foreach ($modes as $mode) {
		echo "<option name=\"videomode\" value=\"$mode\" ";
		if ($mode == $videomode)
			echo "selected ";
		echo ">$mode</option>";
	}
	echo "</select>";

	echo "</td>\n";

	echo "</tr>\n";
	echo "<tr ";
	if (!$enable_sound)
		echo "class=\"haiku_online_disabled\"";
	echo ">\n";
	echo "<td align=\"right\">\n";
	echo "Click here to enable sound:";
	echo "</td>\n";
	echo "<td>\n";
	echo "<input type=\"checkbox\" name=\"sound\" ";
	echo "value=\"1\" disabled ";
	if ($enable_sound)
		echo "checked ";
	echo ">Sound</input>";
	echo "</td>\n";
	echo "</tr>\n";
	echo "<tr>\n";
	echo "<td align=\"right\">\n";
	//out("Click here to enable sound:");
	echo "</td>\n";
	echo "<td>\n";
	echo "</td>\n";
	echo "</tr>\n";
	echo "<tr>\n";
	echo "<td align=\"right\">\n";
	echo "Click here to start the session:";
	echo "</td>\n";
	echo "<td>\n";
	echo "<input type=\"submit\" name=\"run\" ";
	echo "value=\"Start!\" />";
	echo "</td>\n";
	echo "</tr>\n";
	echo "</table>\n";
	echo "</form>\n";
	out("NOTE: You will need a Java-enabled browser to display the VNC Applet needed by this demo.");
	out("You can however use instead an external <a " .
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
	$idx = find_qemu_slot();
	if ($idx < 0) {
		err("No available qemu slot, please try later.");
		return $idx;
	}
	$pidfile = make_qemu_pidfile_name($idx);
	$cmd = QEMU_BIN . " " . QEMU_ARGS . " -k " . $vnckeymap . " -vnc " . QEMU_VNC_PREFIX . vnc_display() . " -pidfile " . $pidfile . " " . QEMU_IMAGE_PATH;

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
	$cmd = "(sleep " . SESSION_TIMEOUT . "; kill -9 `cat " . $pidfile . "`; rm " . $pidfile . " " . $sessfile . ") &";

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
	out("You can use an external VNC client, click " .
	"<a href=\"vnc://" . vnc_addr_display() . "\">here</a> " .
	"or enter <tt>" . vnc_addr_display() . "</tt> in your " .
	"<a href=\"http://fr.wikipedia.org/wiki/Virtual_Network_Computing\"" .
	">VNC viewer</a>.<br />");
}

function output_applet_code()
{
	$w = APPLET_WIDTH;
	$h = APPLET_HEIGHT;
	$port = vnc_port();
	$vncjpath = VNCJAVA_PATH;
	$jar = VNCJAR;
	$class = VNCCLASS;
	echo "<a name=\"haiku_online_applet\">";
	echo "<center>";
	echo "<applet code=$class codebase=\"$vncjpath/\" ";
	echo "archive=\"$vncjpath/$jar\" width=$w height=$h ";
	echo "bgcolor=\"#336698\">\n";
	//not needed
	//echo "<param name=\"HOST\" value=\"$HTTP_HOST\">\n";
	echo "<param name=\"PORT\" value=\"$port\">\n";
	echo "<param name=\"PASSWORD\" value=\"\">\n";
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
	echo "window.location.hash = \"haiku_online_applet\";";
	echo "//--></script>\n";
	ob_flush();
	flush();
}

out("<div align\"right\">Available displays: " .
    available_qemu_slots() . "/" . total_qemu_slots() .
    "</div>");


probe_keymap();


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
	out("Waiting for vnc server...");
	sleep(5);
}


if ($qemuidx >= 0 && !$do_kill) {
	output_kill_form();
	output_vnc_info();
	echo "<br />\n";
	output_applet_code();
} else {
	output_options_form();
}



?>

</body>
</html>
