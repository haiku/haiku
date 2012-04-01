/*
 * Copyright 2012, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 *
 * Loosely inspired by
 * http://git.gnome.org/browse/gtk+/tree/gdk/broadway/broadway.js?h=broadway
 */

var logDiv;
var desktop;	// the canvas

function dbg(str)
{
	var div = document.createElement("div");
	div.className = "haiku_online_debug";
	div.appendChild(document.createTextNode(str));
	logDiv.appendChild(div);
}

function err(str)
{
	var div = document.createElement("div");
	div.className = "haiku_online_error";
	div.appendChild(document.createTextNode(str));
	logDiv.appendChild(div);
}

function createXHR()
{
	try {
		return new XMLHttpRequest();
	} catch(e) {
		err("Failed to create XHR: " + e);
	}
	return null;
}

function onMessage(message)
{
	dbg("onMessage: ");
window.popup("plop");
	dbg(message.target.responseText);
}

function initDesktop()
{
	dbg("initDesktop()");
	desktop = document.getElementById("desktop");
	var xhr = createXHR();
	if (xhr) {
		if (typeof xhr.multipart != "undefined")
			xhr.multipart = true;
		else {
			err("XHR has no multipart field!");
			return;
		}
		if (typeof xhr.async != "undefined")
			xhr.async = true;
		else {
			dbg("XHR has no async field!");
		}
		dbg("multipart: " + xhr.multipart);
		xhr.open("GET", "http://127.0.0.1:8080/output", true);
		//xhr.onload = onMessage;
		xhr.onreadystatechange = function() {
			dbg("readystate changed: " + xhr.readyState);
			if (xhr.readyState != 4)
				return;
			dbg("status: " + xhr.status);
			dbg("resonseType: " + xhr.responseType);
			dbg("response: " + xhr.responseText);
			dbg("headers: " + xhr.getAllResponseHeaders());

		}
		//xhr.responseType = "arraybuffer";
		//dbg("xhr.onload:" + xhr.onload);
		//dbg("xhr.send:" + xhr.send);
		xhr.overrideMimeType("multipart/x-mixed-replace;boundary=x");
		xhr.send(null);
		//dbg("headers: " + xhr.getAllResponseHeaders());
	} else
		err("No XHR");
	dbg("done");
}


function onPageLoad() {
	logDiv = document.getElementById("log");
	dbg("onPageLoad()");
	initDesktop();
}

function onPageUnload() {
	dbg("onPageUnload()");
}


