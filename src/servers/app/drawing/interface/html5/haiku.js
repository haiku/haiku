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

var b64Values = new Array(
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -2, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1);

//DEBUG
var stream={s:"YWJjZA==YWI=YQ==VQ==qg==qlU=Vao=VaqqVQ==qlVVqg==",i:0};
var stream2={s:"YWJjZAYWIYQVQqgqlUVaoVaqqVQqlVVqg",i:0};


function b64_get_u32_(t, i)
{
	var v = 0;
	if (b64Values[t.charCodeAt(i)] < 0)	return v;
	v += b64Values[t.charCodeAt(i++)] << 26;
	if (b64Values[t.charCodeAt(i)] < 0)	return v;
	v += b64Values[t.charCodeAt(i++)] << 20;
	if (b64Values[t.charCodeAt(i)] < 0)	return v;
	v += b64Values[t.charCodeAt(i++)] << 14;
	if (b64Values[t.charCodeAt(i)] < 0)	return v;
	v += b64Values[t.charCodeAt(i++)] << 8;
	if (b64Values[t.charCodeAt(i)] < 0)	return v;
	v += b64Values[t.charCodeAt(i++)] << 2;
	if (b64Values[t.charCodeAt(i)] < 0)	return v;
	v += b64Values[t.charCodeAt(i++)] >> 4;
	if (b64Values[t.charCodeAt(i)] < 0)	i++;
	if (b64Values[t.charCodeAt(i)] < 0)	i++;
	dbg("u32:" + v + " 0x" + v.toString(16));
	return v;
}


function b64_get_u8(s)
{
	var v = 0;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] << 2;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] >> 4;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	s.i++;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	s.i++;
	dbg(" u8:" + v + " 0x" + v.toString(16));
	return v;
}

function b64_get_u16(s)
{
	var v = 0;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] << 10;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] << 4;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] >> 2;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	s.i++;
	dbg("u16:" + v + " 0x" + v.toString(16));
	return v;
}

function b64_get_u32(s)
{
	var v = 0;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] << 26;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] << 20;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] << 14;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] << 8;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] << 2;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	return v;
	v += b64Values[s.s.charCodeAt(s.i++)] >> 4;
	//v &= 0x0ffffffff;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	s.i++;
	if (b64Values[s.s.charCodeAt(s.i)] < 0)	s.i++;
	dbg("u32:" + v + " 0x" + v.toString(16));
	return v;
}

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

function decodeCanvasMessage(msg)
{
	dbg("decodeCanvasMessage()");
	var code = b64_get_u16(msg);
	dbg("code: " + code);
/*	switch (code) {
	//case 1:
		//break;
	default:
		dbg("unhandled code " + code);
		return;
	}*/
}

function onReadyStateChange()
{
	dbg("readystate changed: " + this.readyState);
	if (this.readyState != 4)
		return;
	dbg("status: " + this.status);
	dbg("resonseType: " + this.responseType);
	dbg("response: " + this.responseText);
	//dbg("headers: " + this.getAllResponseHeaders());
//var stream={s:"YWJjZA==YWI=YQ==VQ==qg==qlU=Vao=VaqqVQ==qlVVqg==",i:0};
	var msg = {s:(this.responseText),i:0};
	decodeCanvasMessage(msg);
}

//DEBUG
function testB64(stream)
{
	var i = 0;
	b64_get_u32(stream);
	b64_get_u16(stream);
	b64_get_u8(stream);
	b64_get_u8(stream);
	b64_get_u8(stream);
	b64_get_u16(stream);
	b64_get_u16(stream);
	b64_get_u32(stream);
	b64_get_u32(stream);
	dbg("s.i:"+stream.i);
}

function initDesktop()
{
	dbg("initDesktop()");

	//testB64(stream);
	//testB64(stream2);

//var stream={s:"YWJjZA==YWI=YQ==VQ==qg==qlU=Vao=VaqqVQ==qlVVqg==",i:0};
	var msg = {s:"YWJjZA==YWI=YQ==VQ==qg==qlU=Vao=VaqqVQ==qlVVqg==",i:0};
	decodeCanvasMessage(stream);

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
		xhr.onreadystatechange = onReadyStateChange;
		//xhr.responseType = "arraybuffer";
		//dbg("xhr.onload:" + xhr.onload);
		//dbg("xhr.send:" + xhr.send);
		//xhr.overrideMimeType("multipart/x-mixed-replace;boundary=x");
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


