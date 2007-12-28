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
define("APPLET_WIDTH", "800");
define("APPLET_HEIGHT", "600");
// vnc protocol base port.
define("VNCPORTBASE", 5900);

// timeout before the demo session is killed, as argument to /bin/sleep
define("SESSION_TIMEOUT", "10m");

// path to qemu binary
define("QEMU_BIN", "/usr/bin/qemu");
// default arguments: no network, emulate tablet, readonly image file.
define("QEMU_ARGS","-net none -usbdevice tablet -snapshot");
// absolute path to the image.
define("QEMU_IMAGE_PATH","/home/revol/haiku/trunk/generated/haiku.image");

// name of session and pid files in /tmp
define("QEMU_SESSFILE_TMPL", "qemu-haiku-session-");
define("QEMU_PIDFILE_TMPL", "qemu-haiku-pid-");
// name of session variable holding the qemu slot; not yet used correctly
define("QEMU_IDX_VAR", "QEMU_HAIKU_SESSION_VAR");

session_start();

if (isset($_GET['frame'])) {
}

?>
<html>
<head>
<title>Haiku Test</title>
</head>
<script>
function onPageUnload() {
	//window.open("<?php echo $_SERVER["SCRIPT_NAME"] . "?close"; ?>", "closing", "width=100,height=30,location=no,menubar=no,toolbar=no,scrollbars=no");
}
</script>
<?php


// statics

$count = $_SESSION['compteur'];
//$count = $GLOBALS['compteur'];
$closing = 0;
if (isset($_GET['close'])) {
	$closing = 1;
	echo "<body>";
} else
	echo "<body onunload=\"onPageUnload();\">";

function dbg($str)
{
	echo "<div class=\"debug\">$str</div>\n";
}

function err($str)
{
	echo "<div class=\"error\">$str</div>\n";
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


function start_qemu()
{
	$idx = find_qemu_slot();
	if ($idx < 0) {
		err("No available qemu slot, please try later.");
		return $idx;
	}
	$pidfile = make_qemu_pidfile_name($idx);
	$cmd = QEMU_BIN . " " . QEMU_ARGS . " -vnc " . vnc_display() . " -pidfile " . $pidfile . " " . QEMU_IMAGE_PATH;

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


function output_applet_code()
{
	$w = APPLET_WIDTH;
	$h = APPLET_HEIGHT;
	$port = vnc_port();
	$vncjpath = VNCJAVA_PATH;
	$jar = VNCJAR;
	$class = VNCCLASS;
	echo "<applet code=$class codebase=\"$vncjpath/\" archive=\"$vncjpath/$jar\" width=$w height=$h>
	<param name=\"PORT\" value=\"$port\">
	<!param name=\"HOST\" value=\"$HTTP_HOST\"><!-- no need -->
	<param name=\"PORT\" value=\"$port\">
	<param name=\"PASSWORD\" value=\"\">
	There should be a java applet here... make sure you have a JVM and it's enabled!
	</applet>";
}

dbg("Checking if session is running...");

if (is_my_session_valid()) {
	dbg("Session running");
	$qemuidx = qemu_slot();
} else if ($closing != 1) {
	dbg("Need to start qemu");

	$qemuidx = start_qemu();
}

if ($qemuidx >= 0) {
	if ($closing) {
		dbg("closing...");
		unlink(make_qemu_sessionfile_name($qemuidx));
		//unlink(make_qemu_sessionfile_name($qemuidx));
		sleep(1);
		//echo "<script> self.close(\"closing\"); </script>";
	} else {
		dbg("Waiting for vnc server...");
		sleep(5);
		output_applet_code();
	}
}



?>

</body>
</html>
