/*---------------------------------------------------------------------------*
 |              PDFlib - A library for generating PDF on the fly             |
 +---------------------------------------------------------------------------+
 | Copyright (c) 1997-2004 Thomas Merz and PDFlib GmbH. All rights reserved. |
 +---------------------------------------------------------------------------+
 |                                                                           |
 |    This software is subject to the PDFlib license. It is NOT in the       |
 |    public domain. Extended versions and commercial licenses are           |
 |    available, please check http://www.pdflib.com.                         |
 |                                                                           |
 *---------------------------------------------------------------------------*/

/* $Id: pdflib_java.c 14574 2005-10-29 16:27:43Z bonefish $
 *
 * synch'd with pdflib.h 1.151.2.22
 *
 * JNI wrapper code for the PDFlib Java binding
 *
 */

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#ifdef OS390
#define IBM_MVS
#define NEEDSIEEE754
#define NEEDSLONGLONG
#endif

#include <jni.h>

#ifdef OS390
#include <jni_convert.h>
#endif

#if	defined __ILEC400__ && !defined AS400
#define	AS400
#endif

#include "pdflib.h"

/* figure out whether or not we're running on an EBCDIC-based machine */
#define	ASCII_A			0x41
#define PLATFORM_A		'A'
#define EBCDIC_A		0xC1

#if (ASCII_A != PLATFORM_A && EBCDIC_A == PLATFORM_A)
#define PDFLIB_EBCDIC
#endif

#ifndef OS390

#ifndef int2ll
#define int2ll(a)	((jlong) (a))
#endif

#ifndef ll2int
#define ll2int(a)	((int) (a))
#endif

#ifndef tofloat
#define tofloat(a)	((jfloat) (a))
#endif

#ifndef flt2nat
#define	flt2nat(a)	((float) (a))
#endif

#endif /* OS390 */


#define NULL_INT		NULL
#define PDF_ENV			jlong
#define PDF_ENV2PTR(env)	*((PDF **) &(env))



/* Compilers which are not strictly ANSI conforming can set PDF_VOLATILE
 * to an empty value.
 */
#ifndef PDF_VOLATILE
#define PDF_VOLATILE    volatile
#endif

/* thread-specific data */
typedef struct {
    jint	jdkversion;
    jmethodID	MID_String_getBytes;	/* cached method identifier */
} pdf_wrapper_data;

/* This is used in the wrapper functions for thread-safe exception handling */
#define EXC_STUFF pdf_wrapper_data * PDF_VOLATILE ex

/* Exception handling */

#define PDF_ISNULL(j)	(j == (PDF_ENV) 0)


#define PDF_JAVA_SANITY_CHECK_VOID(j) 				\
    if (PDF_ISNULL(j)) {					\
	jthrow(jenv, "Must not call any PDFlib method after delete()", \
		0, "pdflib_java.c");				\
	return;							\
    }

#define PDF_JAVA_SANITY_CHECK(j) 				\
    if (PDF_ISNULL(j)) {					\
	jthrow(jenv, "Must not call any PDFlib method after delete()",	\
		0, "pdflib_java.c");				\
	return _jresult;					\
    }

/* {{{ jthrow */
static void
jthrow(JNIEnv *jenv, const char *msg, int errnum, const char *apiname)
{
    jclass PDFlibException;
    jstring jmsg, japiname;
    jmethodID cid;
    jthrowable e;
    char *classstring;

#ifndef PDFLIB_EBCDIC
    classstring = "com/pdflib/PDFlibException";
    jmsg = (*jenv)->NewStringUTF(jenv, msg);
    japiname = (*jenv)->NewStringUTF(jenv, apiname);
#endif /* PDFLIB_EBCDIC */

    PDFlibException = (*jenv)->FindClass(jenv, classstring);
    if (PDFlibException == NULL_INT) {
        return;  /* Exception thrown */
    }

    /* Get method ID for PDFlibException(String, int, String) constructor */
#ifndef PDFLIB_EBCDIC
    cid = (*jenv)->GetMethodID(jenv, PDFlibException, "<init>",
        "(Ljava/lang/String;ILjava/lang/String;)V");
#endif /* PDFLIB_EBCDIC */
    if (cid == NULL_INT) {
        return;  /* Exception thrown */
    }

    e = (*jenv)->NewObject(jenv, PDFlibException, cid, jmsg, errnum, japiname);
    (*jenv)->Throw(jenv, e);
}
/* }}} */

#define pdf_catch	PDF_CATCH(p) { 					\
		    jthrow(jenv, PDF_get_errmsg(p), PDF_get_errnum(p), \
		                            PDF_get_apiname(p)); \
		}

/* {{{ GetStringPDFChars
 * Single-byte sequence for file names e.g.: pick only low bytes from Unicode
 * and end with null byte.
 */

static char *
GetStringPDFChars(PDF *p, JNIEnv *jenv, jstring string)
{
    const jchar *unistring;
    char *result;
    size_t i, len;

    if (!string)
	return NULL;

    len = (size_t) (*jenv)->GetStringLength(jenv, string);

    if (len == (size_t) 0)
	return NULL;

    unistring = (*jenv)->GetStringChars(jenv, string, NULL);

    if (!unistring) {
	pdf_throw(p, "Java", "GetStringPDFChars",
	    "JNI internal string allocation failed");
    }

    result = (char *) malloc(len + 1);

    if (!result) {
	pdf_throw(p, "Java", "GetStringPDFChars",
	    "JNI internal string allocation failed");
    }

    /* pick the low-order bytes only */
    for (i = 0; i < len; i++) {
        if (unistring[i] > 0xFF) {
	    pdf_throw(p, "Java", "GetStringPDFChars",
                    "PDFlib supports only Latin-1 character strings");
        }
	result[i] = (char) unistring[i];
    }

    result[i] = '\0';	/* NULL-terminate */

    (*jenv)->ReleaseStringChars(jenv, string, unistring);


    return result;
}
/* }}} */

#define ReleaseStringPDFChars(jenv, string, chars) \
    if (chars) free(chars);

#define ReleaseStringJavaChars(jenv, string, chars) \
    if (chars) (*jenv)->ReleaseStringChars(jenv, string, (jchar *) chars);

/* {{{ GetStringUnicodePDFChars
 * Unicode strings (page description text or hyper text strings)
 */
static const jchar *
GetStringUnicodePDFChars(PDF *p, JNIEnv *jenv, jstring string, int *lenP)
{
    const jchar *unistring;
    size_t len;

    *lenP = 0;

    if (!string)
        return NULL;

    len = (size_t) (*jenv)->GetStringLength(jenv, string);

    if (len == (size_t) 0)
        return NULL;

    unistring = (*jenv)->GetStringChars(jenv, string, NULL);

    if (!unistring) {
	pdf_throw(p, "Java", "GetStringUnicodePDFChars",
            "JNI internal string allocation failed");
    }

    *lenP = (int) (2 * len);

    return unistring;
}
/* }}} */

/* {{{ GetStringPDFChars
 * Converts a C string to a Java string for
 */

/* The Unicode byte order mark (BOM) byte parts */
#define PDF_BOM0                0376            /* '\xFE' */
#define PDF_BOM1                0377            /* '\xFF' */

static jstring
GetNewStringUTF(JNIEnv *jenv, char *cstring)
{
    jstring _jresult = 0;
    char *_result = cstring;


    _jresult = (jstring)(*jenv)->NewStringUTF(jenv, _result);


    return _jresult;
}
/* }}} */


/* export the PDFlib routines to the shared library */
#ifdef __MWERKS__
#pragma export on
#endif

/* p_annots.c */

/* {{{ PDF_add_launchlink */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1add_1launchlink(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jllx, jfloat jlly, jfloat jurx, jfloat jury,
    jstring jfilename)
{
    PDF *p;
    float llx;
    float lly;
    float urx;
    float ury;
    char * PDF_VOLATILE filename = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        llx = flt2nat(jllx);
        lly = flt2nat(jlly);
        urx = flt2nat(jurx);
        ury = flt2nat(jury);
	filename = (char *)GetStringPDFChars(p, jenv, jfilename);

	PDF_add_launchlink(p, llx, lly, urx, ury, filename);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfilename, filename);
}
/* }}} */

/* {{{ PDF_add_locallink */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1add_1locallink(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jllx, jfloat jlly, jfloat jurx, jfloat jury,
    jint jpage, jstring joptlist)
{
    PDF *p;
    float llx;
    float lly;
    float urx;
    float ury;
    int page;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        llx = flt2nat(jllx);
        lly = flt2nat(jlly);
        urx = flt2nat(jurx);
        ury = flt2nat(jury);
	page = (int )jpage;
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

        PDF_add_locallink(p, llx, lly, urx, ury, page, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_add_note */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1add_1note(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jllx, jfloat jlly, jfloat jurx, jfloat jury,
    jstring jcontents, jstring jtitle, jstring jicon, jint jopen)
{
    PDF *p;
    float llx;
    float lly;
    float urx;
    float ury;
    char * PDF_VOLATILE contents = NULL;
    char * PDF_VOLATILE title = NULL;
    char * PDF_VOLATILE icon = NULL;
    int open;
    int clen, tlen;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        llx = flt2nat(jllx);
        lly = flt2nat(jlly);
        urx = flt2nat(jurx);
        ury = flt2nat(jury);
	contents = (char *)GetStringUnicodePDFChars(p, jenv, jcontents, &clen);
	title = (char *)GetStringUnicodePDFChars(p, jenv, jtitle, &tlen);
        icon = (char *)GetStringPDFChars(p, jenv, jicon);
	open = (int )jopen;

	PDF_add_note2(p, llx, lly, urx, ury, contents, clen, title, tlen,
            icon, open);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jcontents, contents);
    ReleaseStringJavaChars(jenv, jtitle, title);
    ReleaseStringPDFChars(jenv, jicon, icon);
}
/* }}} */

/* {{{ PDF_add_pdflink */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1add_1pdflink(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jllx, jfloat jlly, jfloat jurx, jfloat jury,
    jstring jfilename, jint jpage, jstring joptlist)
{
    PDF *p;
    float llx;
    float lly;
    float urx;
    float ury;
    char * PDF_VOLATILE filename = NULL;
    int page;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	llx = flt2nat(jllx);
	lly = flt2nat(jlly);
	urx = flt2nat(jurx);
	ury = flt2nat(jury);
        filename = (char *)GetStringPDFChars(p, jenv, jfilename);
	page = (int )jpage;
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	PDF_add_pdflink(p, llx, lly, urx, ury, filename, page, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfilename, filename);
    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_add_weblink */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1add_1weblink(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jllx, jfloat jlly, jfloat jurx, jfloat jury,
    jstring jurl)
{
    PDF *p;
    float llx;
    float lly;
    float urx;
    float ury;
    char *PDF_VOLATILE url = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        llx = flt2nat(jllx);
        lly = flt2nat(jlly);
        urx = flt2nat(jurx);
        ury = flt2nat(jury);
        url = (char *)GetStringPDFChars(p, jenv, jurl);

	PDF_add_weblink(p, llx, lly, urx, ury, url);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jurl, url);
}
/* }}} */

/* {{{ PDF_attach_file */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1attach_1file(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jllx, jfloat jlly, jfloat jurx, jfloat jury,
    jstring jfilename, jstring jdescription, jstring jauthor,
    jstring jmimetype, jstring jicon)
{
    PDF *p;
    float llx;
    float lly;
    float urx;
    float ury;
    char * PDF_VOLATILE filename = NULL;
    char * PDF_VOLATILE description = NULL;
    char * PDF_VOLATILE author = NULL;
    char * PDF_VOLATILE mimetype = NULL;
    char * PDF_VOLATILE icon = NULL;
    int dlen, alen;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        llx = flt2nat(jllx);
        lly = flt2nat(jlly);
        urx = flt2nat(jurx);
        ury = flt2nat(jury);
        filename = (char *)GetStringPDFChars(p, jenv, jfilename);
        description =
	    (char *)GetStringUnicodePDFChars(p, jenv, jdescription, &dlen);
        author = (char *)GetStringUnicodePDFChars(p, jenv, jauthor, &alen);
        mimetype = (char *)GetStringPDFChars(p, jenv, jmimetype);
        icon = (char *)GetStringPDFChars(p, jenv, jicon);

	PDF_attach_file2(p, llx, lly, urx, ury, filename, 0,
	     description, dlen, author, alen, mimetype, icon);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfilename, filename);
    ReleaseStringJavaChars(jenv, jdescription, description);
    ReleaseStringJavaChars(jenv, jauthor, author);
    ReleaseStringPDFChars(jenv, jmimetype, mimetype);
    ReleaseStringPDFChars(jenv, jicon, icon);
}
/* }}} */

/* {{{ PDF_set_border_color */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1set_1border_1color(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jred, jfloat jgreen, jfloat jblue)
{
    PDF *p;
    float red;
    float green;
    float blue;

    PDF_JAVA_SANITY_CHECK_VOID(jp);

    p = PDF_ENV2PTR(jp);
    red = flt2nat(jred);
    green = flt2nat(jgreen);
    blue = flt2nat(jblue);

    PDF_TRY(p) {     PDF_set_border_color(p, red, green, blue);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_set_border_dash */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1set_1border_1dash(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jb, jfloat jw)
{
    PDF *p;
    float b;
    float w;

    PDF_JAVA_SANITY_CHECK_VOID(jp);

    p = PDF_ENV2PTR(jp);
    b = flt2nat(jb);
    w = flt2nat(jw);

    PDF_TRY(p) {     PDF_set_border_dash(p, b, w);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_set_border_style */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1set_1border_1style(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jstyle, jfloat jwidth)
{
    PDF *p;
    char * PDF_VOLATILE style = NULL;
    float width;

    PDF_JAVA_SANITY_CHECK_VOID(jp);

    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        style = (char *)GetStringPDFChars(p, jenv, jstyle);
	width = flt2nat(jwidth);

	PDF_set_border_style(p, style, width);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jstyle, style);
}
/* }}} */

/* p_basic.c */

/* {{{ PDF_boot */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1boot(JNIEnv *jenv, jclass jcls)
{
    /* throws nothing from within the library */
    PDF_boot();
}
/* }}} */

/* {{{ PDF_begin_page */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1begin_1page(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jwidth, jfloat jheight)
{
    PDF *p;
    float width;
    float height;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	width = flt2nat(jwidth);
	height = flt2nat(jheight);
	PDF_begin_page(p, width, height);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_close */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1close(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_close(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_delete */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1delete(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    PDF *p;
    EXC_STUFF;

    /* Different sanity check here since PDF_delete() may be called multiply,
     * e.g., once from the finalizer and once explicitly
     */

    if (PDF_ISNULL(jp))
	return;

    p = PDF_ENV2PTR(jp);


    ex = (pdf_wrapper_data *) (PDF_get_opaque(p));
    free(ex);

    PDF_delete(p);
}
/* }}} */

/* {{{ PDF_end_page */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1end_1page(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_end_page(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_get_apiname */
JNIEXPORT jstring JNICALL
Java_com_pdflib_pdflib_PDF_1get_1apiname(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    jstring PDF_VOLATILE _jresult = 0;
    char * PDF_VOLATILE _result = NULL;
    PDF *p;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	_result = (char *)PDF_get_apiname(p);
    } pdf_catch;

    if(_result)
        _jresult = (jstring)GetNewStringUTF(jenv, _result);
    return _jresult;
}
/* }}} */

/* {{{ PDF_get_buffer */
JNIEXPORT jbyteArray JNICALL
Java_com_pdflib_pdflib_PDF_1get_1buffer(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    jbyteArray PDF_VOLATILE _jresult = 0;
    const unsigned char *buffer;
    PDF *p;
    long size;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	buffer = (const unsigned char *) PDF_get_buffer(p, &size);
	_jresult = (*jenv)->NewByteArray(jenv, (jsize) size);

	if (_jresult == (jbyteArray) NULL) {
	    pdf_throw(p, "Java", "get_buffer",
		"Couldn't allocate PDF output buffer");
	} else {

	    (*jenv)->SetByteArrayRegion(jenv, _jresult,
			0, (jsize) size, (jbyte *) buffer);
	}
    } pdf_catch;

    return _jresult;
}
/* }}} */

/* {{{ PDF_get_errnum */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1get_1errnum(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result = 0;
    PDF *p;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	_result = PDF_get_errnum(p);
    } pdf_catch;

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_get_errmsg */
JNIEXPORT jstring JNICALL
Java_com_pdflib_pdflib_PDF_1get_1errmsg(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    jstring PDF_VOLATILE _jresult = 0;
    char * PDF_VOLATILE _result = NULL;
    PDF *p;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	_result = (char *)PDF_get_errmsg(p);
    } pdf_catch;

    if(_result)
        _jresult = (jstring)GetNewStringUTF(jenv, _result);
    return _jresult;
}
/* }}} */

/* {{{ PDF_new */
JNIEXPORT jlong JNICALL
Java_com_pdflib_pdflib_PDF_1new(JNIEnv *jenv, jclass jcls)
{
    jlong _jresult;
    PDF * PDF_VOLATILE p = (PDF *) 0;
    EXC_STUFF;
    char jdkname[64];
    jclass stringClass;

#ifdef AS400
    int aux = PDF_JAVA_AS400_FIRSTINDEX;
    JavaI2L(aux, _jresult);
#else
    _jresult = int2ll(0);
#endif


    if ((ex = malloc(sizeof(pdf_wrapper_data))) == NULL) {
	jthrow(jenv, "Couldn't initialize PDFlib (malloc)", 0, "pdflib_java.c");
	return _jresult;
    }

    p = (PDF *)PDF_new2(NULL, NULL, NULL, NULL, (void *) ex);
    if (p == (PDF *)0) {
	jthrow(jenv, "Couldn't initialize PDF object due to memory shortage",
	                0, "pdflibp_java.c");
	return _jresult;
    }

    PDF_TRY(p) {
	ex->jdkversion = (*jenv)->GetVersion(jenv);

	if (((*jenv)->ExceptionOccurred(jenv)) != NULL_INT) {
	    (*jenv)->ExceptionDescribe(jenv);
	    return _jresult;
	}

	sprintf(jdkname, "JDK %d.%d",
		(int) ((ex->jdkversion & 0xFF0000) >> 16),
		(int) (ex->jdkversion & 0xFF));
        PDF_set_parameter(p, "binding", jdkname);

        PDF_set_parameter(p, "textformat", "auto2");
        PDF_set_parameter(p, "hypertextformat", "auto2");
        PDF_set_parameter(p, "hypertextencoding", "");

/* "java/lang/String */
#define PDF_java_lang_String \
	"\x6a\x61\x76\x61\x2f\x6c\x61\x6e\x67\x2f\x53\x74\x72\x69\x6e\x67"
#define PDF_getBytes	"\x67\x65\x74\x42\x79\x74\x65\x73"	/* "getBytes" */
#define PDF_sig		"\x28\x29\x5B\x42"			/* "()[B" */

	stringClass = (*jenv)->FindClass(jenv, PDF_java_lang_String);

	if (stringClass == NULL_INT) {
	    (*jenv)->ExceptionDescribe(jenv);
	    jthrow(jenv,
		"Couldn't initialize PDFlib (FindClass)", 0, "pdflib_java.c");
#ifdef AS400
	    JavaI2L(0, _jresult);
#else
	    _jresult = int2ll(0);
#endif
	    return _jresult;
	}

	ex->MID_String_getBytes =
	    (*jenv)->GetMethodID(jenv, stringClass, PDF_getBytes, PDF_sig);

	if (ex->MID_String_getBytes == NULL_INT) {
	    (*jenv)->ExceptionDescribe(jenv);
	    jthrow(jenv,
		"Couldn't initialize PDFlib (GetMethodID)", 0, "pdflib_java.c");
	    return _jresult;
	}

    } pdf_catch;


#ifdef AS400
    JavaI2L(aux, _jresult);
#else
    *(PDF **)&_jresult = p;
#endif

    return _jresult;
}
/* }}} */

/* {{{ PDF_open_file */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1open_1file(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfilename)
{
    jint _jresult = -1;
    int PDF_VOLATILE _result = -1;
    PDF *p;
    char * PDF_VOLATILE filename = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        filename = (char *)GetStringPDFChars(p, jenv, jfilename);
	_result = (int )PDF_open_file(p, filename);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfilename, filename);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_shutdown */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1shutdown(JNIEnv *jenv, jclass jcls)
{
    /* throws nothing */
    PDF_shutdown();
}
/* }}} */

/* p_block.c */

/* {{{ PDF_fill_imageblock */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1fill_1imageblock(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jpage, jstring jblockname, jint jimage, jstring joptlist)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    char * PDF_VOLATILE blockname = NULL;
    char * PDF_VOLATILE optlist = NULL;
    int page = (int) jpage;
    int image = (int) jimage;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        blockname = (char *)GetStringPDFChars(p, jenv, jblockname);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);
	_result = (int )PDF_fill_imageblock(p, page, blockname, image, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jblockname, blockname);
    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_fill_pdfblock */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1fill_1pdfblock(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jpage, jstring jblockname, jint jcontents, jstring joptlist)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    char * PDF_VOLATILE blockname = NULL;
    char * PDF_VOLATILE optlist = NULL;
    int page = (int) jpage;
    int contents = (int) jcontents;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        blockname = (char *)GetStringPDFChars(p, jenv, jblockname);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);
	_result =
	    (int )PDF_fill_pdfblock(p, page, blockname, contents, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jblockname, blockname);
    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_fill_textblock */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1fill_1textblock(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jpage, jstring jblockname, jstring jtext, jstring joptlist)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    int page = (int) jpage;
    char * PDF_VOLATILE blockname = NULL;
    char * PDF_VOLATILE text = NULL;
    char * PDF_VOLATILE optlist = NULL;
    int tlen;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        blockname = (char *)GetStringPDFChars(p, jenv, jblockname);
	text = (char *)GetStringUnicodePDFChars(p, jenv, jtext, &tlen);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);
	_result = (int )PDF_fill_textblock(p, page, blockname,
			    text, tlen, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jblockname, blockname);
    ReleaseStringJavaChars(jenv, jtext, text);
    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* p_color.c */

/* {{{ PDF_makespotcolor */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1makespotcolor(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jspotname)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    char * PDF_VOLATILE spotname = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        spotname = (char *)GetStringPDFChars(p, jenv, jspotname);
	_result = (int )PDF_makespotcolor(p, spotname, 0);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jspotname, spotname);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_setcolor */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setcolor(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfstype, jstring jcolorspace, jfloat jc1, jfloat jc2,
    jfloat jc3, jfloat jc4)
{
    PDF *p;
    char * PDF_VOLATILE fstype = NULL;
    char * PDF_VOLATILE colorspace = NULL;
    float c1;
    float c2;
    float c3;
    float c4;

    PDF_JAVA_SANITY_CHECK_VOID(jp);

    p = PDF_ENV2PTR(jp);
    c1 = flt2nat(jc1);
    c2 = flt2nat(jc2);
    c3 = flt2nat(jc3);
    c4 = flt2nat(jc4);

    PDF_TRY(p) {
        fstype = (char *)GetStringPDFChars(p, jenv, jfstype);
        colorspace = (char *)GetStringPDFChars(p, jenv, jcolorspace);
	PDF_setcolor(p, fstype, colorspace, c1, c2, c3, c4);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfstype, fstype);
    ReleaseStringPDFChars(jenv, jcolorspace, colorspace);
}
/* }}} */

/* p_draw.c */

/* {{{ PDF_arc */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1arc(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jx, jfloat jy, jfloat jr, jfloat jalpha,
    jfloat jbeta)
{
    PDF *p;
    float x;
    float y;
    float r;
    float alpha;
    float beta;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	x = flt2nat(jx);
	y = flt2nat(jy);
	r = flt2nat(jr);
	alpha = flt2nat(jalpha);
	beta = flt2nat(jbeta);

	PDF_arc(p, x, y, r, alpha, beta);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_arcn */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1arcn(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jx, jfloat jy, jfloat jr, jfloat jalpha,
    jfloat jbeta)
{
    PDF *p;
    float x;
    float y;
    float r;
    float alpha;
    float beta;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	x = flt2nat(jx);
	y = flt2nat(jy);
	r = flt2nat(jr);
	alpha = flt2nat(jalpha);
	beta = flt2nat(jbeta);

	PDF_arcn(p, x, y, r, alpha, beta);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_circle */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1circle(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jx, jfloat jy, jfloat jr)
{
    PDF *p;
    float x;
    float y;
    float r;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	x = flt2nat(jx);
	y = flt2nat(jy);
	r = flt2nat(jr);
	PDF_circle(p,x,y,r);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_clip */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1clip(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_clip(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_closepath */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1closepath(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_closepath(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_closepath_fill_stroke */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1closepath_1fill_1stroke(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_closepath_fill_stroke(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_closepath_stroke */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1closepath_1stroke(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_closepath_stroke(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_curveto */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1curveto(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jx1, jfloat jy1, jfloat jx2, jfloat jy2,
    jfloat jx3, jfloat jy3)
{
    PDF *p;
    float x1;
    float y1;
    float x2;
    float y2;
    float x3;
    float y3;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	x1 = flt2nat(jx1);
	y1 = flt2nat(jy1);
	x2 = flt2nat(jx2);
	y2 = flt2nat(jy2);
	x3 = flt2nat(jx3);
	y3 = flt2nat(jy3);

	PDF_curveto(p, x1, y1, x2, y2, x3, y3);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_endpath */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1endpath(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_endpath(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_fill */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1fill(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_fill(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_fill_stroke */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1fill_1stroke(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_fill_stroke(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_lineto */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1lineto(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jx, jfloat jy)
{
    PDF *p;
    float x;
    float y;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	x = flt2nat(jx);
	y = flt2nat(jy);

	PDF_lineto(p, x, y);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_moveto */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1moveto(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jx, jfloat jy)
{
    PDF *p;
    float x;
    float y;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	x = flt2nat(jx);
	y = flt2nat(jy);

	PDF_moveto(p, x, y);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_rect */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1rect(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jx, jfloat jy, jfloat jwidth, jfloat jheight)
{
    PDF *p;
    float x;
    float y;
    float width;
    float height;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);


    PDF_TRY(p) {
	x = flt2nat(jx);
	y = flt2nat(jy);
	width = flt2nat(jwidth);
	height = flt2nat(jheight);

	PDF_rect(p, x, y, width, height);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_stroke */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1stroke(JNIEnv *jenv, jclass jcls, PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_stroke(p);
    } pdf_catch;
}
/* }}} */

/* p_encoding.c */

/* {{{ PDF_encoding_set_char */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1encoding_1set_1char(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jencoding, jint jslot, jstring jglyphname, jint juv)
{
    PDF *p;
    char * PDF_VOLATILE encoding = NULL;
    int slot;
    char * PDF_VOLATILE glyphname = NULL;
    int uv;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        encoding = (char *)GetStringPDFChars(p, jenv, jencoding);
	slot = (int)jslot;
        glyphname = (char *)GetStringPDFChars(p, jenv, jglyphname);
	uv = (int)juv;

	PDF_encoding_set_char(p, encoding, slot, glyphname, uv);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jencoding, encoding);
    ReleaseStringPDFChars(jenv, jglyphname, glyphname);
}
/* }}} */

/* p_font.c */

/* {{{ PDF_findfont */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1findfont(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfontname, jstring jencoding, jint jembed)
{
    jint _jresult = -1;
    int PDF_VOLATILE _result = -1;
    PDF *p;
    char * PDF_VOLATILE fontname = NULL;
    char * PDF_VOLATILE encoding = NULL;
    int embed;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        fontname = (char *)GetStringPDFChars(p, jenv, jfontname);
        encoding = (char *)GetStringPDFChars(p, jenv, jencoding);
	embed = (int )jembed;

	_result = (int )PDF_findfont(p, fontname, encoding, embed);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfontname, fontname);
    ReleaseStringPDFChars(jenv, jencoding, encoding);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_load_font */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1load_1font(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfontname, jstring jencoding, jstring joptlist)
{
    jint _jresult = -1;
    int PDF_VOLATILE _result = -1;
    PDF *p;
    char * PDF_VOLATILE fontname = NULL;
    char * PDF_VOLATILE encoding = NULL;
    char * PDF_VOLATILE optlist = NULL;
    int len1;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        fontname = (char *)GetStringUnicodePDFChars(p, jenv, jfontname, &len1);
        encoding = (char *)GetStringPDFChars(p, jenv, jencoding);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	_result = (int )PDF_load_font(p,fontname, len1, encoding, optlist);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jfontname, fontname);
    ReleaseStringPDFChars(jenv, jencoding, encoding);
    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_setfont */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setfont(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jfont, jfloat jfontsize)
{
    PDF *p;
    int font;
    float fontsize;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	font = (int )jfont;
	fontsize = flt2nat(jfontsize);
	PDF_setfont(p, font, fontsize);
    } pdf_catch;
}
/* }}} */

/* p_gstate.c */

/* {{{ PDF_concat */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1concat(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat a, jfloat b, jfloat c, jfloat d, jfloat e, jfloat f)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	PDF_concat(p, flt2nat(a), flt2nat(b), flt2nat(c), flt2nat(d),
                                flt2nat(e), flt2nat(f));
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_initgraphics */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1initgraphics(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_initgraphics(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_restore */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1restore(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_restore(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_rotate */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1rotate(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jphi)
{
    PDF *p;
    float phi;

    PDF_JAVA_SANITY_CHECK_VOID(jp);

    p = PDF_ENV2PTR(jp);
    phi = flt2nat(jphi);

    PDF_TRY(p) {     PDF_rotate(p, phi);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_save */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1save(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_save(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_scale */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1scale(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jsx, jfloat jsy)
{
    PDF *p;
    float sx;
    float sy;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	sx = flt2nat(jsx);
	sy = flt2nat(jsy);

	PDF_scale(p, sx, sy);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_setdash */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setdash(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jb, jfloat jw)
{
    PDF *p;
    float b;
    float w;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	b = flt2nat(jb);
	w = flt2nat(jw);

	PDF_setdash(p, b, w);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_setdashpattern */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setdashpattern(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring joptlist)
{
    PDF *p;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	PDF_setdashpattern(p, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_setflat */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setflat(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jflatness)
{
    PDF *p;
    float flatness;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	flatness = flt2nat(jflatness);

	PDF_setflat(p, flatness);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_setlinecap */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setlinecap(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jlinecap)
{
    PDF *p;
    int linecap;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	linecap = (int )jlinecap;

	PDF_setlinecap(p, linecap);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_setlinejoin */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setlinejoin(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jlinejoin)
{
    PDF *p;
    int linejoin;

    PDF_JAVA_SANITY_CHECK_VOID(jp);

    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	linejoin = (int )jlinejoin;

	PDF_setlinejoin(p, linejoin);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_setlinewidth */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setlinewidth(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jwidth)
{
    PDF *p;
    float width;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	width = flt2nat(jwidth);

	PDF_setlinewidth(p, width);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_setmatrix */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setmatrix(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat ja, jfloat jb, jfloat jc, jfloat jd,
    jfloat je, jfloat jf)
{
    PDF *p;
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	a = flt2nat(ja);
	b = flt2nat(jb);
	c = flt2nat(jc);
	d = flt2nat(jd);
	e = flt2nat(je);
	f = flt2nat(jf);

	PDF_setmatrix(p, a, b, c, d, e, f);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_setmiterlimit */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setmiterlimit(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jmiter)
{
    PDF *p;
    float miter;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	miter = flt2nat(jmiter);

	PDF_setmiterlimit(p, miter);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_setpolydash */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1setpolydash(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloatArray jdasharray)
{
    PDF *p;
    float carray[MAX_DASH_LENGTH];
    jfloat* javaarray;
    int i;
    jsize PDF_VOLATILE length;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	length = (*jenv)->GetArrayLength(jenv, jdasharray);
	if (length > MAX_DASH_LENGTH)
	    length = MAX_DASH_LENGTH;

	javaarray = (*jenv)->GetFloatArrayElements(jenv, jdasharray, 0);

	for(i=0; i < length; i++)
	    carray[i] = flt2nat(javaarray[i]);

	PDF_setpolydash(p, carray, length);
	(*jenv)->ReleaseFloatArrayElements(jenv, jdasharray, javaarray, 0);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_skew */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1skew(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jalpha, jfloat jbeta)
{
    PDF *p;
    float alpha;
    float beta;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	alpha = flt2nat(jalpha);
	beta = flt2nat(jbeta);

	PDF_skew(p, alpha, beta);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_translate */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1translate(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jtx, jfloat jty)
{
    PDF *p;
    float tx;
    float ty;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	tx = flt2nat(jtx);
	ty = flt2nat(jty);

	PDF_translate(p, tx, ty);
    } pdf_catch;
}
/* }}} */

/* p_hyper.c */

/* {{{ PDF_add_bookmark */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1add_1bookmark(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jtext, jint jparent, jint jopen)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE text = NULL;
    int parent;
    int open;
    int len1;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	text = (char *)GetStringUnicodePDFChars(p, jenv, jtext, &len1);
	parent = (int )jparent;
	open = (int )jopen;

	_result = (int )PDF_add_bookmark2(p, text, len1, parent, open);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jtext, text);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_add_nameddest */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1add_1nameddest(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jname, jstring joptlist)
{
    PDF *p;
    char * PDF_VOLATILE name = NULL;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        name = (char *)GetStringPDFChars(p, jenv, jname);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	PDF_add_nameddest(p, name, 0, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jname, name);
    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_set_info */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1set_1info(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jkey, jstring jvaluex)
{
    PDF *p;
    char * PDF_VOLATILE key = NULL;
    char * PDF_VOLATILE value = NULL;
    int len2;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        key = (char *)GetStringPDFChars(p, jenv, jkey);
	value = (char *)GetStringUnicodePDFChars(p, jenv, jvaluex, &len2);

	PDF_set_info2(p, key, value, len2);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jkey, key);
    ReleaseStringJavaChars(jenv, jvaluex, value);
}
/* }}} */

/* p_icc.c */

/* {{{ PDF_load_iccprofile */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1load_1iccprofile(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jprofilename, jstring joptlist)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE profilename = NULL;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        profilename = (char *)GetStringPDFChars(p, jenv, jprofilename);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	_result = (int )PDF_load_iccprofile(p, profilename, 0, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jprofilename, profilename);
    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* p_image.c */

/* {{{ PDF_add_thumbnail */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1add_1thumbnail(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jimage)
{
    PDF *p;
    int image;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	image = (int )jimage;

	PDF_add_thumbnail(p, image);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_close_image */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1close_1image(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jimage)
{
    PDF *p;
    int image;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	image = (int )jimage;

	PDF_close_image(p, image);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_fit_image */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1fit_1image(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jimage, jfloat jx, jfloat jy, jstring joptlist)
{
    PDF *p;
    int image;
    float x;
    float y;
    char *  PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        optlist = (char *) GetStringPDFChars(p, jenv, joptlist);
	image = (int)jimage;
	x = flt2nat(jx);
	y = flt2nat(jy);
	PDF_fit_image(p, image, x, y, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_load_image */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1load_1image(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jimagetype, jstring jfilename, jstring joptlist)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE imagetype = NULL;
    char * PDF_VOLATILE filename = NULL;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        imagetype = (char *)GetStringPDFChars(p, jenv, jimagetype);
        filename = (char *)GetStringPDFChars(p, jenv, jfilename);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	_result = (int )PDF_load_image(p, imagetype, filename, 0, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jimagetype, imagetype);
    ReleaseStringPDFChars(jenv, jfilename, filename);
    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_open_CCITT */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1open_1CCITT(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfilename, jint jwidth, jint jheight, jint jBitReverse,
    jint jK, jint jBlackIs1)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE filename = NULL;
    int width;
    int height;
    int BitReverse;
    int K;
    int BlackIs1;

    PDF_JAVA_SANITY_CHECK(jp);

    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        filename = (char *)GetStringPDFChars(p, jenv, jfilename);
	width = (int )jwidth;
	height = (int )jheight;
	BitReverse = (int )jBitReverse;
	K = (int )jK;
	BlackIs1 = (int )jBlackIs1;

	_result = (int )PDF_open_CCITT(p, filename, width, height, BitReverse,
					    K, BlackIs1);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfilename, filename);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_open_image */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1open_1image(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jimagetype, jstring jsource, jbyteArray jdata,
    jlong jlength, jint jwidth, jint jheight, jint jcomponents,
    jint jbpc, jstring jparams)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE imagetype = NULL;
    char * PDF_VOLATILE source = NULL;
    char * PDF_VOLATILE data = NULL;
#ifdef OS390
    int ilength;
#endif
    long length;
    int width;
    int height;
    int components;
    int bpc;
    char * PDF_VOLATILE params = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	data = (char *)
	    (*jenv)->GetByteArrayElements(jenv, jdata, (jboolean *) NULL);
        imagetype = (char *)GetStringPDFChars(p, jenv, jimagetype);
        source = (char *)GetStringPDFChars(p, jenv, jsource);

#ifdef AS400
	/* deal with the AS/400's idea of a jlong... */
	JavaL2I(jlength, length);
#elif OS390
	ilength = ll2int(jlength);
	length = (long )ilength;
#else
	length = (long )jlength;
#endif

	width = (int )jwidth;
	height = (int )jheight;
	components = (int )jcomponents;
	bpc = (int )jbpc;
        params = (char *)GetStringPDFChars(p, jenv, jparams);

	_result = (int )PDF_open_image(p, imagetype, source, data, length,
		     width, height, components, bpc, params);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jimagetype, imagetype);
    ReleaseStringPDFChars(jenv, jsource, source);
    (*jenv)->ReleaseByteArrayElements(jenv, jdata, (jbyte*) data, JNI_ABORT);
    ReleaseStringPDFChars(jenv, jparams, params);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_open_image_file */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1open_1image_1file(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jimagetype, jstring jfilename, jstring jstringparam,
    jint jintparam)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE imagetype = NULL;
    char * PDF_VOLATILE filename = NULL;
    char * PDF_VOLATILE stringparam = NULL;
    int intparam;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        imagetype = (char *)GetStringPDFChars(p, jenv, jimagetype);
        filename = (char *)GetStringPDFChars(p, jenv, jfilename);
        stringparam = (char *)GetStringPDFChars(p, jenv, jstringparam);
	intparam = (int )jintparam;

	_result = (int )PDF_open_image_file(p, imagetype, filename,
				    stringparam, intparam);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jimagetype, imagetype);
    ReleaseStringPDFChars(jenv, jfilename, filename);
    ReleaseStringPDFChars(jenv, jstringparam, stringparam);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_place_image */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1place_1image(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jimage, jfloat jx, jfloat jy, jfloat jscale)
{
    PDF *p;
    int image;
    float x;
    float y;
    float scale;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	image = (int )jimage;
	x = flt2nat(jx);
	y = flt2nat(jy);
	scale = flt2nat(jscale);

	PDF_place_image(p, image, x, y, scale);
    } pdf_catch;
}
/* }}} */

/* p_params.c */

/* {{{ PDF_get_parameter */
JNIEXPORT jstring JNICALL
Java_com_pdflib_pdflib_PDF_1get_1parameter(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jkey, jfloat jmodifier)
{
    jstring PDF_VOLATILE _jresult = 0;
    char * PDF_VOLATILE _result = NULL;
    char * PDF_VOLATILE key = NULL;
    float modifier;
    PDF *p;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        key = (char *) GetStringPDFChars(p, jenv, jkey);
	modifier = flt2nat(jmodifier);

	_result = (char *)PDF_get_parameter(p, key, modifier);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jkey, key);

    if(_result)
        _jresult = (jstring)GetNewStringUTF(jenv, _result);
    return _jresult;
}
/* }}} */

/* {{{ PDF_get_value */
JNIEXPORT jfloat JNICALL
Java_com_pdflib_pdflib_PDF_1get_1value(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jkey, jfloat jvaluex)
{
    jfloat _jresult = tofloat(0);
    float PDF_VOLATILE _result;
    PDF *p;
    char *  PDF_VOLATILE key = NULL;
    float value;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        key = (char *) GetStringPDFChars(p, jenv, jkey);
	value = flt2nat(jvaluex);

	_result = (float )PDF_get_value(p, key, value);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jkey, key);

    _jresult = tofloat(_result);
    return _jresult;
}
/* }}} */

/* {{{ PDF_set_parameter */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1set_1parameter(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jkey, jstring jvaluex)
{
    PDF *p;
    char * PDF_VOLATILE key = NULL;
    char * PDF_VOLATILE value = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        key = (char *)GetStringPDFChars(p, jenv, jkey);
        value = (char *)GetStringPDFChars(p, jenv, jvaluex);

        PDF_set_parameter(p, key, value);

    } pdf_catch;

    ReleaseStringPDFChars(jenv, jkey, key);
    ReleaseStringPDFChars(jenv, jvaluex, value);
}
/* }}} */

/* {{{ PDF_set_value */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1set_1value(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jkey, jfloat jvaluex)
{
    PDF *p;
    char *  PDF_VOLATILE key = NULL;
    float value;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        key = (char *) GetStringPDFChars(p, jenv, jkey);
	value = flt2nat(jvaluex);

	PDF_set_value(p, key, value);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jkey, key);
}
/* }}} */

/* p_pattern.c */

/* {{{ PDF_begin_pattern */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1begin_1pattern(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jwidth, jfloat jheight, jfloat jxstep, jfloat jystep,
    jint jpainttype)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    float width;
    float height;
    float xstep;
    float ystep;
    int painttype;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	width = flt2nat(jwidth);
	height = flt2nat(jheight);
	xstep = flt2nat(jxstep);
	ystep = flt2nat(jystep);
	painttype = (int)jpainttype;

	_result = (int )PDF_begin_pattern(p, width, height, xstep,
					ystep, painttype);
    } pdf_catch;

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_end_pattern */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1end_1pattern(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);

    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_end_pattern(p);
    } pdf_catch;
}
/* }}} */

/* p_pdi.c */
/* {{{ PDF_close_pdi */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1close_1pdi(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jdoc)
{
    PDF *p;
    int doc;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	doc = (int )jdoc;

	PDF_close_pdi(p, doc);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_close_pdi_page */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1close_1pdi_1page(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jpage)
{
    PDF *p;
    int page;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	page = (int )jpage;

	PDF_close_pdi_page(p, page);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_fit_pdi_page */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1fit_1pdi_1page(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jpage, jfloat jx, jfloat jy, jstring joptlist)
{
    PDF *p;
    int page;
    float x;
    float y;
    char *  PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	page = (int) jpage;
	x = flt2nat(jx);
	y = flt2nat(jy);
        optlist = (char *) GetStringPDFChars(p, jenv, joptlist);

	PDF_fit_pdi_page(p, page, x, y, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_get_pdi_parameter */
JNIEXPORT jstring JNICALL
Java_com_pdflib_pdflib_PDF_1get_1pdi_1parameter(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jkey, jint jdoc, jint jpage, jint jreserved)
{
    PDF *p;
    jstring PDF_VOLATILE _jresult = 0;
    char * PDF_VOLATILE _result = NULL;
    char * PDF_VOLATILE key = NULL;
    int doc;
    int page;
    int reserved;
    int len;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	doc = (int )jdoc;
	page = (int )jpage;
	reserved = (int )jreserved;
        key = (char *)GetStringPDFChars(p, jenv, jkey);

	_result = (char *)
	    PDF_get_pdi_parameter(p, key, doc, page, reserved, &len);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jkey, key);

    /* TODO: return a Unicode string based on _result and len */
    if(_result != NULL)
        _jresult = (jstring)GetNewStringUTF(jenv, _result);
    return _jresult;
}
/* }}} */

/* {{{ PDF_get_pdi_value */
JNIEXPORT jfloat JNICALL
Java_com_pdflib_pdflib_PDF_1get_1pdi_1value(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jkey, jint jdoc, jint jpage, jint jreserved)
{
    PDF *p;
    jfloat _jresult = tofloat(0);
    jfloat PDF_VOLATILE _result = tofloat(0);
    char * PDF_VOLATILE key = NULL;
    int doc;
    int page;
    int reserved;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	doc = (int )jdoc;
	page = (int )jpage;
	reserved = (int )jreserved;
        key = (char *)GetStringPDFChars(p, jenv, jkey);

	_result = tofloat(PDF_get_pdi_value(p, key, doc, page, reserved));
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jkey, key);

    _jresult = _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_open_pdi */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1open_1pdi(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfilename, jstring joptlist, jint jreserved)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    char * PDF_VOLATILE filename = NULL;
    char * PDF_VOLATILE optlist = NULL;
    int reserved;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        filename = (char *)GetStringPDFChars(p, jenv, jfilename);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);
	reserved = (int )jreserved;

	_result = (int )PDF_open_pdi(p, filename, optlist, reserved);

    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfilename, filename);
    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_open_pdi_page */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1open_1pdi_1page(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jdoc, jint jpagenumber, jstring joptlist)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    int doc;
    int pagenumber;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	doc = (int )jdoc;
	pagenumber = (int )jpagenumber;
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	_result = (int )PDF_open_pdi_page(p, doc, pagenumber, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_place_pdi_page */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1place_1pdi_1page(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jpage, jfloat jx, jfloat jy, jfloat jsx, jfloat jsy)
{
    PDF *p;
    int page;
    float x;
    float y;
    float sx;
    float sy;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	page = (int )jpage;
	x = flt2nat(jx);
	y = flt2nat(jy);
	sx = flt2nat(jsx);
	sy = flt2nat(jsy);

	PDF_place_pdi_page(p, page, x, y, sx, sy);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_process_pdi */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1process_1pdi(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jdoc, jint jpage, jstring joptlist)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    int doc;
    int page;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	doc = (int )jdoc;
	page = (int )jpage;
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	_result = (int )PDF_process_pdi(p, doc, page, optlist);

    } pdf_catch;

    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* p_resource.c */

/* {{{ PDF_create_pvf */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1create_1pvf(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfilename, jbyteArray jdata, jstring joptlist)
{
    PDF *p;
    char *  PDF_VOLATILE filename = NULL;
    char *  PDF_VOLATILE data = NULL;
    char *  PDF_VOLATILE optlist = NULL;
    size_t dlen;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        filename = (char *) GetStringPDFChars(p, jenv, jfilename);
	dlen = (*jenv)->GetArrayLength(jenv, jdata);
	data = (char *)
	    (*jenv)->GetByteArrayElements(jenv, jdata, (jboolean *) NULL);
        optlist = (char *) GetStringPDFChars(p, jenv, joptlist);

	/* TODO: append "copy" to existing optlist, if optlist changes */
	PDF_create_pvf(p, filename, 0, data, dlen, "copy");
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfilename, filename);
    (*jenv)->ReleaseByteArrayElements(jenv, jdata, (jbyte*) data, JNI_ABORT);
    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_delete_pvf */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1delete_1pvf(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfilename)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    char * PDF_VOLATILE filename = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        filename = (char *)GetStringPDFChars(p, jenv, jfilename);

	_result = (int )PDF_delete_pvf(p, filename, 0);

    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfilename, filename);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* p_shading.c */

/* {{{ PDF_shading */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1shading(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jshtype, jfloat jx0, jfloat jy0, jfloat jx1, jfloat jy1,
    jfloat jc1, jfloat jc2, jfloat jc3, jfloat jc4, jstring joptlist)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    char * PDF_VOLATILE shtype = NULL;
    char * PDF_VOLATILE optlist = NULL;
    float x0, y0, x1, y1, c1, c2, c3, c4;


    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        shtype = (char *)GetStringPDFChars(p, jenv, jshtype);
	x0 = flt2nat(jx0);
	y0 = flt2nat(jy0);
	x1 = flt2nat(jx1);
	y1 = flt2nat(jy1);
	c1 = flt2nat(jc1);
	c2 = flt2nat(jc2);
	c3 = flt2nat(jc3);
	c4 = flt2nat(jc4);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	_result = (int )PDF_shading(p, shtype, x0, y0, x1, y1, c1, c2, c3, c4,
					optlist);

    } pdf_catch;

    ReleaseStringPDFChars(jenv, jshtype, shtype);
    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_shading_pattern */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1shading_1pattern(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jshading, jstring joptlist)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    char * PDF_VOLATILE optlist = NULL;
    int shading;


    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	shading = (int)jshading;
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	_result = (int )PDF_shading_pattern(p, shading, optlist);

    } pdf_catch;

    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_shfill */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1shfill(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jshading)
{
    PDF *p;
    int shading;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	shading = (int )jshading;

	PDF_shfill(p, shading);
    } pdf_catch;
}
/* }}} */

/* p_template.c */

/* {{{ PDF_begin_template */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1begin_1template(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jwidth, jfloat jheight)
{
    PDF *p;
    jint _jresult = 0;
    int PDF_VOLATILE _result = -1;
    float width;
    float height;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	width = flt2nat(jwidth);
	height = flt2nat(jheight);

	_result = (int )PDF_begin_template(p, width, height);
    } pdf_catch;

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_end_template */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1end_1template(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_end_template(p);
    } pdf_catch;
}
/* }}} */

/* p_text.c */

/* {{{ PDF_continue_text */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1continue_1text(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jtext)
{
    PDF *p;
    char * PDF_VOLATILE text = NULL;
    int len1;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	text = (char *)GetStringUnicodePDFChars(p, jenv, jtext, &len1);

	PDF_continue_text2(p, text, len1);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jtext, text);
}
/* }}} */

/* {{{ PDF_fit_textline */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1fit_1textline(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jtext, jfloat jx, jfloat jy, jstring joptlist)
{
    PDF *p;
    char *  PDF_VOLATILE text = NULL;
    float x;
    float y;
    char *  PDF_VOLATILE optlist = NULL;
    int len;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	text = (char *)GetStringUnicodePDFChars(p, jenv, jtext, &len);
	x = flt2nat(jx);
	y = flt2nat(jy);
        optlist = (char *) GetStringPDFChars(p, jenv, joptlist);

	PDF_fit_textline(p, text, len, x, y, optlist);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jtext, text);
    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_set_text_pos */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1set_1text_1pos(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jfloat jx, jfloat jy)
{
    PDF *p;
    float x;
    float y;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	x = flt2nat(jx);
	y = flt2nat(jy);

	PDF_set_text_pos(p, x, y);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_show */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1show(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jtext)
{
    PDF *p;
    char * PDF_VOLATILE text = NULL;
    int len1;

    PDF_JAVA_SANITY_CHECK_VOID(jp);

    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	text = (char *)GetStringUnicodePDFChars(p, jenv, jtext, &len1);

	PDF_show2(p, text, len1);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jtext, text);
}
/* }}} */

/* {{{ PDF_show_boxed */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1show_1boxed(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jtext, jfloat jleft, jfloat jtop, jfloat jwidth,
    jfloat jheight, jstring jhmode, jstring jfeature)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE text = NULL;
    float left;
    float top;
    float width;
    float height;
    char * PDF_VOLATILE hmode = NULL;
    char * PDF_VOLATILE feature = NULL;
    int len1;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        text = (char *)GetStringUnicodePDFChars(p, jenv, jtext, &len1);
	left = flt2nat(jleft);
	top = flt2nat(jtop);
	width = flt2nat(jwidth);
	height = flt2nat(jheight);
        hmode = (char *)GetStringPDFChars(p, jenv, jhmode);
        feature = (char *)GetStringPDFChars(p, jenv, jfeature);

	_result = PDF_show_boxed2(p, text, len1, left, top, width, height,
                                  hmode, feature);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jtext, text);
    ReleaseStringPDFChars(jenv, jhmode, hmode);
    ReleaseStringPDFChars(jenv, jfeature, feature);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_show_xy */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1show_1xy(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jtext, jfloat jx, jfloat jy)
{
    PDF *p;
    char * PDF_VOLATILE text = NULL;
    float x;
    float y;
    int len1;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	text = (char *)GetStringUnicodePDFChars(p, jenv, jtext, &len1);
	x = flt2nat(jx);
	y = flt2nat(jy);

	PDF_show_xy2(p, text, len1,  x, y);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jtext, text);
}
/* }}} */

/* {{{ PDF_stringwidth */
JNIEXPORT jfloat JNICALL
Java_com_pdflib_pdflib_PDF_1stringwidth(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jtext, jint jfont, jfloat jfontsize)
{
    jfloat _jresult = tofloat(0);
    float PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE text = NULL;
    int font;
    float fontsize;
    int len1;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	text = (char *)GetStringUnicodePDFChars(p, jenv, jtext, &len1);
	font = (int )jfont;
	fontsize = flt2nat(jfontsize);

	_result = (float )PDF_stringwidth2(p, text, len1, font, fontsize);
    } pdf_catch;

    ReleaseStringJavaChars(jenv, jtext, text);

    _jresult = tofloat(_result);
    return _jresult;
}
/* }}} */

/* p_type3.c */

/* {{{ PDF_begin_font */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1begin_1font(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jfontname, jfloat ja, jfloat jb, jfloat jc,
    jfloat jd, jfloat je, jfloat jf, jstring joptlist)
{
    PDF *p;
    char * PDF_VOLATILE fontname = NULL;
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        fontname = (char *)GetStringPDFChars(p, jenv, jfontname);
	a = flt2nat(ja);
	b = flt2nat(jb);
	c = flt2nat(jc);
	d = flt2nat(jd);
	e = flt2nat(je);
	f = flt2nat(jf);
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	PDF_begin_font(p, fontname, 0, a, b, c, d, e, f, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jfontname, fontname);
    ReleaseStringPDFChars(jenv, joptlist, optlist);
}
/* }}} */

/* {{{ PDF_begin_glyph */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1begin_1glyph(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring jglyphname, jfloat jwx, jfloat jllx, jfloat jlly,
    jfloat jurx, jfloat jury)
{
    PDF *p;
    char * PDF_VOLATILE glyphname = NULL;
    float wx;
    float llx;
    float lly;
    float urx;
    float ury;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        glyphname = (char *)GetStringPDFChars(p, jenv, jglyphname);
	wx = flt2nat(jwx);
	llx = flt2nat(jllx);
	lly = flt2nat(jlly);
	urx = flt2nat(jurx);
	ury = flt2nat(jury);

	PDF_begin_glyph(p, glyphname, wx, llx, lly, urx, ury);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, jglyphname, glyphname);
}
/* }}} */

/* {{{ PDF_end_font */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1end_1font(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_end_font(p);
    } pdf_catch;
}
/* }}} */

/* {{{ PDF_end_glyph */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1end_1glyph(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp)
{
    PDF *p;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {     PDF_end_glyph(p);
    } pdf_catch;
}
/* }}} */

/* p_xgstate.c */

/* {{{ PDF_create_gstate */
JNIEXPORT jint JNICALL
Java_com_pdflib_pdflib_PDF_1create_1gstate(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jstring joptlist)
{
    jint _jresult = 0;
    int PDF_VOLATILE _result;
    PDF *p;
    char * PDF_VOLATILE optlist = NULL;

    PDF_JAVA_SANITY_CHECK(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
        optlist = (char *)GetStringPDFChars(p, jenv, joptlist);

	_result = PDF_create_gstate(p, optlist);
    } pdf_catch;

    ReleaseStringPDFChars(jenv, joptlist, optlist);

    _jresult = (jint) _result;
    return _jresult;
}
/* }}} */

/* {{{ PDF_set_gstate */
JNIEXPORT void JNICALL
Java_com_pdflib_pdflib_PDF_1set_1gstate(JNIEnv *jenv, jclass jcls,
    PDF_ENV jp, jint jgstate)
{
    PDF *p;
    int gstate;

    PDF_JAVA_SANITY_CHECK_VOID(jp);
    p = PDF_ENV2PTR(jp);

    PDF_TRY(p) {
	gstate = (int)jgstate;

	PDF_set_gstate(p, gstate);
    } pdf_catch;
}
/* }}} */

/*
 * vim600: sw=4 fdm=marker
 */
