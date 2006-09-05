/*
 * transform.c: Implementation of the XSL Transformation 1.0 engine
 *              transform part, i.e. applying a Stylesheet to a document
 *
 * References:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 *   Michael Kay "XSLT Programmer's Reference" pp 637-643
 *   Writing Multiple Output Files
 *
 *   XSLT-1.1 Working Draft
 *   http://www.w3.org/TR/xslt11#multiple-output
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXSLT
#include "libxslt.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/encoding.h>
#include <libxml/xmlerror.h>
#include <libxml/xpath.h>
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include <libxml/HTMLtree.h>
#include <libxml/debugXML.h>
#include <libxml/uri.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "pattern.h"
#include "transform.h"
#include "variables.h"
#include "numbersInternals.h"
#include "namespaces.h"
#include "attributes.h"
#include "templates.h"
#include "imports.h"
#include "keys.h"
#include "documents.h"
#include "extensions.h"
#include "extra.h"
#include "preproc.h"
#include "security.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_EXTRA
#define WITH_XSLT_DEBUG_PROCESS
#endif

#define XSLT_GENERATE_HTML_DOCTYPE
#ifdef XSLT_GENERATE_HTML_DOCTYPE
static int xsltGetHTMLIDs(const xmlChar *version, const xmlChar **publicID,
			  const xmlChar **systemID);
#endif

static void
xsltApplyOneTemplateInt(xsltTransformContextPtr ctxt, xmlNodePtr node,
                     xmlNodePtr list, xsltTemplatePtr templ,
                     xsltStackElemPtr params, int notcur);

int xsltMaxDepth = 5000;

/*
 * Useful macros
 */

#ifndef FALSE
# define FALSE (0 == 1)
# define TRUE (!FALSE)
#endif

#define IS_BLANK_NODE(n)						\
    (((n)->type == XML_TEXT_NODE) && (xsltIsBlank((n)->content)))

/**
 * templPush:
 * @ctxt: the transformation context
 * @value:  the template to push on the stack
 *
 * Push a template on the stack
 *
 * Returns the new index in the stack or 0 in case of error
 */
static int
templPush(xsltTransformContextPtr ctxt, xsltTemplatePtr value)
{
    if (ctxt->templMax == 0) {
        ctxt->templMax = 4;
        ctxt->templTab =
            (xsltTemplatePtr *) xmlMalloc(ctxt->templMax *
                                          sizeof(ctxt->templTab[0]));
        if (ctxt->templTab == NULL) {
            xmlGenericError(xmlGenericErrorContext, "malloc failed !\n");
            return (0);
        }
    }
    if (ctxt->templNr >= ctxt->templMax) {
        ctxt->templMax *= 2;
        ctxt->templTab =
            (xsltTemplatePtr *) xmlRealloc(ctxt->templTab,
                                           ctxt->templMax *
                                           sizeof(ctxt->templTab[0]));
        if (ctxt->templTab == NULL) {
            xmlGenericError(xmlGenericErrorContext, "realloc failed !\n");
            return (0);
        }
    }
    ctxt->templTab[ctxt->templNr] = value;
    ctxt->templ = value;
    return (ctxt->templNr++);
}
/**
 * templPop:
 * @ctxt: the transformation context
 *
 * Pop a template value from the stack
 *
 * Returns the stored template value
 */
static xsltTemplatePtr
templPop(xsltTransformContextPtr ctxt)
{
    xsltTemplatePtr ret;

    if (ctxt->templNr <= 0)
        return (0);
    ctxt->templNr--;
    if (ctxt->templNr > 0)
        ctxt->templ = ctxt->templTab[ctxt->templNr - 1];
    else
        ctxt->templ = (xsltTemplatePtr) 0;
    ret = ctxt->templTab[ctxt->templNr];
    ctxt->templTab[ctxt->templNr] = 0;
    return (ret);
}
/**
 * varsPush:
 * @ctxt: the transformation context
 * @value:  the variable to push on the stack
 *
 * Push a variable on the stack
 *
 * Returns the new index in the stack or 0 in case of error
 */
static int
varsPush(xsltTransformContextPtr ctxt, xsltStackElemPtr value)
{
    if (ctxt->varsMax == 0) {
        ctxt->varsMax = 4;
        ctxt->varsTab =
            (xsltStackElemPtr *) xmlMalloc(ctxt->varsMax *
                                           sizeof(ctxt->varsTab[0]));
        if (ctxt->varsTab == NULL) {
            xmlGenericError(xmlGenericErrorContext, "malloc failed !\n");
            return (0);
        }
    }
    if (ctxt->varsNr >= ctxt->varsMax) {
        ctxt->varsMax *= 2;
        ctxt->varsTab =
            (xsltStackElemPtr *) xmlRealloc(ctxt->varsTab,
                                            ctxt->varsMax *
                                            sizeof(ctxt->varsTab[0]));
        if (ctxt->varsTab == NULL) {
            xmlGenericError(xmlGenericErrorContext, "realloc failed !\n");
            return (0);
        }
    }
    ctxt->varsTab[ctxt->varsNr] = value;
    ctxt->vars = value;
    return (ctxt->varsNr++);
}
/**
 * varsPop:
 * @ctxt: the transformation context
 *
 * Pop a variable value from the stack
 *
 * Returns the stored variable value
 */
static xsltStackElemPtr
varsPop(xsltTransformContextPtr ctxt)
{
    xsltStackElemPtr ret;

    if (ctxt->varsNr <= 0)
        return (0);
    ctxt->varsNr--;
    if (ctxt->varsNr > 0)
        ctxt->vars = ctxt->varsTab[ctxt->varsNr - 1];
    else
        ctxt->vars = (xsltStackElemPtr) 0;
    ret = ctxt->varsTab[ctxt->varsNr];
    ctxt->varsTab[ctxt->varsNr] = 0;
    return (ret);
}
/**
 * profPush:
 * @ctxt: the transformation context
 * @value:  the profiling value to push on the stack
 *
 * Push a profiling value on the stack
 *
 * Returns the new index in the stack or 0 in case of error
 */
static int
profPush(xsltTransformContextPtr ctxt, long value)
{
    if (ctxt->profMax == 0) {
        ctxt->profMax = 4;
        ctxt->profTab =
            (long *) xmlMalloc(ctxt->profMax * sizeof(ctxt->profTab[0]));
        if (ctxt->profTab == NULL) {
            xmlGenericError(xmlGenericErrorContext, "malloc failed !\n");
            return (0);
        }
    }
    if (ctxt->profNr >= ctxt->profMax) {
        ctxt->profMax *= 2;
        ctxt->profTab =
            (long *) xmlRealloc(ctxt->profTab,
                                ctxt->profMax * sizeof(ctxt->profTab[0]));
        if (ctxt->profTab == NULL) {
            xmlGenericError(xmlGenericErrorContext, "realloc failed !\n");
            return (0);
        }
    }
    ctxt->profTab[ctxt->profNr] = value;
    ctxt->prof = value;
    return (ctxt->profNr++);
}
/**
 * profPop:
 * @ctxt: the transformation context
 *
 * Pop a profiling value from the stack
 *
 * Returns the stored profiling value
 */
static long
profPop(xsltTransformContextPtr ctxt)
{
    long ret;

    if (ctxt->profNr <= 0)
        return (0);
    ctxt->profNr--;
    if (ctxt->profNr > 0)
        ctxt->prof = ctxt->profTab[ctxt->profNr - 1];
    else
        ctxt->prof = (long) 0;
    ret = ctxt->profTab[ctxt->profNr];
    ctxt->profTab[ctxt->profNr] = 0;
    return (ret);
}

/************************************************************************
 *									*
 *			XInclude default settings			*
 *									*
 ************************************************************************/

static int xsltDoXIncludeDefault = 0;

/**
 * xsltSetXIncludeDefault:
 * @xinclude: whether to do XInclude processing
 *
 * Set whether XInclude should be processed on document being loaded by default
 */
void
xsltSetXIncludeDefault(int xinclude) {
    xsltDoXIncludeDefault = (xinclude != 0);
}

/**
 * xsltGetXIncludeDefault:
 *
 * Provides the default state for XInclude processing
 *
 * Returns 0 if there is no processing 1 otherwise
 */
int
xsltGetXIncludeDefault(void) {
    return(xsltDoXIncludeDefault);
}

unsigned long xsltDefaultTrace = (unsigned long) XSLT_TRACE_ALL;

/**
 * xsltDebugSetDefaultTrace:
 * @val: tracing level mask
 *
 * Set the default debug tracing level mask
 */
void xsltDebugSetDefaultTrace(xsltDebugTraceCodes val) {
	xsltDefaultTrace = val;
}

/**
 * xsltDebugGetDefaultTrace:
 *
 * Get the current default debug tracing level mask
 *
 * Returns the current default debug tracing level mask
 */
xsltDebugTraceCodes xsltDebugGetDefaultTrace() {
	return xsltDefaultTrace;
}

/************************************************************************
 *									*
 *			Handling of Transformation Contexts		*
 *									*
 ************************************************************************/

/**
 * xsltNewTransformContext:
 * @style:  a parsed XSLT stylesheet
 * @doc:  the input document
 *
 * Create a new XSLT TransformContext
 *
 * Returns the newly allocated xsltTransformContextPtr or NULL in case of error
 */
xsltTransformContextPtr
xsltNewTransformContext(xsltStylesheetPtr style, xmlDocPtr doc) {
    xsltTransformContextPtr cur;
    xsltDocumentPtr docu;
    int i;

    cur = (xsltTransformContextPtr) xmlMalloc(sizeof(xsltTransformContext));
    if (cur == NULL) {
	xsltTransformError(NULL, NULL, (xmlNodePtr)doc,
		"xsltNewTransformContext : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltTransformContext));

    /*
     * setup of the dictionnary must be done early as some of the
     * processing later like key handling may need it.
     */
    cur->dict = xmlDictCreateSub(style->dict);
    cur->internalized = ((style->internalized) && (cur->dict != NULL));
#ifdef WITH_XSLT_DEBUG
    xsltGenericDebug(xsltGenericDebugContext,
	     "Creating sub-dictionary from stylesheet for transformation\n");
#endif

    /*
     * initialize the template stack
     */
    cur->templTab = (xsltTemplatePtr *)
	        xmlMalloc(10 * sizeof(xsltTemplatePtr));
    if (cur->templTab == NULL) {
	xsltTransformError(NULL, NULL, (xmlNodePtr) doc,
		"xsltNewTransformContext: out of memory\n");
	goto internal_err;
    }
    cur->templNr = 0;
    cur->templMax = 5;
    cur->templ = NULL;

    /*
     * initialize the variables stack
     */
    cur->varsTab = (xsltStackElemPtr *)
	        xmlMalloc(10 * sizeof(xsltStackElemPtr));
    if (cur->varsTab == NULL) {
        xmlGenericError(xmlGenericErrorContext,
		"xsltNewTransformContext: out of memory\n");
	goto internal_err;
    }
    cur->varsNr = 0;
    cur->varsMax = 5;
    cur->vars = NULL;
    cur->varsBase = 0;

    /*
     * the profiling stack is not initialized by default
     */
    cur->profTab = NULL;
    cur->profNr = 0;
    cur->profMax = 0;
    cur->prof = 0;

    cur->style = style;
    xmlXPathInit();
    cur->xpathCtxt = xmlXPathNewContext(doc);
    if (cur->xpathCtxt == NULL) {
	xsltTransformError(NULL, NULL, (xmlNodePtr) doc,
		"xsltNewTransformContext : xmlXPathNewContext failed\n");
	goto internal_err;
    }
    cur->xpathCtxt->proximityPosition = 0;
    cur->xpathCtxt->contextSize = 0;
    /*
    * Create an XPath cache.
    */
    if (xmlXPathContextSetCache(cur->xpathCtxt, 1, -1, 0) == -1)
	goto internal_err;
    /*
     * Initialize the extras array
     */
    if (style->extrasNr != 0) {
	cur->extrasMax = style->extrasNr + 20;
	cur->extras = (xsltRuntimeExtraPtr) 
	    xmlMalloc(cur->extrasMax * sizeof(xsltRuntimeExtra));
	if (cur->extras == NULL) {
	    xmlGenericError(xmlGenericErrorContext,
		    "xsltNewTransformContext: out of memory\n");
	    goto internal_err;
	}
	cur->extrasNr = style->extrasNr;
	for (i = 0;i < cur->extrasMax;i++) {
	    cur->extras[i].info = NULL;
	    cur->extras[i].deallocate = NULL;
	    cur->extras[i].val.ptr = NULL;
	}
    } else {
	cur->extras = NULL;
	cur->extrasNr = 0;
	cur->extrasMax = 0;
    }

    XSLT_REGISTER_VARIABLE_LOOKUP(cur);
    XSLT_REGISTER_FUNCTION_LOOKUP(cur);
    cur->xpathCtxt->nsHash = style->nsHash;
    /*
     * Initialize the registered external modules
     */
    xsltInitCtxtExts(cur);
    /*
     * Setup document element ordering for later efficiencies
     * (bug 133289)
     */
    if (xslDebugStatus == XSLT_DEBUG_NONE)
        xmlXPathOrderDocElems(doc);
    /*
     * Must set parserOptions before calling xsltNewDocument
     * (bug 164530)
     */
    cur->parserOptions = XSLT_PARSE_OPTIONS;
    docu = xsltNewDocument(cur, doc);
    if (docu == NULL) {
	xsltTransformError(cur, NULL, (xmlNodePtr)doc,
		"xsltNewTransformContext : xsltNewDocument failed\n");
	goto internal_err;
    }
    docu->main = 1;
    cur->document = docu;
    cur->inst = NULL;
    cur->outputFile = NULL;
    cur->sec = xsltGetDefaultSecurityPrefs();
    cur->debugStatus = xslDebugStatus;
    cur->traceCode = (unsigned long*) &xsltDefaultTrace;
    cur->xinclude = xsltGetXIncludeDefault();

    return(cur);

internal_err:
    if (cur != NULL)
	xsltFreeTransformContext(cur);
    return(NULL);
}

/**
 * xsltFreeTransformContext:
 * @ctxt:  an XSLT parser context
 *
 * Free up the memory allocated by @ctxt
 */
void
xsltFreeTransformContext(xsltTransformContextPtr ctxt) {
    if (ctxt == NULL)
	return;

    /*
     * Shutdown the extension modules associated to the stylesheet
     * used if needed.
     */
    xsltShutdownCtxtExts(ctxt);

    if (ctxt->xpathCtxt != NULL) {
	ctxt->xpathCtxt->nsHash = NULL;
	xmlXPathFreeContext(ctxt->xpathCtxt);
    }
    if (ctxt->templTab != NULL)
	xmlFree(ctxt->templTab);
    if (ctxt->varsTab != NULL)
	xmlFree(ctxt->varsTab);
    if (ctxt->profTab != NULL)
	xmlFree(ctxt->profTab);
    if ((ctxt->extrasNr > 0) && (ctxt->extras != NULL)) {
	int i;

	for (i = 0;i < ctxt->extrasNr;i++) {
	    if ((ctxt->extras[i].deallocate != NULL) &&
		(ctxt->extras[i].info != NULL))
		ctxt->extras[i].deallocate(ctxt->extras[i].info);
	}
	xmlFree(ctxt->extras);
    }
    xsltFreeGlobalVariables(ctxt);
    xsltFreeDocuments(ctxt);
    xsltFreeCtxtExts(ctxt);
    xsltFreeRVTs(ctxt);
    xmlDictFree(ctxt->dict);
#ifdef WITH_XSLT_DEBUG
    xsltGenericDebug(xsltGenericDebugContext,
                     "freeing transformation dictionnary\n");
#endif
    memset(ctxt, -1, sizeof(xsltTransformContext));
    xmlFree(ctxt);
}

/************************************************************************
 *									*
 *			Copy of Nodes in an XSLT fashion		*
 *									*
 ************************************************************************/

xmlNodePtr xsltCopyTree(xsltTransformContextPtr ctxt,
                        xmlNodePtr node, xmlNodePtr insert, int literal);

/**
 * xsltAddTextString:
 * @ctxt:  a XSLT process context
 * @target:  the text node where the text will be attached
 * @string:  the text string
 * @len:  the string length in byte
 *
 * Extend the current text node with the new string, it handles coalescing
 *
 * Returns: the text node
 */
static xmlNodePtr
xsltAddTextString(xsltTransformContextPtr ctxt, xmlNodePtr target,
		  const xmlChar *string, int len) {
    /*
     * optimization
     */
    if ((len <= 0) || (string == NULL) || (target == NULL))
        return(target);

    if (ctxt->lasttext == target->content) {

	if (ctxt->lasttuse + len >= ctxt->lasttsize) {
	    xmlChar *newbuf;
	    int size;

	    size = ctxt->lasttsize + len + 100;
	    size *= 2;
	    newbuf = (xmlChar *) xmlRealloc(target->content,size);
	    if (newbuf == NULL) {
		xsltTransformError(ctxt, NULL, target,
		 "xsltCopyText: text allocation failed\n");
		return(NULL);
	    }
	    ctxt->lasttsize = size;
	    ctxt->lasttext = newbuf;
	    target->content = newbuf;
	}
	memcpy(&(target->content[ctxt->lasttuse]), string, len);
	ctxt->lasttuse += len;
	target->content[ctxt->lasttuse] = 0;
    } else {
	xmlNodeAddContent(target, string);
	ctxt->lasttext = target->content;
	len = xmlStrlen(target->content);
	ctxt->lasttsize = len;
	ctxt->lasttuse = len;
    }
    return(target);
}

/**
 * xsltCopyTextString:
 * @ctxt:  a XSLT process context
 * @target:  the element where the text will be attached
 * @string:  the text string
 * @noescape:  should disable-escaping be activated for this text node.
 *
 * Create a text node
 *
 * Returns: a new xmlNodePtr, or NULL in case of error.
 */
xmlNodePtr
xsltCopyTextString(xsltTransformContextPtr ctxt, xmlNodePtr target,
	           const xmlChar *string, int noescape) {
    xmlNodePtr copy;
    int len;

    if (string == NULL)
	return(NULL);

#ifdef WITH_XSLT_DEBUG_PROCESS
    XSLT_TRACE(ctxt,XSLT_TRACE_COPY_TEXT,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltCopyTextString: copy text %s\n",
		     string));
#endif

    /* handle coalescing of text nodes here */
    len = xmlStrlen(string);
    if ((ctxt->type == XSLT_OUTPUT_XML) &&
	(ctxt->style->cdataSection != NULL) &&
	(target != NULL) && (target->type == XML_ELEMENT_NODE) &&
	(((target->ns == NULL) && 
	  (xmlHashLookup2(ctxt->style->cdataSection,
		          target->name, NULL) != NULL)) ||
	 ((target->ns != NULL) &&
	  (xmlHashLookup2(ctxt->style->cdataSection,
	                  target->name, target->ns->href) != NULL)))) {
	if ((target != NULL) && (target->last != NULL) &&
	    (target->last->type == XML_CDATA_SECTION_NODE)) {
	    return(xsltAddTextString(ctxt, target->last, string, len));
	}
	copy = xmlNewCDataBlock(ctxt->output, string, len);
    } else if (noescape) {
	if ((target != NULL) && (target->last != NULL) &&
	    (target->last->type == XML_TEXT_NODE) &&
	    (target->last->name == xmlStringTextNoenc)) {
	    return(xsltAddTextString(ctxt, target->last, string, len));
	}
	copy = xmlNewTextLen(string, len);
	if (copy != NULL)
	    copy->name = xmlStringTextNoenc;
    } else {
	if ((target != NULL) && (target->last != NULL) &&
	    (target->last->type == XML_TEXT_NODE) &&
	    (target->last->name == xmlStringText)) {
	    return(xsltAddTextString(ctxt, target->last, string, len));
	}
	copy = xmlNewTextLen(string, len);
    }
    if (copy != NULL) {
	if (target != NULL)
	    xmlAddChild(target, copy);
	ctxt->lasttext = copy->content;
	ctxt->lasttsize = len;
	ctxt->lasttuse = len;
    } else {
	xsltTransformError(ctxt, NULL, target,
			 "xsltCopyTextString: text copy failed\n");
	ctxt->lasttext = NULL;
    }
    return(copy);
}

/**
 * xsltCopyText:
 * @ctxt:  a XSLT process context
 * @target:  the element where the text will be attached
 * @cur:  the text or CDATA node
 * @interned:  the string is in the target doc dictionnary
 *
 * Do a copy of a text node
 *
 * Returns: a new xmlNodePtr, or NULL in case of error.
 */
static xmlNodePtr
xsltCopyText(xsltTransformContextPtr ctxt, xmlNodePtr target,
	     xmlNodePtr cur, int interned) {
    xmlNodePtr copy;

    if ((cur->type != XML_TEXT_NODE) &&
	(cur->type != XML_CDATA_SECTION_NODE))
	return(NULL);
    if (cur->content == NULL) 
	return(NULL);

#ifdef WITH_XSLT_DEBUG_PROCESS
    if (cur->type == XML_CDATA_SECTION_NODE) {
	XSLT_TRACE(ctxt,XSLT_TRACE_COPY_TEXT,xsltGenericDebug(xsltGenericDebugContext,
			 "xsltCopyText: copy CDATA text %s\n",
			 cur->content));
    } else if (cur->name == xmlStringTextNoenc) {
	XSLT_TRACE(ctxt,XSLT_TRACE_COPY_TEXT,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltCopyText: copy unescaped text %s\n",
			 cur->content));
    } else {
	XSLT_TRACE(ctxt,XSLT_TRACE_COPY_TEXT,xsltGenericDebug(xsltGenericDebugContext,
			 "xsltCopyText: copy text %s\n",
			 cur->content));
    }
#endif

    if ((ctxt->type == XSLT_OUTPUT_XML) &&
	(ctxt->style->cdataSection != NULL) &&
	(target != NULL) && (target->type == XML_ELEMENT_NODE) &&
	(((target->ns == NULL) && 
	  (xmlHashLookup2(ctxt->style->cdataSection,
		          target->name, NULL) != NULL)) ||
	 ((target->ns != NULL) &&
	  (xmlHashLookup2(ctxt->style->cdataSection,
	                  target->name, target->ns->href) != NULL)))) {
	/* 
	* OPTIMIZE TODO: xsltCopyText() is also used for attribute content.	
	*/
	/*
	* TODO: Since this doesn't merge adjacent CDATA-section nodes,
	* we'll get: <![CDATA[x]]><!CDATA[y]]>.
	*/
	copy = xmlNewCDataBlock(ctxt->output, cur->content,
				xmlStrlen(cur->content));
	ctxt->lasttext = NULL;
    } else if ((target != NULL) &&
	(target->last != NULL) &&
	/* both escaped or both non-escaped text-nodes */
	(((target->last->type == XML_TEXT_NODE) &&
	(target->last->name == cur->name)) ||
        /* non-escaped text nodes and CDATA-section nodes */
	(((target->last->type == XML_CDATA_SECTION_NODE) &&
	(cur->name == xmlStringTextNoenc)))))
    {
	/*
	 * we are appending to an existing text node
	 */
	return(xsltAddTextString(ctxt, target->last, cur->content,
	    xmlStrlen(cur->content)));
    } else if ((interned) && (target != NULL) &&
	(target->doc != NULL) &&
	(target->doc->dict == ctxt->dict))
    {        
	/*
	* TODO: DO we want to use this also for "text" output?
	*/
        copy = xmlNewTextLen(NULL, 0);
	if (copy == NULL)
	    return NULL;	
	if (cur->name == xmlStringTextNoenc)
	    copy->name = xmlStringTextNoenc;
	
	/* OPTIMIZE TODO: get rid of xmlDictOwns() in safe cases;
	 *  e.g. attribute values don't need the lookup.
	 *
	 * Must confirm that content is in dict (bug 302821)
	 * TODO: Check if bug 302821 still applies here.
	 */
	if (xmlDictOwns(ctxt->dict, cur->content))
	    copy->content = cur->content;
	else {
	    if ((copy->content = xmlStrdup(cur->content)) == NULL)
		return NULL;
	}
    } else {
        /*
	 * normal processing. keep counters to extend the text node
	 * in xsltAddTextString if needed.
	 */
        unsigned int len;

	len = xmlStrlen(cur->content);
	copy = xmlNewTextLen(cur->content, len);
	if (copy == NULL)
	    return NULL;
	if (cur->name == xmlStringTextNoenc)
	    copy->name = xmlStringTextNoenc;
	ctxt->lasttext = copy->content;
	ctxt->lasttsize = len;
	ctxt->lasttuse = len;
    }
    if (copy != NULL) {
	if (target != NULL) {
	    copy->doc = target->doc;
	    /*
	    * MAYBE TODO: Maybe we should reset the ctxt->lasttext here
	    *  to ensure that the optimized text-merging mechanism
	    *  won't interfere with normal node-merging in any case.
	    */
	    xmlAddChild(target, copy);
	}
    } else {
	xsltTransformError(ctxt, NULL, target,
			 "xsltCopyText: text copy failed\n");
    }
    return(copy);
}

/**
 * xsltCopyProp:
 * @ctxt:  a XSLT process context
 * @targetElem:  the element where the attribute will be grafted
 * @attr: the attribute to be copied
 *
 * Do a copy of an attribute
 *
 * Returns: a new xmlAttrPtr, or NULL in case of error.
 */
static xmlAttrPtr
xsltCopyProp(xsltTransformContextPtr ctxt, xmlNodePtr targetElem,
	     xmlAttrPtr attr)
{
    xmlAttrPtr attrCopy;
    xmlChar *value;
#ifdef XSLT_REFACTORED 
    xmlNodePtr txtNode;
#endif

    if (attr == NULL)
	return(NULL);

    if (targetElem->type != XML_ELEMENT_NODE) {
	/*
	* TODO: Hmm, it would be better to have the node at hand of the
	*  instruction which lead to this here.
	*/
	xsltTransformError(ctxt, NULL, NULL,
	    "Result tree construction error: cannot set an attribute node "
	    "on a non-element node.\n");
	return(NULL);
    }    

#ifdef XSLT_REFACTORED
    /*
    * Create the attribute node.
    */
    if (attr->ns != NULL) {
	xmlNsPtr ns = NULL;
	const xmlChar *prefix = attr->ns->prefix;
	
	/*
	* Process namespace semantics
	*
	* RESTRUCTURE TODO: This is the same code as in
	*  xsltAttributeInternal() (attributes.c), but I currently
	*  don't want to add yet another ns-lookup function.
	*/
	if ((targetElem->ns != NULL) &&
	    (targetElem->ns->prefix != NULL) &&
	    xmlStrEqual(targetElem->ns->href, attr->ns->href))
	{
	    ns = targetElem->ns;
	    goto namespace_finished;
	}
	if (prefix != NULL) {
	    /*
	    * Search by ns-prefix.
	    */
	    ns = xmlSearchNs(targetElem->doc, targetElem, prefix);
	    if ((ns != NULL) &&
		(xmlStrEqual(ns->href, attr->ns->href)))
	    {
		goto namespace_finished;
	    }
	}
	/*
	* Fallback to a search by ns-name.
	*/	
	ns = xmlSearchNsByHref(targetElem->doc, targetElem, attr->ns->href);
	if ((ns != NULL) && (ns->prefix != NULL)) {
	    goto namespace_finished;
	}
	/*
	* OK, we need to declare the namespace on the target element.
	*/
	if (prefix) {
	    if (targetElem->nsDef != NULL) {
		ns = targetElem->nsDef;
		do {
		    if ((ns->prefix) && xmlStrEqual(ns->prefix, prefix)) {
			/*
			* The prefix aready occupied.
			*/
			break;
		    }
		    ns = ns->next;
		} while (ns != NULL);
		if (ns == NULL) {
		    ns = xmlNewNs(targetElem, attr->ns->href, prefix);
		    goto namespace_finished;
		}
	    }
	}
	/*
	* Generate a new prefix.
	*/
	{
	    const xmlChar *basepref = prefix;
	    xmlChar pref[30];
	    int counter = 0;
	    
	    if (prefix != NULL)
		basepref = prefix;
	    else
		basepref = xmlStrdup(BAD_CAST "ns");
	    
	    do {
		snprintf((char *) pref, 30, "%s_%d",
		    basepref, counter++);
		ns = xmlSearchNs(targetElem->doc,
		    (xmlNodePtr) attr, BAD_CAST pref);
		if (counter > 1000) {
		    xsltTransformError(ctxt, NULL, (xmlNodePtr) attr,
			"Namespace fixup error: Failed to compute a "
			"new unique ns-prefix for the copied attribute "
			"{%s}%s'.\n", attr->ns->href, attr->name);
		    ns = NULL;
		    break;
		}
	    } while (ns != NULL);
	    if (basepref != prefix)
		xmlFree((xmlChar *)basepref);
	    ns = xmlNewNs(targetElem, attr->ns->href, BAD_CAST pref);
	}

namespace_finished:

	if (ns == NULL) {
	    xsltTransformError(ctxt, NULL, (xmlNodePtr) attr,
		"Namespace fixup error: Failed to acquire an in-scope "
		"namespace binding of the copied attribute '{%s}%s'.\n",
		attr->ns->href, attr->name);
	    /*
	    * TODO: Should we just stop here?
	    */
	}
	attrCopy = xmlSetNsProp(targetElem, ns, attr->name, NULL);
    } else {
	attrCopy = xmlSetNsProp(targetElem, NULL, attr->name, NULL);
    }
    if (attrCopy == NULL)	
	return(NULL);    
    /*
    * NOTE: This was optimized according to bug #342695.
    * TODO: Can this further be optimized, if source and target
    *  share the same dict and attr->children is just 1 text node
    *  which is in the dict? How probable is such a case?
    */
    value = xmlNodeListGetString(attr->doc, attr->children, 1);
    if (value != NULL) {
	txtNode = xmlNewDocText(targetElem->doc, NULL);
	if (txtNode == NULL)
	    return(NULL);
	if ((targetElem->doc != NULL) &&
	    (targetElem->doc->dict != NULL))
	{
	    txtNode->content =
		(xmlChar *) xmlDictLookup(targetElem->doc->dict,
		    BAD_CAST value, -1);
	    xmlFree(value);	    
	} else
	    txtNode->content = value;
	attrCopy->children = txtNode;
    }
    /*
    * URGENT TODO: Do we need to create an empty text node if the value
    *  is the empty string?
    */    

#else /* not XSLT_REFACTORED */

    value = xmlNodeListGetString(attr->doc, attr->children, 1);
    if (attr->ns != NULL) {
	xmlNsPtr ns;
	ns = xsltGetPlainNamespace(ctxt, attr->parent, attr->ns, targetElem);
	attrCopy = xmlSetNsProp(targetElem, ns, attr->name, value);
    } else {
	attrCopy = xmlSetNsProp(targetElem, NULL, attr->name, value);
    }
    if (value != NULL)
	xmlFree(value);

#endif /* not XSLT_REFACTORED */

    return(attrCopy);
}

/**
 * xsltCopyPropList:
 * @ctxt:  a XSLT process context
 * @target:  the element where the properties will be grafted
 * @cur:  the first property
 *
 * Do a copy of a properties list.
 *
 * Returns: a new xmlAttrPtr, or NULL in case of error.
 */
static xmlAttrPtr
xsltCopyPropList(xsltTransformContextPtr ctxt, xmlNodePtr target,
	         xmlAttrPtr cur) {
    xmlAttrPtr ret = NULL;
    xmlAttrPtr p = NULL,q;
    xmlNsPtr ns;

    while (cur != NULL) {
	if (cur->ns != NULL) {
	    ns = xsltGetNamespace(ctxt, cur->parent, cur->ns, target);
	} else {
	    ns = NULL;
	}
        q = xmlCopyProp(target, cur);
	if (q != NULL) {
	    q->ns = ns;
	    if (p == NULL) {
		ret = p = q;
	    } else {
		p->next = q;
		q->prev = p;
		p = q;
	    }
	}
	cur = cur->next;
    }
    return(ret);
}

/**
 * xsltCopyNode:
 * @ctxt:  a XSLT process context
 * @node:  the element node in the source tree.
 * @insert:  the parent in the result tree.
 *
 * Make a copy of the element node @node
 * and insert it as last child of @insert
 * Intended *only* for copying literal result elements and
 * text-nodes.
 * Called from:
 *   xsltApplyOneTemplateInt()
 *   xsltCopy()
 *
 * Returns a pointer to the new node, or NULL in case of error
 */
static xmlNodePtr
xsltCopyNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
	     xmlNodePtr insert) {
    xmlNodePtr copy;

    if ((node->type == XML_DTD_NODE) || (insert == NULL))
	return(NULL);
    if ((node->type == XML_TEXT_NODE) ||
	(node->type == XML_CDATA_SECTION_NODE))
	return(xsltCopyText(ctxt, insert, node, 0));
    copy = xmlDocCopyNode(node, insert->doc, 0);
    if (copy != NULL) {
	copy->doc = ctxt->output;
	xmlAddChild(insert, copy);
	if (node->type == XML_ELEMENT_NODE) {
	    /*
	     * Add namespaces as they are needed
	     */
	    if (node->nsDef != NULL)
		xsltCopyNamespaceList(ctxt, copy, node->nsDef);
	}
	if ((node->type == XML_ELEMENT_NODE) ||
	     (node->type == XML_ATTRIBUTE_NODE)) {
	    if (node->ns != NULL) {
		copy->ns = xsltGetNamespace(ctxt, node, node->ns, copy);
	    } else if ((insert->type == XML_ELEMENT_NODE) &&
		       (insert->ns != NULL)) {
		xmlNsPtr defaultNs;

		defaultNs = xmlSearchNs(insert->doc, insert, NULL);
		if (defaultNs != NULL)
		    xmlNewNs(copy, BAD_CAST "", NULL);
	    }
	}
    } else {
	xsltTransformError(ctxt, NULL, node,
		"xsltCopyNode: copy %s failed\n", node->name);
    }
    return(copy);
}

/**
 * xsltCopyTreeList:
 * @ctxt:  a XSLT process context
 * @list:  the list of element nodes in the source tree.
 * @insert:  the parent in the result tree.
 * @literal:  is this a literal result element list
 *
 * Make a copy of the full list of tree @list
 * and insert it as last children of @insert
 * For literal result element, some of the namespaces may not be copied
 * over according to section 7.1 .
 *
 * Returns a pointer to the new list, or NULL in case of error
 */
static xmlNodePtr
xsltCopyTreeList(xsltTransformContextPtr ctxt, xmlNodePtr list,
	     xmlNodePtr insert, int literal) {
    xmlNodePtr copy, ret = NULL;

    while (list != NULL) {
	copy = xsltCopyTree(ctxt, list, insert, literal);
	if (copy != NULL) {
	    if (ret == NULL) {
		ret = copy;
	    }
	}
	list = list->next;
    }
    return(ret);
}

/**
 * xsltCopyNamespaceListInternal:
 * @node:  the target node
 * @cur:  the first namespace
 *
 * Do a copy of a namespace list. If @node is non-NULL the
 * new namespaces are added automatically.
 * Called by:
 *   xsltCopyTree()
 *
 * TODO: What is the exact difference between this function
 *  and xsltCopyNamespaceList() in "namespaces.c"?
 *
 * Returns: a new xmlNsPtr, or NULL in case of error.
 */
static xmlNsPtr
xsltCopyNamespaceListInternal(xmlNodePtr node, xmlNsPtr cur) {
    xmlNsPtr ret = NULL;
    xmlNsPtr p = NULL,q;

    if (cur == NULL)
	return(NULL);
    if (cur->type != XML_NAMESPACE_DECL)
	return(NULL);

    /*
     * One can add namespaces only on element nodes
     */
    if ((node != NULL) && (node->type != XML_ELEMENT_NODE))
	node = NULL;

    while (cur != NULL) {
	if (cur->type != XML_NAMESPACE_DECL)
	    break;

	/*
	 * Avoid duplicating namespace declarations on the tree
	 */
	if ((node != NULL) && (node->ns != NULL) &&
            (xmlStrEqual(node->ns->href, cur->href)) &&
            (xmlStrEqual(node->ns->prefix, cur->prefix))) {
	    cur = cur->next;
	    continue;
	}
	
	q = xmlNewNs(node, cur->href, cur->prefix);
	if (p == NULL) {
	    ret = p = q;
	} else if (q != NULL) {
	    p->next = q;
	    p = q;
	}
	cur = cur->next;
    }
    return(ret);
}

/**
 * xsltCopyTree:
 * @ctxt:  a XSLT process context
 * @node:  the element node in the source tree.
 * @insert:  the parent in the result tree.
 * @literal:  is this a literal result element list
 *
 * Make a copy of the full tree under the element node @node
 * and insert it as last child of @insert
 * For literal result element, some of the namespaces may not be copied
 * over according to section 7.1.
 * TODO: Why is this a public function?
 *
 * Returns a pointer to the new tree, or NULL in case of error
 */
xmlNodePtr
xsltCopyTree(xsltTransformContextPtr ctxt, xmlNodePtr node,
		     xmlNodePtr insert, int literal) {
    xmlNodePtr copy;

    if (node == NULL)
	return(NULL);
    switch (node->type) {
        case XML_ELEMENT_NODE:
        case XML_ENTITY_REF_NODE:
        case XML_ENTITY_NODE:
        case XML_PI_NODE:
        case XML_COMMENT_NODE:
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
#ifdef LIBXML_DOCB_ENABLED
        case XML_DOCB_DOCUMENT_NODE:
#endif
	    break;
        case XML_TEXT_NODE: {
	    int noenc = (node->name == xmlStringTextNoenc);
	    return(xsltCopyTextString(ctxt, insert, node->content, noenc));
	    }
        case XML_CDATA_SECTION_NODE:
	    return(xsltCopyTextString(ctxt, insert, node->content, 0));
        case XML_ATTRIBUTE_NODE:
	    return((xmlNodePtr)
		   xsltCopyProp(ctxt, insert, (xmlAttrPtr) node));
        case XML_NAMESPACE_DECL:
	    if (insert->type != XML_ELEMENT_NODE)
		return(NULL);
	    return((xmlNodePtr)
		   xsltCopyNamespaceList(ctxt, insert, (xmlNsPtr) node));
	    
        case XML_DOCUMENT_TYPE_NODE:
        case XML_DOCUMENT_FRAG_NODE:
        case XML_NOTATION_NODE:
        case XML_DTD_NODE:
        case XML_ELEMENT_DECL:
        case XML_ATTRIBUTE_DECL:
        case XML_ENTITY_DECL:
        case XML_XINCLUDE_START:
        case XML_XINCLUDE_END:
            return(NULL);
    }    
    if (XSLT_IS_RES_TREE_FRAG(node)) {
	if (node->children != NULL)
	    copy = xsltCopyTreeList(ctxt, node->children, insert, 0);
	else
	    copy = NULL;
	return(copy);
    }
    copy = xmlDocCopyNode(node, insert->doc, 0);
    if (copy != NULL) {
	copy->doc = ctxt->output;
	xmlAddChild(insert, copy);
	/*
	 * The node may have been coalesced into another text node.
	 */
	if (insert->last != copy)
	    return(insert->last);

	copy->next = NULL;
	/*
	 * Add namespaces as they are needed
	 */
	if ((node->type == XML_ELEMENT_NODE) ||
	    (node->type == XML_ATTRIBUTE_NODE)) {
	    xmlNsPtr *nsList, *cur, ns;
	    /*
	     * Must add any new namespaces in scope for the node.
	     * TODO: Since we try to reuse existing in-scope ns-decls by
	     *  using xmlSearchNsByHref(), this will eventually change
	     *  the prefix of an original ns-binding; thus it might
	     *  break QNames in element/attribute content.
	     */
	    nsList = xmlGetNsList(node->doc, node);
	    if (nsList != NULL) {
		cur = nsList;
		while (*cur != NULL) {
		    ns = xmlSearchNsByHref(insert->doc, insert, (*cur)->href);
		    if (ns == NULL)
			xmlNewNs(copy, (*cur)->href, (*cur)->prefix);
		    cur++;
		}
		xmlFree(nsList);
	    }
	    if (node->ns != NULL) {
		/*
		* This will map  copy->ns to one of the newly created
		* in-scope ns-decls.
		*/
		copy->ns = xsltGetNamespace(ctxt, node, node->ns, copy);		
	    } else if ((insert->type == XML_ELEMENT_NODE) &&
		(insert->ns != NULL))
	    {
		xmlNsPtr defaultNs;

		defaultNs = xmlSearchNs(insert->doc, insert, NULL);
		if (defaultNs != NULL)
		    xmlNewNs(copy, BAD_CAST "", NULL);
	    }
	}
	if (node->nsDef != NULL) {
	    if (literal)
	        xsltCopyNamespaceList(ctxt, copy, node->nsDef);
	    else
	        xsltCopyNamespaceListInternal(copy, node->nsDef);
	}
	if (node->properties != NULL)
	    copy->properties = xsltCopyPropList(ctxt, copy,
					       node->properties);
	if (node->children != NULL)
	    xsltCopyTreeList(ctxt, node->children, copy, literal);
    } else {
	xsltTransformError(ctxt, NULL, node,
		"xsltCopyTree: copy %s failed\n", node->name);
    }
    return(copy);
}

/************************************************************************
 *									*
 *		Error/fallback processing				*
 *									*
 ************************************************************************/

/**
 * xsltApplyFallbacks:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the node generating the error
 *
 * Process possible xsl:fallback nodes present under @inst
 *
 * Returns the number of xsl:fallback element found and processed
 */
static int
xsltApplyFallbacks(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst) {

    xmlNodePtr child;
    int ret = 0;
    
    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) ||
	(inst->children == NULL))
	return(0);

    child = inst->children;
    while (child != NULL) {
        if ((IS_XSLT_ELEM(child)) &&
            (xmlStrEqual(child->name, BAD_CAST "fallback"))) {
#ifdef WITH_XSLT_DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
			     "applying xsl:fallback\n");
#endif
	    ret++;
	    xsltApplyOneTemplateInt(ctxt, node, child->children, NULL, NULL, 0);
	}
	child = child->next;
    }
    return(ret);
}

/************************************************************************
 *									*
 *			Default processing				*
 *									*
 ************************************************************************/

void xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
			xsltStackElemPtr params);
/**
 * xsltDefaultProcessOneNode:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @params: extra parameters passed to the template if any
 *
 * Process the source node with the default built-in template rule:
 * <xsl:template match="*|/">
 *   <xsl:apply-templates/>
 * </xsl:template>
 *
 * and
 *
 * <xsl:template match="text()|@*">
 *   <xsl:value-of select="."/>
 * </xsl:template>
 *
 * Note also that namespace declarations are copied directly:
 *
 * the built-in template rule is the only template rule that is applied
 * for namespace nodes.
 */
static void
xsltDefaultProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
			  xsltStackElemPtr params) {
    xmlNodePtr copy;
    xmlNodePtr delete = NULL, cur;
    int nbchild = 0, oldSize;
    int childno = 0, oldPos;
    xsltTemplatePtr template;

    CHECK_STOPPED;
    /*
     * Handling of leaves
     */
    switch (node->type) {
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_ELEMENT_NODE:
	    break;
	case XML_CDATA_SECTION_NODE:
#ifdef WITH_XSLT_DEBUG_PROCESS
	    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltDefaultProcessOneNode: copy CDATA %s\n",
		node->content));
#endif
	    copy = xsltCopyText(ctxt, ctxt->insert, node, 0);
	    if (copy == NULL) {
		xsltTransformError(ctxt, NULL, node,
		 "xsltDefaultProcessOneNode: cdata copy failed\n");
	    }
	    return;
	case XML_TEXT_NODE:
#ifdef WITH_XSLT_DEBUG_PROCESS
	    if (node->content == NULL) {
		XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: copy empty text\n"));
	    } else {
		XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: copy text %s\n",
			node->content));
            }
#endif
	    copy = xsltCopyText(ctxt, ctxt->insert, node, 0);
	    if (copy == NULL) {
		xsltTransformError(ctxt, NULL, node,
		 "xsltDefaultProcessOneNode: text copy failed\n");
	    }
	    return;
	case XML_ATTRIBUTE_NODE:
	    cur = node->children;
	    while ((cur != NULL) && (cur->type != XML_TEXT_NODE))
		cur = cur->next;
	    if (cur == NULL) {
		xsltTransformError(ctxt, NULL, node,
		 "xsltDefaultProcessOneNode: no text for attribute\n");
	    } else {
#ifdef WITH_XSLT_DEBUG_PROCESS
		if (cur->content == NULL) {
		    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: copy empty text\n"));
		} else {
		    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: copy text %s\n",
			cur->content));
                }
#endif
		copy = xsltCopyText(ctxt, ctxt->insert, cur, 0);
		if (copy == NULL) {
		    xsltTransformError(ctxt, NULL, node,
		     "xsltDefaultProcessOneNode: text copy failed\n");
		}
	    }
	    return;
	default:
	    return;
    }
    /*
     * Handling of Elements: first pass, cleanup and counting
     */
    cur = node->children;
    while (cur != NULL) {
	switch (cur->type) {
	    case XML_TEXT_NODE:
	    case XML_CDATA_SECTION_NODE:
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
	    case XML_ELEMENT_NODE:
	    case XML_PI_NODE:
	    case XML_COMMENT_NODE:
		nbchild++;
		break;
            case XML_DTD_NODE:
		/* Unlink the DTD, it's still reachable using doc->intSubset */
		if (cur->next != NULL)
		    cur->next->prev = cur->prev;
		if (cur->prev != NULL)
		    cur->prev->next = cur->next;
		break;
	    default:
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: skipping node type %d\n",
		                 cur->type));
#endif
		delete = cur;
	}
	cur = cur->next;
	if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
	    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: removing ignorable blank node\n"));
#endif
	    xmlUnlinkNode(delete);
	    xmlFreeNode(delete);
	    delete = NULL;
	}
    }
    if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
	XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltDefaultProcessOneNode: removing ignorable blank node\n"));
#endif
	xmlUnlinkNode(delete);
	xmlFreeNode(delete);
	delete = NULL;
    }

    /*
     * Handling of Elements: second pass, actual processing
     */
    oldSize = ctxt->xpathCtxt->contextSize;
    oldPos = ctxt->xpathCtxt->proximityPosition;
    cur = node->children;
    while (cur != NULL) {
	childno++;
	switch (cur->type) {
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
	    case XML_ELEMENT_NODE:
		ctxt->xpathCtxt->contextSize = nbchild;
		ctxt->xpathCtxt->proximityPosition = childno;
		xsltProcessOneNode(ctxt, cur, params);
		break;
	    case XML_CDATA_SECTION_NODE:
		template = xsltGetTemplate(ctxt, cur, NULL);
		if (template) {
#ifdef WITH_XSLT_DEBUG_PROCESS
		    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltDefaultProcessOneNode: applying template for CDATA %s\n",
				     cur->content));
#endif
		    xsltApplyOneTemplateInt(ctxt, cur, template->content,
			                 template, params, 0);
		} else /* if (ctxt->mode == NULL) */ {
#ifdef WITH_XSLT_DEBUG_PROCESS
		    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: copy CDATA %s\n",
				     cur->content));
#endif
		    copy = xsltCopyText(ctxt, ctxt->insert, cur, 0);
		    if (copy == NULL) {
			xsltTransformError(ctxt, NULL, cur,
			    "xsltDefaultProcessOneNode: cdata copy failed\n");
		    }
		}
		break;
	    case XML_TEXT_NODE:
		template = xsltGetTemplate(ctxt, cur, NULL);
		if (template) {
#ifdef WITH_XSLT_DEBUG_PROCESS
		    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltDefaultProcessOneNode: applying template for text %s\n",
				     cur->content));
#endif
		    ctxt->xpathCtxt->contextSize = nbchild;
		    ctxt->xpathCtxt->proximityPosition = childno;
		    xsltApplyOneTemplateInt(ctxt, cur, template->content,
			                 template, params, 0);
		} else /* if (ctxt->mode == NULL) */ {
#ifdef WITH_XSLT_DEBUG_PROCESS
		    if (cur->content == NULL) {
			XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
			 "xsltDefaultProcessOneNode: copy empty text\n"));
		    } else {
			XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: copy text %s\n",
					 cur->content));
                    }
#endif
		    copy = xsltCopyText(ctxt, ctxt->insert, cur, 0);
		    if (copy == NULL) {
			xsltTransformError(ctxt, NULL, cur,
			    "xsltDefaultProcessOneNode: text copy failed\n");
		    }
		}
		break;
	    case XML_PI_NODE:
	    case XML_COMMENT_NODE:
		template = xsltGetTemplate(ctxt, cur, NULL);
		if (template) {
#ifdef WITH_XSLT_DEBUG_PROCESS
		    if (cur->type == XML_PI_NODE) {
			XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: template found for PI %s\n",
			                 cur->name));
		    } else if (cur->type == XML_COMMENT_NODE) {
			XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltDefaultProcessOneNode: template found for comment\n"));
                    }
#endif
		    ctxt->xpathCtxt->contextSize = nbchild;
		    ctxt->xpathCtxt->proximityPosition = childno;
		    xsltApplyOneTemplateInt(ctxt, cur, template->content,
			                 template, params, 0);
		}
		break;
	    default:
		break;
	}
	cur = cur->next;
    }
    ctxt->xpathCtxt->contextSize = oldSize;
    ctxt->xpathCtxt->proximityPosition = oldPos;
}

/**
 * xsltProcessOneNode:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @params:  extra parameters passed to the template if any
 *
 * Process the source node.
 */
void
xsltProcessOneNode(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xsltStackElemPtr params) {
    xsltTemplatePtr template;
    xmlNodePtr oldNode;
    
    template = xsltGetTemplate(ctxt, node, NULL);
    /*
     * If no template is found, apply the default rule.
     */
    if (template == NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
	if (node->type == XML_DOCUMENT_NODE) {
	    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for /\n"));
	} else if (node->type == XML_CDATA_SECTION_NODE) {
	    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for CDATA\n"));
	} else if (node->type == XML_ATTRIBUTE_NODE) {
	    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for attribute %s\n",
	                     ((xmlAttrPtr) node)->name));
	} else  {
	    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: no template found for %s\n", node->name));
        }
#endif
	oldNode = ctxt->node;
	ctxt->node = node;
	xsltDefaultProcessOneNode(ctxt, node, params);
	ctxt->node = oldNode;
	return;
    }

    if (node->type == XML_ATTRIBUTE_NODE) {
#ifdef WITH_XSLT_DEBUG_PROCESS
	XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template '%s' for attribute %s\n",
	                 template->match, node->name));
#endif
	xsltApplyOneTemplateInt(ctxt, node, template->content, template, params, 0);
    } else {
#ifdef WITH_XSLT_DEBUG_PROCESS
	if (node->type == XML_DOCUMENT_NODE) {
	    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template '%s' for /\n",
	                     template->match));
	} else {
	    XSLT_TRACE(ctxt,XSLT_TRACE_PROCESS_NODE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessOneNode: applying template '%s' for %s\n",
	                     template->match, node->name));
        }
#endif
	xsltApplyOneTemplateInt(ctxt, node, template->content, template, params, 0);
    }
}

#ifdef XSLT_REFACTORED
/**
* xsltTransLREUndeclareDefaultNs:
* @ctxt:  the transformation context
* @cur:  the literal result element
* @ns:  the namespace
* @out:  the output node (or its parent)
*
*
* Find a matching (prefix and ns-name) ns-declaration
*  for the given @ns in the result tree.
* If none is found then a new ns-declaration will be
*  added to @out. If, in this case, the given prefix is already
*  in use, then a ns-declaration with a modified ns-prefix
*  be we created.
*
* Returns the acquired ns-declaration
*         or NULL in case of an API or internal error.
*/
static int
xsltTransLREUndeclareResultDefaultNs(xsltTransformContextPtr ctxt,
				     xmlNodePtr cur,
				     xmlNodePtr resultElem)
{
    xmlNsPtr ns;
    /*
    * OPTIMIZE TODO: This all could be optimized by keeping track of
    *  the ns-decls currently in-scope via a specialized context.
    */
    /*
    * Search on the result element itself.
    */
    if (resultElem->nsDef != NULL) {
	ns = resultElem->nsDef;
	do {
	    if (ns->prefix == NULL) {
		if ((ns->href != NULL) && (ns->href[0] != 0)) {
		    /*
		    * Raise a namespace normalization error.
		    */
		    xsltTransformError(ctxt, NULL, cur,
			"Namespace normalization error: Cannot undeclare "
			"the default namespace, since the default namespace "
			"'%s' is already declared on the result element.\n",
			ns->href);
		    return(1);
		} else {
		    /*
		    * The default namespace was undeclared on the
		    * result element.
		    */
		    return(0);
		}
		break;
	    }
	    ns = ns->next;
	} while (ns != NULL);
    }

    if ((resultElem->parent != NULL) &&
	(resultElem->parent->type == XML_ELEMENT_NODE))
    {
	/*
	* The parent element is in no namespace, so assume
	* that there is no default namespace in scope.
	*/
	if (resultElem->parent->ns == NULL)
	    return(0);

	ns = xmlSearchNs(resultElem->doc, resultElem->parent,
	    NULL);
	/*
	* Fine if there's no default ns is scope, or if the
	* default ns was undeclared.
	*/
	if ((ns == NULL) || (ns->href == NULL) || (ns->href[0] == 0))
	    return(0);

	/*
	* Undeclare the default namespace.
	*/
	ns = xmlNewNs(resultElem, BAD_CAST "", NULL);
	/* TODO: Check result */	
	return(0);
    }
    return(0);
}

/**
* xsltTransLREAcquireResultInScopeNs:
* @ctxt: the transformation context
* @cur: the literal result element (in the stylesheet)
* @literalNs: the namespace (in the stylsheet)
* @resultElem: the generated result element
*
*
* Find a matching (prefix and ns-name) ns-declaration
*  for the given @ns in the result tree.
* If none is found then a new ns-declaration will be
*  added to @out. If, in this case, the given prefix is already
*  in use, then a ns-declaration with a modified ns-prefix
*  be we created.
*
* Returns the acquired ns-declaration
*         or NULL in case of an API or internal error.
*/
static xmlNsPtr
xsltTransLREAcquireResultInScopeNs(xsltTransformContextPtr ctxt,
				   xmlNodePtr cur,
				   xmlNsPtr literalNs,
				   xmlNodePtr resultElem)
{    
    xmlNsPtr ns;
    int prefixOccupied = 0;

    if ((ctxt == NULL) || (cur == NULL) || (resultElem == NULL))
	return(NULL);
    
    /*
    * OPTIMIZE TODO: This all could be optimized by keeping track of
    *  the ns-decls currently in-scope via a specialized context.
    */
    /*
    * NOTE: Namespace exclusion and ns-aliasing is performed at
    * compilation-time in the refactored code; so this need not be done
    * here.
    */
    /*
    * First: search on the result element itself.
    */
    if (resultElem->nsDef != NULL) {
	ns = resultElem->nsDef;
	do {
	    if ((ns->prefix == NULL) == (literalNs->prefix == NULL)) {
		if (literalNs->prefix == NULL) {
		    if (xmlStrEqual(ns->href, literalNs->href))
			return(ns);
		    prefixOccupied = 1;
		    break;
		} else if ((ns->prefix[0] == literalNs->prefix[0]) &&
		     xmlStrEqual(ns->prefix, literalNs->prefix))
		{
		    if (xmlStrEqual(ns->href, literalNs->href))
			return(ns);
		    prefixOccupied = 1;
		    break;
		}
	    }
	    ns = ns->next;
	} while (ns != NULL);
    }
    if (prefixOccupied) {
	/*
	* If the ns-prefix is occupied by an other ns-decl on the
	* result element, then this means:
	* 1) The desired prefix is shadowed
	* 2) There's no way around changing the prefix	
	*
	* Try a desperate search for an in-scope ns-decl
	* with a matching ns-name before we use the last option,
	* which is to recreate the ns-decl with a modified prefix.
	*/
	ns = xmlSearchNsByHref(resultElem->doc, resultElem, literalNs->href);
	if (ns != NULL)
	    return(ns);

	/*
	* Fallback to changing the prefix.
	*/    
    } else if ((resultElem->parent != NULL) &&
	(resultElem->parent->type == XML_ELEMENT_NODE)) {
	/*
	* Try to find a matching ns-decl in the ancestor-axis.
	*
	* Check the common case: The parent element of the current
	* result element is in the same namespace (with an equal ns-prefix).
	*/     
	if ((resultElem->parent->ns != NULL) &&
	    ((resultElem->parent->ns->prefix == NULL) ==
	     (literalNs->prefix == NULL)))
	{
	    ns = resultElem->parent->ns;
	    
	    if (literalNs->prefix == NULL) {
		if (xmlStrEqual(ns->href, literalNs->href))
		    return(ns);
	    } else if ((ns->prefix[0] == literalNs->prefix[0]) &&
		xmlStrEqual(ns->prefix, literalNs->prefix) &&
		xmlStrEqual(ns->href, literalNs->href))
	    {
		return(ns);
	    }
	}
	/*
	* Lookup the remaining in-scope namespaces.
	*/    
	ns = xmlSearchNs(resultElem->doc, resultElem->parent,
	    literalNs->prefix);
	if ((ns != NULL) && xmlStrEqual(ns->href, literalNs->href))
	    return(ns);
	ns = NULL;
	/*
	* Either no matching ns-prefix was found or the namespace is
	* shadowed.
	* Create a new ns-decl on the current result element.
	*
	* SPEC TODO: Hmm, we could also try to reuse an in-scope
	*  namespace with a matching ns-name but a different
	*  ns-prefix.
	*  What has higher precedence? 
	*  1) If keeping the prefix: create a new ns-decl.
	*  2) If reusal: first lookup ns-names; then fallback
	*     to creation of a new ns-decl.
	* REVISIT TODO: this currently uses case 2) since this
	*  is the way it used to be before refactoring.
	*/
	ns = xmlSearchNsByHref(resultElem->doc, resultElem,
	    literalNs->href);
	if (ns != NULL)
	    return(ns);
	/*
	* Create the ns-decl on the current result element.
	*/
	ns = xmlNewNs(resultElem, literalNs->href, literalNs->prefix);
	/* TODO: check errors */
	return(ns);
    } else if ((resultElem->parent == NULL) ||
	(resultElem->parent->type != XML_ELEMENT_NODE))
    {
	/*
	* This is the root of the tree.
	*/
	ns = xmlNewNs(resultElem, literalNs->href, literalNs->prefix);
	/* TODO: Check result */
	return(ns);
    }
    /*
    * Fallback: we need to generate a new prefix and declare the namespace
    * on the result element.
    */
    {
	xmlChar prefix[30];
	int counter = 0;
	
	/*
	* Comment copied from xslGetNamespace():
	*  "For an element node, if we don't find it, or it's the default
	*  and this element already defines a default (bug 165560), we
	*  need to create it."
	*/		
	do {
	    snprintf((char *) prefix, 30, "%s_%d",
		literalNs->prefix, counter++);
	    ns = xmlSearchNs(resultElem->doc, resultElem, BAD_CAST prefix);
	    if (counter > 1000) {
		xsltTransformError(ctxt, NULL, cur,
		    "Internal error in xsltTransLREAcquireInScopeNs(): "		    
		    "Failed to compute a unique ns-prefix for the "
		    "result element");
		return(NULL);
	    }
	} while (ns != NULL);
	ns = xmlNewNs(resultElem, literalNs->href, BAD_CAST prefix);
	/* TODO: Check result */
	return(ns);
    }
    return(NULL);    
}

#endif /* XSLT_REFACTORED */

/**
 * xsltApplyOneTemplate:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @list:  the template replacement nodelist
 * @templ: if is this a real template processing, the template processed
 * @params:  a set of parameters for the template or NULL
 *
 * Process the apply-templates node on the source node, if params are passed
 * they are pushed on the variable stack but not popped, it's left to the
 * caller to handle them after return (they may be reused).
 */
void
xsltApplyOneTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
                     xmlNodePtr list, xsltTemplatePtr templ,
                     xsltStackElemPtr params)
{
    xsltApplyOneTemplateInt(ctxt, node, list, templ, params, 0);
}

/**
 * xsltApplyOneTemplateInt:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @list:  the template replacement nodelist
 * @templ: if is this a real template processing, the template processed
 * @params:  a set of parameters for the template or NULL
 * @notcur: flag to show current template rule doesn't change
 *
 * See above description for xsltApplyOneTemplate.  Internally there is
 * an additional parameter 'notcur'.  When this parameter is non-zero,
 * ctxt->templ is not changed (i.e. templPush and tempPop are not called).
 * This is used by xsltCallTemplate in order to meet the XSLT spec (5.6)
 * requirement that the "current template rule" should not be changed
 * (bug 157859).
 */
static void
xsltApplyOneTemplateInt(xsltTransformContextPtr ctxt, xmlNodePtr node,
                     xmlNodePtr list, xsltTemplatePtr templ,
                     xsltStackElemPtr params, int notcur)
{
    xmlNodePtr cur = NULL, insert, copy = NULL;
    xmlNodePtr oldInsert;
    xmlNodePtr oldCurrent = NULL;
    xmlNodePtr oldInst = NULL;
    int oldBase;
    xmlDocPtr tmpRVT = NULL;
#ifdef XSLT_REFACTORED
    xsltStylePreCompPtr info;
#endif

    int level = 0;

#ifdef WITH_DEBUGGER
    int addCallResult = 0;
    xmlNodePtr debugedNode = NULL;
#endif
    long start = 0;

    if (ctxt == NULL) return;

#ifdef WITH_DEBUGGER
    if (ctxt->debugStatus != XSLT_DEBUG_NONE) {
        if (templ) {
            addCallResult = xslAddCall(templ, templ->elem);
        } else {
            addCallResult = xslAddCall(NULL, list);
        }

        switch (ctxt->debugStatus) {

            case XSLT_DEBUG_RUN_RESTART:
            case XSLT_DEBUG_QUIT:
                if (addCallResult)
                    xslDropCall();
                return;
        }

        if (templ) {
            xslHandleDebugger(templ->elem, node, templ, ctxt);
            debugedNode = templ->elem;
        } else if (list) {
            xslHandleDebugger(list, node, templ, ctxt);
            debugedNode = list;
        } else if (ctxt->inst) {
            xslHandleDebugger(ctxt->inst, node, templ, ctxt);
            debugedNode = ctxt->inst;
        }
    }
#endif

    if (list == NULL)
        return;
    CHECK_STOPPED;

    if ((ctxt->templNr >= xsltMaxDepth) ||
        (ctxt->varsNr >= 5 * xsltMaxDepth)) {
        xsltTransformError(ctxt, NULL, list,
                         "xsltApplyOneTemplate: loop found ???\n");
        xsltGenericError(xsltGenericErrorContext,
                         "try increasing xsltMaxDepth (--maxdepth)\n");
        xsltDebug(ctxt, node, list, NULL);
        return;
    }

    /*
     * stack saves, beware ordering of operations counts
     */
    oldInsert = insert = ctxt->insert;
    oldInst = ctxt->inst;
    oldCurrent = ctxt->node;
    varsPush(ctxt, params);
    oldBase = ctxt->varsBase;   /* only needed if templ != NULL */
    if (templ != NULL) {
        ctxt->varsBase = ctxt->varsNr - 1;
        ctxt->node = node;
        if (ctxt->profile) {
            templ->nbCalls++;
            start = xsltTimestamp();
            profPush(ctxt, 0);
        }
	tmpRVT = ctxt->tmpRVT;
	ctxt->tmpRVT = NULL;
	if (!notcur)
            templPush(ctxt, templ);
#ifdef WITH_XSLT_DEBUG_PROCESS
        if (templ->name != NULL)
            XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
                             "applying template '%s'\n", templ->name));
#endif
    }

    /*
     * Insert all non-XSLT nodes found in the template
     */
    cur = list;
    while (cur != NULL) {
        ctxt->inst = cur;
#ifdef WITH_DEBUGGER
        switch (ctxt->debugStatus) {
            case XSLT_DEBUG_RUN_RESTART:
            case XSLT_DEBUG_QUIT:
                break;

        }
#endif
        /*
         * test, we must have a valid insertion point
         */
        if (insert == NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
            XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
                             "xsltApplyOneTemplateInt: insert == NULL !\n"));
#endif
            goto error;
        }
#ifdef WITH_DEBUGGER
        if ((ctxt->debugStatus != XSLT_DEBUG_NONE) && (debugedNode != cur))
            xslHandleDebugger(cur, node, templ, ctxt);
#endif

#ifdef XSLT_REFACTORED
	if (cur->type == XML_ELEMENT_NODE) {
	    info = (xsltStylePreCompPtr) cur->psvi;
	    /*
	    * We expect a compiled representation on:
	    * 1) XSLT instructions of this XSLT version (1.0)
	    *    (with a few exceptions)
	    * 2) Literal result elements
	    * 3) Extension instructions
	    * 4) XSLT instructions of future XSLT versions
	    *    (forwards-compatible mode).
	    */
	    if (info == NULL) {
		/*
		* Handle the rare cases where we don't expect a compiled
		* representation on an XSLT element.
		*/
		if (IS_XSLT_ELEM_FAST(cur) && IS_XSLT_NAME(cur, "message")) {
		    xsltMessage(ctxt, node, cur);
		    goto skip_children;
		}		    		 
		/*
		* Something really went wrong:
		*/
		xsltTransformError(ctxt, NULL, cur,
		    "Internal error in xsltApplyOneTemplateInt(): "
		    "The element '%s' in the stylesheet has no compiled "
		    "representation.\n",
		    cur->name);
                goto skip_children;
            }

	    if (info->type == XSLT_FUNC_LITERAL_RESULT_ELEMENT) {
		xsltStyleItemLRElementInfoPtr lrInfo =
		    (xsltStyleItemLRElementInfoPtr) info;
		/*
		* Literal result elements
		* --------------------------------------------------------
		*/
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt, XSLT_TRACE_APPLY_TEMPLATE,
		    xsltGenericDebug(xsltGenericDebugContext,
		    "xsltApplyOneTemplateInt: copy literal result "
		    "element '%s'\n", cur->name));
#endif
		/*
		* Copy the raw element-node.
		* OLD: if ((copy = xsltCopyNode(ctxt, cur, insert)) == NULL)
		*   goto error;
		*/		
		copy = xmlDocCopyNode(cur, insert->doc, 0);
		if (copy == NULL) {
		    xsltTransformError(ctxt, NULL, cur,
			"Internal error in xsltApplyOneTemplateInt(): "
			"Failed to copy literal result element '%s'.\n",
			cur->name);
		    goto error;
		} else {
		    /*
		    * Add the element-node to the result tree.
		    */
		    copy->doc = ctxt->output;
		    xmlAddChild(insert, copy);
		    /*
		    * Create effective namespaces declarations.
		    * OLD: xsltCopyNamespaceList(ctxt, copy, cur->nsDef);
		    */
		    if (lrInfo->effectiveNs != NULL) {
			xsltEffectiveNsPtr effNs = lrInfo->effectiveNs;
			xmlNsPtr ns, lastns = NULL;

			while (effNs != NULL) {
			    /*
			    * Avoid generating redundant namespace
			    * declarations; thus lookup if there is already
			    * such a ns-decl in the result.
			    */			    
			    ns = xmlSearchNs(copy->doc, copy, effNs->prefix);
			    if ((ns != NULL) &&
				(xmlStrEqual(ns->href, effNs->nsName)))
			    {
				effNs = effNs->next;
				continue;			    
			    }
			    ns = xmlNewNs(copy, effNs->nsName, effNs->prefix);
			    if (ns == NULL) {
				xsltTransformError(ctxt, NULL, cur,
				    "Internal error in xsltApplyOneTemplateInt(): "
				    "Failed to copy a namespace declaration.\n");
				goto error;
			    }
								
			    if (lastns == NULL)
				copy->nsDef = ns;
			    else
				lastns->next =ns;
			    lastns = ns;

			    effNs = effNs->next;
			}
			
		    }
		    /*
		    * NOTE that we don't need to apply ns-alising: this was
		    *  already done at compile-time.
		    */
		    if (cur->ns != NULL) {
			/*
			* If there's no such ns-decl in the result tree,
			* then xsltGetNamespace() will create a ns-decl
			* on the copied node.
			*/
			/*
			* REVISIT TODO: Changed to use
			*  xsltTransLREAcquireInScopeNs() instead of
			*  xsltGetNamespace().
			*  OLD: copy->ns = xsltGetNamespace(ctxt, cur,
			*                     cur->ns, copy);
			*/
			copy->ns = xsltTransLREAcquireResultInScopeNs(ctxt,
			    cur, cur->ns, copy);
		    } else {
			/*
			* Undeclare the default namespace if needed.
			* This can be skipped, if:
			* 1) If the result element has no ns-decls, in which
			*    case the result element abviously does not
			*    declare a default namespace.
			* 2) AND there's either no parent, or the parent
			*   is in no namespace; this means there's no
			*   default namespace is scope to care about.
			*
			* REVISIT TODO: This might result in massive
			*  generation of ns-decls if nodes in a default
			*  namespaces are mixed with nodes in no namespace.
			*  
			*/
			if (copy->nsDef ||
			    ((insert != NULL) && (insert->ns != NULL)))
			    xsltTransLREUndeclareResultDefaultNs(ctxt,
				cur, copy);
#if 0
			defaultNs = xmlSearchNs(insert->doc, insert, NULL);
			if ((defaultNs != NULL) && (defaultNs->href != NULL))
			    xmlNewNs(copy, BAD_CAST "", NULL);
#endif
		    }
		}
		/*
		* SPEC XSLT 2.0 "Each attribute of the literal result
		*  element, other than an attribute in the XSLT namespace,
		*  is processed to produce an attribute for the element in
		*  the result tree."
		* TODO: Refactor this, since it still uses ns-aliasing.
		*/
		if (cur->properties != NULL) {
		    xsltAttrListTemplateProcess(ctxt, copy, cur->properties);
		}
		/*
		* OLD-COMMENT: "Add extra namespaces inherited from the
		*   current template if we are in the first level children
		*   and this is a "real" template.
		*
		* SPEC XSLT 2.0:
		*  "The following namespaces are designated as excluded
		*   namespaces:
		*  - The XSLT namespace URI
		*      (http://www.w3.org/1999/XSL/Transform)
		*  - A namespace URI declared as an extension namespace
		*  - A namespace URI designated by using an
		*      [xsl:]exclude-result-prefixes
		*
		* TODO:
		*  XSLT 1.0
		*   1) Supply all in-scope namespaces
		*   2) Skip excluded namespaces (see above)
		*   3) Apply namespace aliasing
		*
		*  XSLT 2.0 (will generate
		*            redundant namespaces in some cases):
		*   1) Supply all in-scope namespaces
		*   2) Skip excluded namespaces if *not* target-namespace
		*      of an namespace alias
		*   3) Apply namespace aliasing
		*
		* NOTE: See bug #341325.
		*/
#if 0		
		if ((templ != NULL) && (oldInsert == insert) &&
		    (ctxt->templ != NULL) &&
		    (ctxt->templ->inheritedNs != NULL)) {
		    int i;
		    xmlNsPtr ns, ret;
		    
		    for (i = 0; i < ctxt->templ->inheritedNsNr; i++) {
			const xmlChar *URI = NULL;
			xsltStylesheetPtr style;

			ns = ctxt->templ->inheritedNs[i];
			/*
			* Apply namespace aliasing.
			*
			* TODO: Compute the effective value of namespace
			*  aliases at compilation-time in order to avoid
			*  the lookup in the import-tree here.
			*/
			style = ctxt->style;
			while (style != NULL) {
			    if (style->nsAliases != NULL)
				URI = (const xmlChar *) 
				    xmlHashLookup(style->nsAliases, ns->href);
			    if (URI != NULL)
				break;
			    
			    style = xsltNextImport(style);
			}
			if (URI == UNDEFINED_DEFAULT_NS) {
			    xmlNsPtr defaultNs;

			    defaultNs = xmlSearchNs(cur->doc, cur, NULL);
			    if (defaultNs == NULL) {
				/*
				* TODO: Should not happen; i.e., it is
				*  an error at compilation-time if there's
				*  no default namespace in scope if
				*  "#default" is used.
				*/
				continue;
			    } else
				URI = defaultNs->href;
			}
			
			if (URI == NULL) {
			    /*
			    * There was no matching namespace-alias, so
			    * just create a matching ns-decl if not
			    * already in scope.
			    */
			    ret = xmlSearchNs(copy->doc, copy, ns->prefix);
			    if ((ret == NULL) ||
				(!xmlStrEqual(ret->href, ns->href)))
				xmlNewNs(copy, ns->href, ns->prefix);
			} else if (!xmlStrEqual(URI, XSLT_NAMESPACE)) {
			    ret = xmlSearchNs(copy->doc, copy, ns->prefix);
			    if ((ret == NULL) ||
				(!xmlStrEqual(ret->href, URI))) {
				/*
				* Here we create a namespace
				* declaration with the literal namespace
				* prefix and with the target namespace name.
				* TODO: We should consider to fix this and
				*  use the *target* namespace prefix, not the
				*  literal one (see bug #341325).
				*/
				xmlNewNs(copy, URI, ns->prefix);
			    }
			}
		    }
		    if (copy->ns != NULL) {
			/*
			* Fix the node namespace if needed
			*/
			copy->ns = xsltGetNamespace(ctxt, copy, copy->ns, copy);
		    }
		}
#endif
	    } else if (IS_XSLT_ELEM_FAST(cur)) {
		/*
		* XSLT instructions
		* --------------------------------------------------------
		*/
		if (info->type == XSLT_FUNC_UNKOWN_FORWARDS_COMPAT) {
		    /*
		    * We hit an unknown XSLT element.
		    * Try to apply one of the fallback cases.
		    */		
		    ctxt->insert = insert;
		    if (!xsltApplyFallbacks(ctxt, node, cur)) {
			xsltTransformError(ctxt, NULL, cur,
			    "The is no fallback behaviour defined for "
			    "the unknown XSLT element '%s'.\n",
			    cur->name);
		    }			
		    ctxt->insert = oldInsert;
		    goto skip_children;
		}
		/*
		* Execute the XSLT instruction.
		*/
		if (info->func != NULL) {
		    ctxt->insert = insert;
		    info->func(ctxt, node, cur, (xsltElemPreCompPtr) info);
		    ctxt->insert = oldInsert;
		    goto skip_children;
		}
		/*
		* Some XSLT instructions need custom execution.
		*/		 
		if (info->type == XSLT_FUNC_VARIABLE) {
		    if (level != 0) {
			/*
			* Build a new subframe and skip all the nodes
			* at that level.
			*/
			ctxt->insert = insert;
			xsltApplyOneTemplateInt(ctxt, node, cur, NULL, NULL, 0);
			while (cur->next != NULL)
			    cur = cur->next;
			ctxt->insert = oldInsert;
		    } else {
			xsltParseStylesheetVariable(ctxt, cur);
		    }
		} else if (info->type == XSLT_FUNC_PARAM) {
		    xsltParseStylesheetParam(ctxt, cur);
		} else if (info->type == XSLT_FUNC_MESSAGE) {
		    /*
		    * TODO: Won't be hit, since we don't compile xsl:message.
		    */
		    xsltMessage(ctxt, node, cur);
		} else {
		    xsltGenericError(xsltGenericErrorContext,
			"Internal error in xsltApplyOneTemplateInt(): "
			"Don't know how to process the XSLT element "
			"'%s'.\n", cur->name);			
		}
		goto skip_children;

	    } else {
		xsltTransformFunction func;
		/*
		* Extension intructions (elements)
		* --------------------------------------------------------
		*/				
		if (cur->psvi == xsltExtMarker) {
		    /*
		    * The xsltExtMarker was set during the compilation
		    * of extension instructions if there was no registered
		    * handler for this specific extension function at
		    * compile-time.
		    * Libxslt will now lookup if a handler is
		    * registered in the context of this transformation.
		    */
		    func = (xsltTransformFunction)
			xsltExtElementLookup(ctxt, cur->name, cur->ns->href);
		} else
		    func = ((xsltElemPreCompPtr) cur->psvi)->func;
		
		if (func == NULL) {
		    /*
		    * No handler available.
		    * Try to execute fallback behaviour via xsl:fallback.
		    */
#ifdef WITH_XSLT_DEBUG_PROCESS
		    XSLT_TRACE(ctxt, XSLT_TRACE_APPLY_TEMPLATE,
			xsltGenericDebug(xsltGenericDebugContext,
			    "xsltApplyOneTemplate: unknown extension %s\n",
			    cur->name));
#endif
		    ctxt->insert = insert;
		    if (!xsltApplyFallbacks(ctxt, node, cur)) {
			xsltTransformError(ctxt, NULL, cur,
			    "Unknown extension instruction '{%s}%s'.\n",
			    cur->ns->href, cur->name);
		    }			
		    ctxt->insert = oldInsert;		    
		} else {
		    /*
		    * Execute the handler-callback.
		    */
#ifdef WITH_XSLT_DEBUG_PROCESS
		    XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
			"xsltApplyOneTemplate: extension construct %s\n",
			cur->name));
#endif		    
		    ctxt->insert = insert;
		    func(ctxt, node, cur, cur->psvi);
		    ctxt->insert = oldInsert;
		}
		goto skip_children;
	    }

	} else if (XSLT_IS_TEXT_NODE(cur)) {
	    /*
	    * Text
	    * ------------------------------------------------------------
	    */
#ifdef WITH_XSLT_DEBUG_PROCESS
            if (cur->name == xmlStringTextNoenc) {
                XSLT_TRACE(ctxt, XSLT_TRACE_APPLY_TEMPLATE,
		    xsltGenericDebug(xsltGenericDebugContext,
		    "xsltApplyOneTemplateInt: copy unescaped text '%s'\n",
		    cur->content));
            } else {
                XSLT_TRACE(ctxt, XSLT_TRACE_APPLY_TEMPLATE,
		    xsltGenericDebug(xsltGenericDebugContext,
		    "xsltApplyOneTemplateInt: copy text '%s'\n",
		    cur->content));
            }
#endif
            if (xsltCopyText(ctxt, insert, cur, ctxt->internalized) == NULL)
		goto error;	    
	}

#else /* XSLT_REFACTORED */

        if (IS_XSLT_ELEM(cur)) {
            /*
             * This is an XSLT node
             */
            xsltStylePreCompPtr info = (xsltStylePreCompPtr) cur->psvi;

            if (info == NULL) {
                if (IS_XSLT_NAME(cur, "message")) {
                    xsltMessage(ctxt, node, cur);
                } else {
                    /*
                     * That's an error try to apply one of the fallback cases
                     */
                    ctxt->insert = insert;
                    if (!xsltApplyFallbacks(ctxt, node, cur)) {
                        xsltGenericError(xsltGenericErrorContext,
                                         "xsltApplyOneTemplate: %s was not compiled\n",
                                         cur->name);
                    }
                    ctxt->insert = oldInsert;
                }
                goto skip_children;
            }

            if (info->func != NULL) {
                ctxt->insert = insert;
                info->func(ctxt, node, cur, (xsltElemPreCompPtr) info);
                ctxt->insert = oldInsert;
                goto skip_children;
            }

            if (IS_XSLT_NAME(cur, "variable")) {
		if (level != 0) {
		    /*
		     * Build a new subframe and skip all the nodes
		     * at that level.
		     */
		    ctxt->insert = insert;
		    xsltApplyOneTemplateInt(ctxt, node, cur, NULL, NULL, 0);
		    while (cur->next != NULL)
			cur = cur->next;
		    ctxt->insert = oldInsert;
		} else {
		    xsltParseStylesheetVariable(ctxt, cur);
		}
            } else if (IS_XSLT_NAME(cur, "param")) {
                xsltParseStylesheetParam(ctxt, cur);
            } else if (IS_XSLT_NAME(cur, "message")) {
                xsltMessage(ctxt, node, cur);
            } else {
                xsltGenericError(xsltGenericErrorContext,
                                 "xsltApplyOneTemplate: problem with xsl:%s\n",
                                 cur->name);
            }
            goto skip_children;
        } else if ((cur->type == XML_TEXT_NODE) ||
                   (cur->type == XML_CDATA_SECTION_NODE)) {

            /*
             * This text comes from the stylesheet
             * For stylesheets, the set of whitespace-preserving
             * element names consists of just xsl:text.
             */
#ifdef WITH_XSLT_DEBUG_PROCESS
            if (cur->type == XML_CDATA_SECTION_NODE) {
                XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
                                 "xsltApplyOneTemplate: copy CDATA text %s\n",
                                 cur->content));
            } else if (cur->name == xmlStringTextNoenc) {
                XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
                                 "xsltApplyOneTemplate: copy unescaped text %s\n",
                                 cur->content));
            } else {
                XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
                                 "xsltApplyOneTemplate: copy text %s\n",
                                 cur->content));
            }
#endif
            if (xsltCopyText(ctxt, insert, cur, ctxt->internalized) == NULL)
		goto error;
        } else if ((cur->type == XML_ELEMENT_NODE) &&
                   (cur->ns != NULL) && (cur->psvi != NULL)) {
            xsltTransformFunction function;

            /*
             * Flagged as an extension element
             */
            if (cur->psvi == xsltExtMarker)
                function = (xsltTransformFunction)
                    xsltExtElementLookup(ctxt, cur->name, cur->ns->href);
            else
                function = ((xsltElemPreCompPtr) cur->psvi)->func;

            if (function == NULL) {
                xmlNodePtr child;
                int found = 0;

#ifdef WITH_XSLT_DEBUG_PROCESS
                XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
                                 "xsltApplyOneTemplate: unknown extension %s\n",
                                 cur->name));
#endif
                /*
                 * Search if there are fallbacks
                 */
                child = cur->children;
                while (child != NULL) {
                    if ((IS_XSLT_ELEM(child)) &&
                        (IS_XSLT_NAME(child, "fallback"))) {
                        found = 1;
                        xsltApplyOneTemplateInt(ctxt, node, child->children,
                                             NULL, NULL, 0);
                    }
                    child = child->next;
                }

                if (!found) {
                    xsltTransformError(ctxt, NULL, cur,
                                     "xsltApplyOneTemplate: failed to find extension %s\n",
                                     cur->name);
                }
            } else {
#ifdef WITH_XSLT_DEBUG_PROCESS
                XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
                                 "xsltApplyOneTemplate: extension construct %s\n",
                                 cur->name));
#endif

                ctxt->insert = insert;
                function(ctxt, node, cur, cur->psvi);
                ctxt->insert = oldInsert;
            }
            goto skip_children;
        } else if (cur->type == XML_ELEMENT_NODE) {
#ifdef WITH_XSLT_DEBUG_PROCESS
            XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
                             "xsltApplyOneTemplate: copy node %s\n",
                             cur->name));
#endif
            if ((copy = xsltCopyNode(ctxt, cur, insert)) == NULL)
		goto error;
            /*
             * all the attributes are directly inherited
             */
            if (cur->properties != NULL) {
                xsltAttrListTemplateProcess(ctxt, copy,
					    cur->properties);
            }
            /*
             * Add extra namespaces inherited from the current template
             * if we are in the first level children and this is a
	     * "real" template.	     
             */
            if ((templ != NULL) && (oldInsert == insert) &&
                (ctxt->templ != NULL) && (ctxt->templ->inheritedNs != NULL)) {
                int i;
                xmlNsPtr ns, ret;

                for (i = 0; i < ctxt->templ->inheritedNsNr; i++) {
		    const xmlChar *URI = NULL;
		    xsltStylesheetPtr style;
                    ns = ctxt->templ->inheritedNs[i];
		    style = ctxt->style;
		    while (style != NULL) {
		      if (style->nsAliases != NULL)
			URI = (const xmlChar *) 
			  xmlHashLookup(style->nsAliases, ns->href);
		      if (URI != NULL)
			break;
		      
		      style = xsltNextImport(style);
		    }
		    		    
		    if (URI == UNDEFINED_DEFAULT_NS) {
		      xmlNsPtr dflt;
		      dflt = xmlSearchNs(cur->doc, cur, NULL);
		      if (dflt == NULL)
		        continue;
		      else
		        URI = dflt->href;
		    }

		    if (URI == NULL) {
		      ret = xmlSearchNs(copy->doc, copy, ns->prefix);
		      if ((ret == NULL) ||
			  (!xmlStrEqual(ret->href, ns->href)))
			xmlNewNs(copy, ns->href, ns->prefix);
		    } else if (!xmlStrEqual(URI, XSLT_NAMESPACE)) {
		      ret = xmlSearchNs(copy->doc, copy, ns->prefix);
		      if ((ret == NULL) ||
			  (!xmlStrEqual(ret->href, URI)))
			xmlNewNs(copy, URI, ns->prefix);
		    }
                }
		if (copy->ns != NULL) {
		    /*
		     * Fix the node namespace if needed
		     */
		    copy->ns = xsltGetNamespace(ctxt, copy, copy->ns, copy);
		}
            }
        }
#endif /* else of XSLT_REFACTORED */

        /*
         * Descend into content in document order.
         */
        if (cur->children != NULL) {
            if (cur->children->type != XML_ENTITY_DECL) {
                cur = cur->children;
		level++;
                if (copy != NULL)
                    insert = copy;
                continue;
            }
        }

skip_children:
	/*
	* If xslt:message was just processed, we might have hit a
	* terminate='yes'; if so, then break the loop and clean up.
	* TODO: Do we need to check this also before trying to descend
	*  into the content?
	*/
	if (ctxt->state == XSLT_STATE_STOPPED)
	    break;
        if (cur->next != NULL) {
            cur = cur->next;
            continue;
        }

        do {
            cur = cur->parent;
	    level--;
            insert = insert->parent;
            if (cur == NULL)
                break;
            if (cur == list->parent) {
                cur = NULL;
                break;
            }
            if (cur->next != NULL) {
                cur = cur->next;
                break;
            }
        } while (cur != NULL);
    }
  error:
    ctxt->node = oldCurrent;
    ctxt->inst = oldInst;
    ctxt->insert = oldInsert;
    if (params == NULL)
        xsltFreeStackElemList(varsPop(ctxt));
    else {
        xsltStackElemPtr p, tmp = varsPop(ctxt);

        if (tmp != params) {
            p = tmp;
            while ((p != NULL) && (p->next != params))
                p = p->next;
            if (p == NULL) {
                xsltFreeStackElemList(tmp);
            } else {
                p->next = NULL;
                xsltFreeStackElemList(tmp);
            }
        }
    }
    if (templ != NULL) {
        ctxt->varsBase = oldBase;
	if (!notcur)
            templPop(ctxt);
	/*
	 * Free up all the unreferenced RVT
	 * Also set any global variables instantiated
	 * using them, to be "not yet computed".
	 */
	if (ctxt->tmpRVT != NULL) {
	    xsltStackElemPtr elem;
	    xmlDocPtr tmp = ctxt->tmpRVT, next;
            while (tmp != NULL) {
	        elem = (xsltStackElemPtr)tmp->psvi;
		if (elem != NULL) {
		    elem->computed = 0;
		    xmlXPathFreeObject(elem->value);
		}
	        next = (xmlDocPtr) tmp->next;
		if (tmp->_private != NULL) {
		    xsltFreeDocumentKeys(tmp->_private);
		    xmlFree(tmp->_private);
		}
		xmlFreeDoc(tmp);
		tmp = next;
	    }
	}
	ctxt->tmpRVT = tmpRVT;
        if (ctxt->profile) {
            long spent, child, total, end;

            end = xsltTimestamp();
            child = profPop(ctxt);
            total = end - start;
            spent = total - child;
            if (spent <= 0) {
                /*
                 * Not possible unless the original calibration failed
                 * we can try to correct it on the fly.
                 */
                xsltCalibrateAdjust(spent);
                spent = 0;
            }

            templ->time += spent;
            if (ctxt->profNr > 0)
                ctxt->profTab[ctxt->profNr - 1] += total;
        }
    }
#ifdef WITH_DEBUGGER
    if ((ctxt->debugStatus != XSLT_DEBUG_NONE) && (addCallResult)) {
        xslDropCall();
    }
#endif
}

/************************************************************************
 *									*
 *		    XSLT-1.1 extensions					*
 *									*
 ************************************************************************/

/**
 * xsltDocumentElem:
 * @ctxt:  an XSLT processing context
 * @node:  The current node
 * @inst:  the instruction in the stylesheet
 * @comp:  precomputed information
 *
 * Process an EXSLT/XSLT-1.1 document element
 */
void
xsltDocumentElem(xsltTransformContextPtr ctxt, xmlNodePtr node,
                 xmlNodePtr inst, xsltStylePreCompPtr castedComp)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemDocumentPtr comp = (xsltStyleItemDocumentPtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xsltStylesheetPtr style = NULL;
    int ret;
    xmlChar *filename = NULL, *prop, *elements;
    xmlChar *element, *end;
    xmlDocPtr res = NULL;
    xmlDocPtr oldOutput;
    xmlNodePtr oldInsert, root;
    const char *oldOutputFile;
    xsltOutputType oldType;
    xmlChar *URL = NULL;
    const xmlChar *method;
    const xmlChar *doctypePublic;
    const xmlChar *doctypeSystem;
    const xmlChar *version;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
        return;

    if (comp->filename == NULL) {

        if (xmlStrEqual(inst->name, (const xmlChar *) "output")) {
	    /*
	    * The element "output" is in the namespace XSLT_SAXON_NAMESPACE
	    *   (http://icl.com/saxon)
	    * The @file is in no namespace.
	    */
#ifdef WITH_XSLT_DEBUG_EXTRA
            xsltGenericDebug(xsltGenericDebugContext,
                             "Found saxon:output extension\n");
#endif
            URL = xsltEvalAttrValueTemplate(ctxt, inst,
                                                 (const xmlChar *) "file",
                                                 XSLT_SAXON_NAMESPACE);
	     
	    if (URL == NULL)
		URL = xsltEvalAttrValueTemplate(ctxt, inst,
                                                 (const xmlChar *) "href",
                                                 XSLT_SAXON_NAMESPACE);
        } else if (xmlStrEqual(inst->name, (const xmlChar *) "write")) {
#ifdef WITH_XSLT_DEBUG_EXTRA
            xsltGenericDebug(xsltGenericDebugContext,
                             "Found xalan:write extension\n");
#endif
            URL = xsltEvalAttrValueTemplate(ctxt, inst,
                                                 (const xmlChar *)
                                                 "select",
                                                 XSLT_XALAN_NAMESPACE);
	    if (URL != NULL) {
		xmlXPathCompExprPtr cmp;
		xmlChar *val;

		/*
		 * Trying to handle bug #59212
		 * The value of the "select" attribute is an
		 * XPath expression.
		 * (see http://xml.apache.org/xalan-j/extensionslib.html#redirect) 
		 */
		cmp = xmlXPathCompile(URL);
                val = xsltEvalXPathString(ctxt, cmp);
		xmlXPathFreeCompExpr(cmp);
		xmlFree(URL);
		URL = val;
	    }
	    if (URL == NULL)
		URL = xsltEvalAttrValueTemplate(ctxt, inst,
						     (const xmlChar *)
						     "file",
						     XSLT_XALAN_NAMESPACE);
	    if (URL == NULL)
		URL = xsltEvalAttrValueTemplate(ctxt, inst,
						     (const xmlChar *)
						     "href",
						     XSLT_XALAN_NAMESPACE);
        } else if (xmlStrEqual(inst->name, (const xmlChar *) "document")) {
            URL = xsltEvalAttrValueTemplate(ctxt, inst,
                                                 (const xmlChar *) "href",
                                                 NULL);
        }

    } else {
        URL = xmlStrdup(comp->filename);
    }

    if (URL == NULL) {
	xsltTransformError(ctxt, NULL, inst,
		         "xsltDocumentElem: href/URI-Reference not found\n");
	return;
    }

    /*
     * If the computation failed, it's likely that the URL wasn't escaped
     */
    filename = xmlBuildURI(URL, (const xmlChar *) ctxt->outputFile);
    if (filename == NULL) {
	xmlChar *escURL;

	escURL=xmlURIEscapeStr(URL, BAD_CAST ":/.?,");
	if (escURL != NULL) {
	    filename = xmlBuildURI(escURL, (const xmlChar *) ctxt->outputFile);
	    xmlFree(escURL);
	}
    }

    if (filename == NULL) {
	xsltTransformError(ctxt, NULL, inst,
		         "xsltDocumentElem: URL computation failed for %s\n",
			 URL);
	xmlFree(URL);
	return;
    }

    /*
     * Security checking: can we write to this resource
     */
    if (ctxt->sec != NULL) {
	ret = xsltCheckWrite(ctxt->sec, ctxt, filename);
	if (ret == 0) {
	    xsltTransformError(ctxt, NULL, inst,
		 "xsltDocumentElem: write rights for %s denied\n",
			     filename);
	    xmlFree(URL);
	    xmlFree(filename);
	    return;
	}
    }

    oldOutputFile = ctxt->outputFile;
    oldOutput = ctxt->output;
    oldInsert = ctxt->insert;
    oldType = ctxt->type;
    ctxt->outputFile = (const char *) filename;

    style = xsltNewStylesheet();
    if (style == NULL) {
	xsltTransformError(ctxt, NULL, inst,
                         "xsltDocumentElem: out of memory\n");
        goto error;
    }

    /*
     * Version described in 1.1 draft allows full parameterization
     * of the output.
     */
    prop = xsltEvalAttrValueTemplate(ctxt, inst,
				     (const xmlChar *) "version",
				     NULL);
    if (prop != NULL) {
	if (style->version != NULL)
	    xmlFree(style->version);
	style->version = prop;
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst,
				     (const xmlChar *) "encoding",
				     NULL);
    if (prop != NULL) {
	if (style->encoding != NULL)
	    xmlFree(style->encoding);
	style->encoding = prop;
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst,
				     (const xmlChar *) "method",
				     NULL);
    if (prop != NULL) {
	const xmlChar *URI;

	if (style->method != NULL)
	    xmlFree(style->method);
	style->method = NULL;
	if (style->methodURI != NULL)
	    xmlFree(style->methodURI);
	style->methodURI = NULL;

	URI = xsltGetQNameURI(inst, &prop);
	if (prop == NULL) {
	    if (style != NULL) style->errors++;
	} else if (URI == NULL) {
	    if ((xmlStrEqual(prop, (const xmlChar *) "xml")) ||
		(xmlStrEqual(prop, (const xmlChar *) "html")) ||
		(xmlStrEqual(prop, (const xmlChar *) "text"))) {
		style->method = prop;
	    } else {
		xsltTransformError(ctxt, NULL, inst,
				 "invalid value for method: %s\n", prop);
		if (style != NULL) style->warnings++;
	    }
	} else {
	    style->method = prop;
	    style->methodURI = xmlStrdup(URI);
	}
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst,
				     (const xmlChar *)
				     "doctype-system", NULL);
    if (prop != NULL) {
	if (style->doctypeSystem != NULL)
	    xmlFree(style->doctypeSystem);
	style->doctypeSystem = prop;
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst,
				     (const xmlChar *)
				     "doctype-public", NULL);
    if (prop != NULL) {
	if (style->doctypePublic != NULL)
	    xmlFree(style->doctypePublic);
	style->doctypePublic = prop;
    }
    prop = xsltEvalAttrValueTemplate(ctxt, inst,
				     (const xmlChar *) "standalone",
				     NULL);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *) "yes")) {
	    style->standalone = 1;
	} else if (xmlStrEqual(prop, (const xmlChar *) "no")) {
	    style->standalone = 0;
	} else {
	    xsltTransformError(ctxt, NULL, inst,
			     "invalid value for standalone: %s\n",
			     prop);
	    if (style != NULL) style->warnings++;
	}
	xmlFree(prop);
    }

    prop = xsltEvalAttrValueTemplate(ctxt, inst,
				     (const xmlChar *) "indent",
				     NULL);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *) "yes")) {
	    style->indent = 1;
	} else if (xmlStrEqual(prop, (const xmlChar *) "no")) {
	    style->indent = 0;
	} else {
	    xsltTransformError(ctxt, NULL, inst,
			     "invalid value for indent: %s\n", prop);
	    if (style != NULL) style->warnings++;
	}
	xmlFree(prop);
    }

    prop = xsltEvalAttrValueTemplate(ctxt, inst,
				     (const xmlChar *)
				     "omit-xml-declaration",
				     NULL);
    if (prop != NULL) {
	if (xmlStrEqual(prop, (const xmlChar *) "yes")) {
	    style->omitXmlDeclaration = 1;
	} else if (xmlStrEqual(prop, (const xmlChar *) "no")) {
	    style->omitXmlDeclaration = 0;
	} else {
	    xsltTransformError(ctxt, NULL, inst,
			     "invalid value for omit-xml-declaration: %s\n",
			     prop);
	    if (style != NULL) style->warnings++;
	}
	xmlFree(prop);
    }

    elements = xsltEvalAttrValueTemplate(ctxt, inst,
					 (const xmlChar *)
					 "cdata-section-elements",
					 NULL);
    if (elements != NULL) {
	if (style->stripSpaces == NULL)
	    style->stripSpaces = xmlHashCreate(10);
	if (style->stripSpaces == NULL)
	    return;

	element = elements;
	while (*element != 0) {
	    while (IS_BLANK_CH(*element))
		element++;
	    if (*element == 0)
		break;
	    end = element;
	    while ((*end != 0) && (!IS_BLANK_CH(*end)))
		end++;
	    element = xmlStrndup(element, end - element);
	    if (element) {
		const xmlChar *URI;

#ifdef WITH_XSLT_DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
				 "add cdata section output element %s\n",
				 element);
#endif
                URI = xsltGetQNameURI(inst, &element);

		xmlHashAddEntry2(style->stripSpaces, element, URI,
			        (xmlChar *) "cdata");
		xmlFree(element);
	    }
	    element = end;
	}
	xmlFree(elements);
    }

    /*
     * Create a new document tree and process the element template
     */
    XSLT_GET_IMPORT_PTR(method, style, method)
    XSLT_GET_IMPORT_PTR(doctypePublic, style, doctypePublic)
    XSLT_GET_IMPORT_PTR(doctypeSystem, style, doctypeSystem)
    XSLT_GET_IMPORT_PTR(version, style, version)

    if ((method != NULL) &&
	(!xmlStrEqual(method, (const xmlChar *) "xml"))) {
	if (xmlStrEqual(method, (const xmlChar *) "html")) {
	    ctxt->type = XSLT_OUTPUT_HTML;
	    if (((doctypePublic != NULL) || (doctypeSystem != NULL)))
		res = htmlNewDoc(doctypeSystem, doctypePublic);
	    else {
		if (version != NULL) {
#ifdef XSLT_GENERATE_HTML_DOCTYPE
		    xsltGetHTMLIDs(version, &doctypePublic, &doctypeSystem);
#endif
                }
		res = htmlNewDocNoDtD(doctypeSystem, doctypePublic);
	    }
	    if (res == NULL)
		goto error;
	    res->dict = ctxt->dict;
	    xmlDictReference(res->dict);
	} else if (xmlStrEqual(method, (const xmlChar *) "xhtml")) {
	    xsltTransformError(ctxt, NULL, inst,
	     "xsltDocumentElem: unsupported method xhtml\n",
		             style->method);
	    ctxt->type = XSLT_OUTPUT_HTML;
	    res = htmlNewDocNoDtD(doctypeSystem, doctypePublic);
	    if (res == NULL)
		goto error;
	    res->dict = ctxt->dict;
	    xmlDictReference(res->dict);
	} else if (xmlStrEqual(method, (const xmlChar *) "text")) {
	    ctxt->type = XSLT_OUTPUT_TEXT;
	    res = xmlNewDoc(style->version);
	    if (res == NULL)
		goto error;
	    res->dict = ctxt->dict;
	    xmlDictReference(res->dict);
#ifdef WITH_XSLT_DEBUG
	    xsltGenericDebug(xsltGenericDebugContext,
                     "reusing transformation dict for output\n");
#endif
	} else {
	    xsltTransformError(ctxt, NULL, inst,
			     "xsltDocumentElem: unsupported method %s\n",
		             style->method);
	    goto error;
	}
    } else {
	ctxt->type = XSLT_OUTPUT_XML;
	res = xmlNewDoc(style->version);
	if (res == NULL)
	    goto error;
	res->dict = ctxt->dict;
	xmlDictReference(res->dict);
#ifdef WITH_XSLT_DEBUG
	xsltGenericDebug(xsltGenericDebugContext,
                     "reusing transformation dict for output\n");
#endif
    }
    res->charset = XML_CHAR_ENCODING_UTF8;
    if (style->encoding != NULL)
	res->encoding = xmlStrdup(style->encoding);
    ctxt->output = res;
    ctxt->insert = (xmlNodePtr) res;
    xsltApplyOneTemplateInt(ctxt, node, inst->children, NULL, NULL, 0);

    /*
     * Do some post processing work depending on the generated output
     */
    root = xmlDocGetRootElement(res);
    if (root != NULL) {
        const xmlChar *doctype = NULL;

        if ((root->ns != NULL) && (root->ns->prefix != NULL))
	    doctype = xmlDictQLookup(ctxt->dict, root->ns->prefix, root->name);
	if (doctype == NULL)
	    doctype = root->name;

        /*
         * Apply the default selection of the method
         */
        if ((method == NULL) &&
            (root->ns == NULL) &&
            (!xmlStrcasecmp(root->name, (const xmlChar *) "html"))) {
            xmlNodePtr tmp;

            tmp = res->children;
            while ((tmp != NULL) && (tmp != root)) {
                if (tmp->type == XML_ELEMENT_NODE)
                    break;
                if ((tmp->type == XML_TEXT_NODE) && (!xmlIsBlankNode(tmp)))
                    break;
		tmp = tmp->next;
            }
            if (tmp == root) {
                ctxt->type = XSLT_OUTPUT_HTML;
                res->type = XML_HTML_DOCUMENT_NODE;
                if (((doctypePublic != NULL) || (doctypeSystem != NULL))) {
                    res->intSubset = xmlCreateIntSubset(res, doctype,
                                                        doctypePublic,
                                                        doctypeSystem);
#ifdef XSLT_GENERATE_HTML_DOCTYPE
		} else if (version != NULL) {
                    xsltGetHTMLIDs(version, &doctypePublic,
                                   &doctypeSystem);
                    if (((doctypePublic != NULL) || (doctypeSystem != NULL)))
                        res->intSubset =
                            xmlCreateIntSubset(res, doctype,
                                               doctypePublic,
                                               doctypeSystem);
#endif
                }
            }

        }
        if (ctxt->type == XSLT_OUTPUT_XML) {
            XSLT_GET_IMPORT_PTR(doctypePublic, style, doctypePublic)
                XSLT_GET_IMPORT_PTR(doctypeSystem, style, doctypeSystem)
                if (((doctypePublic != NULL) || (doctypeSystem != NULL)))
                res->intSubset = xmlCreateIntSubset(res, doctype,
                                                    doctypePublic,
                                                    doctypeSystem);
        }
    }

    /*
     * Save the result
     */
    ret = xsltSaveResultToFilename((const char *) filename,
                                   res, style, 0);
    if (ret < 0) {
	xsltTransformError(ctxt, NULL, inst,
                         "xsltDocumentElem: unable to save to %s\n",
                         filename);
	ctxt->state = XSLT_STATE_ERROR;
#ifdef WITH_XSLT_DEBUG_EXTRA
    } else {
        xsltGenericDebug(xsltGenericDebugContext,
                         "Wrote %d bytes to %s\n", ret, filename);
#endif
    }

  error:
    ctxt->output = oldOutput;
    ctxt->insert = oldInsert;
    ctxt->type = oldType;
    ctxt->outputFile = oldOutputFile;
    if (URL != NULL)
        xmlFree(URL);
    if (filename != NULL)
        xmlFree(filename);
    if (style != NULL)
        xsltFreeStylesheet(style);
    if (res != NULL)
        xmlFreeDoc(res);
}

/************************************************************************
 *									*
 *		Most of the XSLT-1.0 transformations			*
 *									*
 ************************************************************************/

/**
 * xsltSort:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt sort node
 * @comp:  precomputed information
 *
 * function attached to xsl:sort nodes, but this should not be
 * called directly
 */
void
xsltSort(xsltTransformContextPtr ctxt,
	xmlNodePtr node ATTRIBUTE_UNUSED, xmlNodePtr inst,
	xsltStylePreCompPtr comp) {
    if (comp == NULL) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:sort : compilation failed\n");
	return;
    }
    xsltTransformError(ctxt, NULL, inst,
	 "xsl:sort : improper use this should not be reached\n");
}

/**
 * xsltCopy:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt copy node
 * @comp:  precomputed information
 *
 * Execute the xsl:copy instruction on the source node.
 */
void
xsltCopy(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr castedComp) {
#ifdef XSLT_REFACTORED
    xsltStyleItemCopyPtr comp = (xsltStyleItemCopyPtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xmlNodePtr copy, oldInsert;
   
    oldInsert = ctxt->insert;
    if (ctxt->insert != NULL) {
	switch (node->type) {
	    case XML_TEXT_NODE:
	    case XML_CDATA_SECTION_NODE:
		/*
		 * This text comes from the stylesheet
		 * For stylesheets, the set of whitespace-preserving
		 * element names consists of just xsl:text.
		 */
#ifdef WITH_XSLT_DEBUG_PROCESS
		if (node->type == XML_CDATA_SECTION_NODE) {
		    XSLT_TRACE(ctxt,XSLT_TRACE_COPY,xsltGenericDebug(xsltGenericDebugContext,
			 "xsltCopy: CDATA text %s\n", node->content));
		} else {
		    XSLT_TRACE(ctxt,XSLT_TRACE_COPY,xsltGenericDebug(xsltGenericDebugContext,
			 "xsltCopy: text %s\n", node->content));
                }
#endif
		xsltCopyText(ctxt, ctxt->insert, node, 0);
		break;
	    case XML_DOCUMENT_NODE:
	    case XML_HTML_DOCUMENT_NODE:
		break;
	    case XML_ELEMENT_NODE:
		/*
		* NOTE: The "fake" is a doc-node, not an element node.
		* OLD:
		*   if (xmlStrEqual(node->name, BAD_CAST " fake node libxslt"))
		*    return;
		*/		

#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_COPY,xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: node %s\n", node->name));
#endif
		copy = xsltCopyNode(ctxt, node, ctxt->insert);
		ctxt->insert = copy;
		if (comp->use != NULL) {
		    xsltApplyAttributeSet(ctxt, node, inst, comp->use);
		}
		break;
	    case XML_ATTRIBUTE_NODE: {
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_COPY,xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: attribute %s\n", node->name));
#endif
		if (ctxt->insert->type == XML_ELEMENT_NODE) {
		    xmlAttrPtr attr = (xmlAttrPtr) node, ret = NULL, cur;

		    if (attr->ns != NULL) {
			if (!xmlStrEqual(attr->ns->href, XSLT_NAMESPACE)) {
			    ret = xmlCopyProp(ctxt->insert, attr);
			    ret->ns = xsltGetNamespace(ctxt, node, attr->ns,
						       ctxt->insert);
			} 
		    } else
			ret = xmlCopyProp(ctxt->insert, attr);

		    if (ret != NULL) {
			cur = ctxt->insert->properties;
			if (cur != NULL) {
			    /*
			     * Avoid duplicates and insert at the end
			     * of the attribute list
			     */
			    while (cur->next != NULL) {
				if ((xmlStrEqual(cur->name, ret->name)) &&
                                    (((cur->ns == NULL) && (ret->ns == NULL)) ||
				     ((cur->ns != NULL) && (ret->ns != NULL) &&
				      (xmlStrEqual(cur->ns->href,
						   ret->ns->href))))) {
				    xmlFreeProp(ret);
				    return;
				}
				cur = cur->next;
			    }
			    if ((xmlStrEqual(cur->name, ret->name)) &&
				(((cur->ns == NULL) && (ret->ns == NULL)) ||
				 ((cur->ns != NULL) && (ret->ns != NULL) &&
				  (xmlStrEqual(cur->ns->href,
					       ret->ns->href))))) {
				xmlNodePtr tmp;

				/*
				 * Attribute already exists,
				 * update it with the new value
				 */
				tmp = cur->children;
				cur->children = ret->children;
				ret->children = tmp;
				tmp = cur->last;
				cur->last = ret->last;
				ret->last = tmp;
				xmlFreeProp(ret);
				return;
			    }
			    cur->next = ret;
			    ret->prev = cur;
			} else
			    ctxt->insert->properties = ret;
		    }
		}
		break;
	    }
	    case XML_PI_NODE:
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_COPY,xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: PI %s\n", node->name));
#endif
		copy = xmlNewDocPI(ctxt->insert->doc, node->name,
		                   node->content);
		xmlAddChild(ctxt->insert, copy);
		break;
	    case XML_COMMENT_NODE:
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_COPY,xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: comment\n"));
#endif
		copy = xmlNewComment(node->content);
		xmlAddChild(ctxt->insert, copy);
		break;
	    case XML_NAMESPACE_DECL:
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_COPY,xsltGenericDebug(xsltGenericDebugContext,
				 "xsltCopy: namespace declaration\n"));
#endif
                xsltCopyNamespace(ctxt, ctxt->insert, (xmlNsPtr)node);
		break;
	    default:
		break;

	}
    }

    switch (node->type) {
	case XML_DOCUMENT_NODE:
	case XML_HTML_DOCUMENT_NODE:
	case XML_ELEMENT_NODE:
	    xsltApplyOneTemplateInt(ctxt, ctxt->node, inst->children,
		                 NULL, NULL, 0);
	    break;
	default:
	    break;
    }
    ctxt->insert = oldInsert;
}

/**
 * xsltText:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt text node
 * @comp:  precomputed information
 *
 * Process the xslt text node on the source node
 */
void
xsltText(xsltTransformContextPtr ctxt, xmlNodePtr node ATTRIBUTE_UNUSED,
	    xmlNodePtr inst, xsltStylePreCompPtr comp ATTRIBUTE_UNUSED) {
    if ((inst->children != NULL) && (comp != NULL)) {
	xmlNodePtr text = inst->children;
	xmlNodePtr copy;

	while (text != NULL) {
	    if ((text->type != XML_TEXT_NODE) &&
	         (text->type != XML_CDATA_SECTION_NODE)) {
		xsltTransformError(ctxt, NULL, inst,
				 "xsl:text content problem\n");
		break;
	    }
	    copy = xmlNewDocText(ctxt->output, text->content);
	    if (text->type != XML_CDATA_SECTION_NODE) {
#ifdef WITH_XSLT_DEBUG_PARSING
		xsltGenericDebug(xsltGenericDebugContext,
		     "Disable escaping: %s\n", text->content);
#endif
		copy->name = xmlStringTextNoenc;
	    }
	    xmlAddChild(ctxt->insert, copy);
	    text = text->next;
	}
    }
}

/**
 * xsltElement:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt element node
 * @comp:  precomputed information
 *
 * Process the xslt element node on the source node
 */
void
xsltElement(xsltTransformContextPtr ctxt, xmlNodePtr node,
	    xmlNodePtr inst, xsltStylePreCompPtr castedComp) {
#ifdef XSLT_REFACTORED
    xsltStyleItemElementPtr comp = (xsltStyleItemElementPtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xmlChar *prop = NULL, *attributes = NULL, *namespace;
    const xmlChar *name;
    const xmlChar *prefix;
    xmlNsPtr ns = NULL, oldns = NULL;
    xmlNodePtr copy;
    xmlNodePtr oldInsert;
    int generateDefault = 0;


    if (ctxt->insert == NULL)
	return;
    if (!comp->has_name) {
	return;
    }

    /*
     * stack and saves
     */
    oldInsert = ctxt->insert;

    if (comp->name == NULL) {
	prop = xsltEvalAttrValueTemplate(ctxt, inst,
		      (const xmlChar *)"name", NULL);
	if (prop == NULL) {
	    xsltTransformError(ctxt, NULL, inst,
		 "xsl:element : name is missing\n");
	    return;
	}
	if (xmlValidateQName(prop, 0)) {
	    xsltTransformError(ctxt, NULL, inst,
		    "xsl:element : invalid name\n");
	    /* we fall through to catch any other errors if possible */
	}
	name = xsltSplitQName(ctxt->dict, prop, &prefix);
	xmlFree(prop);
    } else {
	name = xsltSplitQName(ctxt->dict, comp->name, &prefix);
    }

    /*
     * Create the new element
     */
    if (ctxt->output->dict == ctxt->dict) {
	copy = xmlNewDocNodeEatName(ctxt->output, NULL, (xmlChar *)name, NULL);
    } else {
	copy = xmlNewDocNode(ctxt->output, NULL, (xmlChar *)name, NULL);
    }
    if (copy == NULL) {
	xsltTransformError(ctxt, NULL, inst,
	    "xsl:element : creation of %s failed\n", name);
	return;
    }
    xmlAddChild(ctxt->insert, copy);
    ctxt->insert = copy;

    if ((comp->ns == NULL) && (comp->has_ns)) {
	namespace = xsltEvalAttrValueTemplate(ctxt, inst,
		(const xmlChar *)"namespace", NULL);
	if (namespace != NULL) {
	    ns = xsltGetSpecialNamespace(ctxt, inst, namespace, prefix,
		                         ctxt->insert);
	    xmlFree(namespace);
	}
    } else if ((comp->ns != NULL) && (prefix == NULL) && (comp->has_ns)) {
	generateDefault = 1;
    } else if (comp->ns != NULL) {
	ns = xsltGetSpecialNamespace(ctxt, inst, comp->ns, prefix,
				     ctxt->insert);
    }
    if ((ns == NULL) && (prefix != NULL)) {
	if (!xmlStrncasecmp(prefix, (xmlChar *)"xml", 3)) {
#ifdef WITH_XSLT_DEBUG_PARSING
	    xsltGenericDebug(xsltGenericDebugContext,
		 "xsltElement: xml prefix forbidden\n");
#endif
	    return;
	}
	oldns = xmlSearchNs(inst->doc, inst, prefix);
	if (oldns == NULL) {
	    xsltTransformError(ctxt, NULL, inst,
		"xsl:element : no namespace bound to prefix %s\n", prefix);
	} else {
	    ns = xsltGetNamespace(ctxt, inst, oldns, ctxt->insert);
	}
    }

    if (generateDefault == 1) {
	xmlNsPtr defaultNs = NULL;

	if ((oldInsert != NULL) && (oldInsert->type == XML_ELEMENT_NODE))
	    defaultNs = xmlSearchNs(oldInsert->doc, oldInsert, NULL);
	if ((defaultNs == NULL) || (!xmlStrEqual(defaultNs->href, comp->ns))) {
	    ns = xmlNewNs(ctxt->insert, comp->ns, NULL);
	    ctxt->insert->ns = ns;
	} else {
	    ctxt->insert->ns = defaultNs;
	}
    } else if ((ns == NULL) && (oldns != NULL)) {
	/* very specific case xsltGetNamespace failed */
        ns = xmlNewNs(ctxt->insert, oldns->href, oldns->prefix);
	ctxt->insert->ns = ns;
    } else
        ctxt->insert->ns = ns;


    if (comp->has_use) {
	if (comp->use != NULL) {
	    xsltApplyAttributeSet(ctxt, node, inst, comp->use);
	} else {
	    /*
	    * BUG TODO: use-attribute-sets is not a value template.
	    *  use-attribute-sets = qnames
	    */
	    attributes = xsltEvalAttrValueTemplate(ctxt, inst,
		       (const xmlChar *)"use-attribute-sets", NULL);
	    if (attributes != NULL) {
		xsltApplyAttributeSet(ctxt, node, inst, attributes);
		xmlFree(attributes);
	    }
	}
    }
    
    xsltApplyOneTemplateInt(ctxt, ctxt->node, inst->children, NULL, NULL, 0);

    ctxt->insert = oldInsert;
}


/**
 * xsltComment:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt comment node
 * @comp:  precomputed information
 *
 * Process the xslt comment node on the source node
 */
void
xsltComment(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr comp ATTRIBUTE_UNUSED) {
    xmlChar *value = NULL;
    xmlNodePtr commentNode;
    int len;
    
    value = xsltEvalTemplateString(ctxt, node, inst);
    /* TODO: use or generate the compiled form */
    len = xmlStrlen(value);
    if (len > 0) {
        if ((value[len-1] == '-') || 
	    (xmlStrstr(value, BAD_CAST "--"))) {
	    xsltTransformError(ctxt, NULL, inst,
	    	    "xsl:comment : '--' or ending '-' not allowed in comment\n");
	    /* fall through to try to catch further errors */
	}
    }
#ifdef WITH_XSLT_DEBUG_PROCESS
    if (value == NULL) {
	XSLT_TRACE(ctxt,XSLT_TRACE_COMMENT,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltComment: empty\n"));
    } else {
	XSLT_TRACE(ctxt,XSLT_TRACE_COMMENT,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltComment: content %s\n", value));
    }
#endif

    commentNode = xmlNewComment(value);
    xmlAddChild(ctxt->insert, commentNode);

    if (value != NULL)
	xmlFree(value);
}

/**
 * xsltProcessingInstruction:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt processing-instruction node
 * @comp:  precomputed information
 *
 * Process the xslt processing-instruction node on the source node
 */
void
xsltProcessingInstruction(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr castedComp) {
#ifdef XSLT_REFACTORED
    xsltStyleItemPIPtr comp = (xsltStyleItemPIPtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    const xmlChar *name;
    xmlChar *value = NULL;
    xmlNodePtr pi;


    if (ctxt->insert == NULL)
	return;
    if (comp->has_name == 0)
	return;
    if (comp->name == NULL) {
	name = xsltEvalAttrValueTemplate(ctxt, inst,
			    (const xmlChar *)"name", NULL);
	if (name == NULL) {
	    xsltTransformError(ctxt, NULL, inst,
		 "xsl:processing-instruction : name is missing\n");
	    goto error;
	}
    } else {
	name = comp->name;
    }
    /* TODO: check that it's both an an NCName and a PITarget. */


    value = xsltEvalTemplateString(ctxt, node, inst);
    if (xmlStrstr(value, BAD_CAST "?>") != NULL) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:processing-instruction: '?>' not allowed within PI content\n");
	goto error;
    }
#ifdef WITH_XSLT_DEBUG_PROCESS
    if (value == NULL) {
	XSLT_TRACE(ctxt,XSLT_TRACE_PI,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessingInstruction: %s empty\n", name));
    } else {
	XSLT_TRACE(ctxt,XSLT_TRACE_PI,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltProcessingInstruction: %s content %s\n", name, value));
    }
#endif

    pi = xmlNewDocPI(ctxt->insert->doc, name, value);
    xmlAddChild(ctxt->insert, pi);

error:
    if ((name != NULL) && (name != comp->name))
        xmlFree((xmlChar *) name);
    if (value != NULL)
	xmlFree(value);
}

/**
 * xsltCopyOf:
 * @ctxt:  an XSLT transformation context
 * @node:  the current node in the source tree
 * @inst:  the element node of the XSLT copy-of instruction 
 * @comp:  precomputed information of the XSLT copy-of instruction
 *
 * Process the XSLT copy-of instruction.
 */
void
xsltCopyOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr castedComp) {
#ifdef XSLT_REFACTORED
    xsltStyleItemCopyOfPtr comp = (xsltStyleItemCopyOfPtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xmlXPathObjectPtr res = NULL;
    xmlNodeSetPtr list = NULL;
    int i;
    int oldProximityPosition, oldContextSize;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;
    if ((comp == NULL) || (comp->select == NULL) || (comp->comp == NULL)) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:copy-of : compilation failed\n");
	return;
    }

     /*
    * SPEC XSLT 1.0:
    *  "The xsl:copy-of element can be used to insert a result tree
    *  fragment into the result tree, without first converting it to
    *  a string as xsl:value-of does (see [7.6.1 Generating Text with
    *  xsl:value-of]). The required select attribute contains an
    *  expression. When the result of evaluating the expression is a
    *  result tree fragment, the complete fragment is copied into the
    *  result tree. When the result is a node-set, all the nodes in the
    *  set are copied in document order into the result tree; copying
    *  an element node copies the attribute nodes, namespace nodes and
    *  children of the element node as well as the element node itself;
    *  a root node is copied by copying its children. When the result
    *  is neither a node-set nor a result tree fragment, the result is
    *  converted to a string and then inserted into the result tree,
    *  as with xsl:value-of.
    */

#ifdef WITH_XSLT_DEBUG_PROCESS
    XSLT_TRACE(ctxt,XSLT_TRACE_COPY_OF,xsltGenericDebug(xsltGenericDebugContext,
	 "xsltCopyOf: select %s\n", comp->select));
#endif

    /*
    * Set up the XPath evaluation context.
    */
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    ctxt->xpathCtxt->node = node;
#ifdef XSLT_REFACTORED
    if (comp->inScopeNs != NULL) {
	ctxt->xpathCtxt->namespaces = comp->inScopeNs->list;
	ctxt->xpathCtxt->nsNr = comp->inScopeNs->number;
    } else {
	ctxt->xpathCtxt->namespaces = NULL;
	ctxt->xpathCtxt->nsNr = 0;
    }
#else
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
#endif
    /*
    * Evaluate the "select" expression.
    */
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    /*
    * Revert the XPath evaluation context to previous state.
    */
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;

    if (res != NULL) {
	if (res->type == XPATH_NODESET) {
	    /*
	    * Node-set
	    * --------
	    */
#ifdef WITH_XSLT_DEBUG_PROCESS
	    XSLT_TRACE(ctxt,XSLT_TRACE_COPY_OF,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltCopyOf: result is a node set\n"));
#endif
	    list = res->nodesetval;
	    if (list != NULL) {
		xmlNodePtr cur;
		/*
		* The list is already sorted in document order by XPath.
		* Append everything in this order under ctxt->insert.
		*/
		for (i = 0;i < list->nodeNr;i++) {
		    cur = list->nodeTab[i];
		    if (cur == NULL)
			continue;
		    if ((cur->type == XML_DOCUMENT_NODE) ||
			(cur->type == XML_HTML_DOCUMENT_NODE))
		    {
			xsltCopyTreeList(ctxt, cur->children, ctxt->insert, 0);
		    } else if (cur->type == XML_ATTRIBUTE_NODE) {
			xsltCopyProp(ctxt, ctxt->insert, (xmlAttrPtr) cur);
		    } else {
			xsltCopyTree(ctxt, cur, ctxt->insert, 0);
		    }
		}
	    }
	} else if (res->type == XPATH_XSLT_TREE) {
	    /*
	    * Result tree fragment
	    * --------------------
	    */
#ifdef WITH_XSLT_DEBUG_PROCESS
	    XSLT_TRACE(ctxt,XSLT_TRACE_COPY_OF,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltCopyOf: result is a result tree fragment\n"));
#endif
	    /*
	    * TODO: Is list->nodeTab[0] is an xmlDocPtr?
	    */
	    list = res->nodesetval;
	    if ((list != NULL) && (list->nodeTab != NULL) &&
		(list->nodeTab[0] != NULL) &&
		(IS_XSLT_REAL_NODE(list->nodeTab[0])))
	    {
		xsltCopyTreeList(ctxt, list->nodeTab[0]->children,
			         ctxt->insert, 0);
	    }
	} else {
	    /* Convert to a string. */
	    res = xmlXPathConvertString(res);
	    if ((res != NULL) && (res->type == XPATH_STRING)) {
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_COPY_OF,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltCopyOf: result %s\n", res->stringval));
#endif
		/* Append content as text node. */
		xsltCopyTextString(ctxt, ctxt->insert, res->stringval, 0);
	    }
	}
    } else {
	ctxt->state = XSLT_STATE_STOPPED;
    }

    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltValueOf:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt value-of node
 * @comp:  precomputed information
 *
 * Process the xslt value-of node on the source node
 */
void
xsltValueOf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr castedComp)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemValueOfPtr comp = (xsltStyleItemValueOfPtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr copy = NULL;
    int oldProximityPosition, oldContextSize;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;
    if ((comp == NULL) || (comp->select == NULL) || (comp->comp == NULL)) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:value-of : compilation failed\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    XSLT_TRACE(ctxt,XSLT_TRACE_VALUE_OF,xsltGenericDebug(xsltGenericDebugContext,
	 "xsltValueOf: select %s\n", comp->select));
#endif

    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    ctxt->xpathCtxt->node = node;
#ifdef XSLT_REFACTORED
    if (comp->inScopeNs != NULL) {
	ctxt->xpathCtxt->namespaces = comp->inScopeNs->list;
	ctxt->xpathCtxt->nsNr = comp->inScopeNs->number;
    } else {
	ctxt->xpathCtxt->namespaces = NULL;
	ctxt->xpathCtxt->nsNr = 0;
    }
#else
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
#endif
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    if (res != NULL) {
	if (res->type != XPATH_STRING)
	    res = xmlXPathConvertString(res);
	if (res->type == XPATH_STRING) {
	    copy = xsltCopyTextString(ctxt, ctxt->insert, res->stringval,
		               comp->noescape);
	}
    } else {
	ctxt->state = XSLT_STATE_STOPPED;
    }
    if (copy == NULL) {
	if ((res == NULL) || (res->stringval != NULL)) {
	    xsltTransformError(ctxt, NULL, inst,
		"xsltValueOf: text copy failed\n");
	}
    }
#ifdef WITH_XSLT_DEBUG_PROCESS
    else
	XSLT_TRACE(ctxt,XSLT_TRACE_VALUE_OF,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltValueOf: result %s\n", res->stringval));
#endif
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltNumber:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt number node
 * @comp:  precomputed information
 *
 * Process the xslt number node on the source node
 */
void
xsltNumber(xsltTransformContextPtr ctxt, xmlNodePtr node,
	   xmlNodePtr inst, xsltStylePreCompPtr castedComp)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemNumberPtr comp = (xsltStyleItemNumberPtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    if (comp == NULL) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:number : compilation failed\n");
	return;
    }

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

    comp->numdata.doc = inst->doc;
    comp->numdata.node = inst;
    
    xsltNumberFormat(ctxt, &comp->numdata, node);
}

/**
 * xsltApplyImports:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt apply-imports node
 * @comp:  precomputed information
 *
 * Process the xslt apply-imports node on the source node
 */
void
xsltApplyImports(xsltTransformContextPtr ctxt, xmlNodePtr node,
	         xmlNodePtr inst,
		 xsltStylePreCompPtr comp ATTRIBUTE_UNUSED) {
    xsltTemplatePtr template;

    if ((ctxt->templ == NULL) || (ctxt->templ->style == NULL)) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:apply-imports : internal error no current template\n");
	return;
    }
    template = xsltGetTemplate(ctxt, node, ctxt->templ->style);
    if (template != NULL) {
	xsltApplyOneTemplateInt(ctxt, node, template->content, template, NULL, 0);
    }
}

/**
 * xsltCallTemplate:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt call-template node
 * @comp:  precomputed information
 *
 * Process the xslt call-template node on the source node
 */
void
xsltCallTemplate(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr castedComp)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemCallTemplatePtr comp =
	(xsltStyleItemCallTemplatePtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xmlNodePtr cur = NULL;
    xsltStackElemPtr params = NULL, param;

    if (ctxt->insert == NULL)
	return;
    if (comp == NULL) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:call-template : compilation failed\n");
	return;
    }

    /*
     * The template must have been precomputed
     */
    if (comp->templ == NULL) {
	comp->templ = xsltFindTemplate(ctxt, comp->name, comp->ns);
	if (comp->templ == NULL) {
	    if (comp->ns != NULL) {
	        xsltTransformError(ctxt, NULL, inst,
			"xsl:call-template : template %s:%s not found\n",
			comp->ns, comp->name);
	    } else {
	        xsltTransformError(ctxt, NULL, inst,
			"xsl:call-template : template %s not found\n",
			comp->name);
	    }
	    return;
	}
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    if ((comp != NULL) && (comp->name != NULL))
	XSLT_TRACE(ctxt,XSLT_TRACE_CALL_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
			 "call-template: name %s\n", comp->name));
#endif

    cur = inst->children;
    while (cur != NULL) {
#ifdef WITH_DEBUGGER
        if (ctxt->debugStatus != XSLT_DEBUG_NONE)
            xslHandleDebugger(cur, node, comp->templ, ctxt);
#endif
	if (ctxt->state == XSLT_STATE_STOPPED) break;
	/*
	* TODO: The "with-param"s could be part of the "call-template"
	*   structure. Avoid to "search" for params dynamically
	*   in the XML tree every time.
	*/
#ifdef XSLT_REFACTORED
	if (IS_XSLT_ELEM_FAST(cur)) {
#else
	if (IS_XSLT_ELEM(cur)) {
#endif
	    if (IS_XSLT_NAME(cur, "with-param")) {
		param = xsltParseStylesheetCallerParam(ctxt, cur);
		if (param != NULL) {
		    param->next = params;
		    params = param;
		}
	    } else {
		xsltGenericError(xsltGenericErrorContext,
		     "xsl:call-template: misplaced xsl:%s\n", cur->name);
	    }
	} else {
	    xsltGenericError(xsltGenericErrorContext,
		 "xsl:call-template: misplaced %s element\n", cur->name);
	}
	cur = cur->next;
    }
    /*
     * Create a new frame using the params first
     * Set the "notcur" flag to abide by Section 5.6 of the spec
     */
    xsltApplyOneTemplateInt(ctxt, node, comp->templ->content, comp->templ, params, 1);
    if (params != NULL)
	xsltFreeStackElemList(params);

#ifdef WITH_XSLT_DEBUG_PROCESS
    if ((comp != NULL) && (comp->name != NULL))
	XSLT_TRACE(ctxt,XSLT_TRACE_CALL_TEMPLATE,xsltGenericDebug(xsltGenericDebugContext,
			 "call-template returned: name %s\n", comp->name));
#endif
}

/**
 * xsltApplyTemplates:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the apply-templates node
 * @comp:  precomputed information
 *
 * Process the apply-templates node on the source node
 */
void
xsltApplyTemplates(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr castedComp)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemApplyTemplatesPtr comp =
	(xsltStyleItemApplyTemplatesPtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xmlNodePtr cur, delete = NULL, oldNode;
    xmlXPathObjectPtr res = NULL;
    xmlNodeSetPtr list = NULL, oldList;
    int i, oldProximityPosition, oldContextSize;
    const xmlChar *oldMode, *oldModeURI;
    xsltStackElemPtr params = NULL, param;
    int nbsorts = 0;
    xmlNodePtr sorts[XSLT_MAX_SORT];
    xmlDocPtr oldXDocPtr;
    xsltDocumentPtr oldCDocPtr;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if (comp == NULL) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:apply-templates : compilation failed\n");
	return;
    }
    if ((ctxt == NULL) || (node == NULL) || (inst == NULL) || (comp == NULL))
	return;

#ifdef WITH_XSLT_DEBUG_PROCESS
    if ((node != NULL) && (node->name != NULL))
	XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATES,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: node: %s\n", node->name));
#endif

    /*
     * Get mode if any
     */
    oldNode = ctxt->node;
    oldMode = ctxt->mode;
    oldModeURI = ctxt->modeURI;
    ctxt->mode = comp->mode;
    ctxt->modeURI = comp->modeURI;

    /*
     * The xpath context size and proximity position, as
     * well as the xpath and context documents, may be changed
     * so we save their initial state and will restore on exit
     */
    oldXDocPtr = ctxt->xpathCtxt->doc;
    oldCDocPtr = ctxt->document;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    oldList = ctxt->nodeList;

    if (comp->select != NULL) {
	if (comp->comp == NULL) {
	    xsltTransformError(ctxt, NULL, inst,
		 "xsl:apply-templates : compilation failed\n");
	    goto error;
	}
#ifdef WITH_XSLT_DEBUG_PROCESS
	XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATES,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: select %s\n", comp->select));
#endif

	ctxt->xpathCtxt->node = node;
#ifdef XSLT_REFACTORED
	if (comp->inScopeNs != NULL) {
	    ctxt->xpathCtxt->namespaces = comp->inScopeNs->list;
	    ctxt->xpathCtxt->nsNr = comp->inScopeNs->number;
	} else {
	    ctxt->xpathCtxt->namespaces = NULL;
	    ctxt->xpathCtxt->nsNr = 0;
	}
#else
	ctxt->xpathCtxt->namespaces = comp->nsList;
	ctxt->xpathCtxt->nsNr = comp->nsNr;
#endif
	res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
	ctxt->xpathCtxt->contextSize = oldContextSize;
	ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	if (res != NULL) {
	    if (res->type == XPATH_NODESET) {
		list = res->nodesetval;
		res->nodesetval = NULL;
		/*
		 In order to take care of potential keys we need to
		 do some extra work in the case of an RVT converted
		 into a nodeset (e.g. exslt:node-set())
		 We create a "pseudo-doc" (if not already created) and
		 store it's pointer into _private.  This doc, together
		 with the keyset, will be freed when the RVT is freed.
	        */
		if ((list != NULL) && (ctxt->document->keys != NULL)) {
		    if ((list->nodeNr != 0) &&
		        (list->nodeTab[0]->doc != NULL) &&

		        XSLT_IS_RES_TREE_FRAG(list->nodeTab[0]->doc) &&

			(list->nodeTab[0]->doc->_private == NULL)) {
			    list->nodeTab[0]->doc->_private = xsltNewDocument(
			    	ctxt, list->nodeTab[0]->doc);
			if (list->nodeTab[0]->doc->_private == NULL) {
			    xsltTransformError(ctxt, NULL, inst,
		    "xsltApplyTemplates : failed to allocate subdoc\n");
		        }

			ctxt->document = list->nodeTab[0]->doc->_private;
		    }

		}
	     } else {
		list = NULL;
	     }
	} else {
	    ctxt->state = XSLT_STATE_STOPPED;
	}
	if (list == NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
	    XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATES,xsltGenericDebug(xsltGenericDebugContext,
		"xsltApplyTemplates: select didn't evaluate to a node list\n"));
#endif
	    goto error;
	}
    } else {
	/*
	 * Build an XPath nodelist with the children
	 */
	list = xmlXPathNodeSetCreate(NULL);
	cur = node->children;
	while (cur != NULL) {
	    switch (cur->type) {
		case XML_TEXT_NODE:
		    if ((IS_BLANK_NODE(cur)) &&
			(cur->parent != NULL) &&
			(cur->parent->type == XML_ELEMENT_NODE) &&
			(ctxt->style->stripSpaces != NULL)) {
			const xmlChar *val;

			if (cur->parent->ns != NULL) {
			    val = (const xmlChar *)
				  xmlHashLookup2(ctxt->style->stripSpaces,
						 cur->parent->name,
						 cur->parent->ns->href);
			    if (val == NULL) {
				val = (const xmlChar *)
				  xmlHashLookup2(ctxt->style->stripSpaces,
						 BAD_CAST "*",
						 cur->parent->ns->href);
			    }
			} else {
			    val = (const xmlChar *)
				  xmlHashLookup2(ctxt->style->stripSpaces,
						 cur->parent->name, NULL);
			}
			if ((val != NULL) &&
			    (xmlStrEqual(val, (xmlChar *) "strip"))) {
			    delete = cur;
			    break;
			}
		    }
		    /* no break on purpose */
		case XML_ELEMENT_NODE:
		case XML_DOCUMENT_NODE:
		case XML_HTML_DOCUMENT_NODE:
		case XML_CDATA_SECTION_NODE:
		case XML_PI_NODE:
		case XML_COMMENT_NODE:
		    xmlXPathNodeSetAddUnique(list, cur);
		    break;
		case XML_DTD_NODE:
		    /* Unlink the DTD, it's still reachable
		     * using doc->intSubset */
		    if (cur->next != NULL)
			cur->next->prev = cur->prev;
		    if (cur->prev != NULL)
			cur->prev->next = cur->next;
		    break;
		default:
#ifdef WITH_XSLT_DEBUG_PROCESS
		    XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATES,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyTemplates: skipping cur type %d\n",
				     cur->type));
#endif
		    delete = cur;
	    }
	    cur = cur->next;
	    if (delete != NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATES,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltApplyTemplates: removing ignorable blank cur\n"));
#endif
		xmlUnlinkNode(delete);
		xmlFreeNode(delete);
		delete = NULL;
	    }
	}
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    if (list != NULL)
    XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATES,xsltGenericDebug(xsltGenericDebugContext,
	"xsltApplyTemplates: list of %d nodes\n", list->nodeNr));
#endif

    ctxt->nodeList = list;
    ctxt->xpathCtxt->contextSize = list->nodeNr;

    /* 
     * handle (or skip) the xsl:sort and xsl:with-param
     */
    cur = inst->children;
    while (cur!=NULL) {
#ifdef WITH_DEBUGGER
        if (ctxt->debugStatus != XSLT_DEBUG_NONE)
#ifdef XSLT_REFACTORED
            xslHandleDebugger(cur, node, NULL, ctxt);
#else
	    /* TODO: Isn't comp->templ always NULL for apply-template? */
            xslHandleDebugger(cur, node, comp->templ, ctxt);
#endif
#endif
        if (ctxt->state == XSLT_STATE_STOPPED) break;
#ifdef XSLT_REFACTORED
	if (IS_XSLT_ELEM_FAST(cur)) {
#else
        if (IS_XSLT_ELEM(cur)) {
#endif
            if (IS_XSLT_NAME(cur, "with-param")) {
                param = xsltParseStylesheetCallerParam(ctxt, cur);
		if (param != NULL) {
		    param->next = params;
		    params = param;
		}
	    } else if (IS_XSLT_NAME(cur, "sort")) {
		if (nbsorts >= XSLT_MAX_SORT) {
		    xsltGenericError(xsltGenericErrorContext,
			"xsl:apply-template: %s too many sort\n", node->name);
		} else {
		    sorts[nbsorts++] = cur;
		}
	    } else {
		xsltGenericError(xsltGenericErrorContext,
		    "xsl:apply-template: misplaced xsl:%s\n", cur->name);
	    }
        } else {
            xsltGenericError(xsltGenericErrorContext,
                 "xsl:apply-template: misplaced %s element\n", cur->name);
        }
        cur = cur->next;
    }

    if (nbsorts > 0) {
	xsltDoSortFunction(ctxt, sorts, nbsorts);
    }

    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	/* For a 'select' nodeset, need to check if document has changed */
	if ((IS_XSLT_REAL_NODE(list->nodeTab[i])) &&
	    (list->nodeTab[i]->doc!=NULL) &&
	    (list->nodeTab[i]->doc->doc!=NULL) &&
	    (list->nodeTab[i]->doc->doc)!=ctxt->xpathCtxt->doc) {	  
	    /* The nodeset is from another document, so must change */
	    ctxt->xpathCtxt->doc=list->nodeTab[i]->doc->doc;
	    if ((list->nodeTab[i]->doc->name != NULL) ||
		(list->nodeTab[i]->doc->URL != NULL)) {
		ctxt->document = xsltFindDocument(ctxt,
			            list->nodeTab[i]->doc->doc);
		if (ctxt->document == NULL) {
		    /* restore the previous context */
		    ctxt->document = oldCDocPtr;
		}
		ctxt->xpathCtxt->node = list->nodeTab[i];
#ifdef WITH_XSLT_DEBUG_PROCESS
		if ((ctxt->document != NULL) &&
		    (ctxt->document->doc != NULL)) {
		    XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATES,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltApplyTemplates: Changing document - context doc %s, xpathdoc %s\n",
		   ctxt->document->doc->URL, ctxt->xpathCtxt->doc->URL));
		} else {
		    XSLT_TRACE(ctxt,XSLT_TRACE_APPLY_TEMPLATES,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyTemplates: Changing document - Return tree fragment\n"));
		}
#endif
	    }
	}
	xsltProcessOneNode(ctxt, list->nodeTab[i], params);
    }
error:
    if (params != NULL)
	xsltFreeStackElemList(params);	/* free the parameter list */
    if (list != NULL)
	xmlXPathFreeNodeSet(list);
    /*
     * res must be deallocated after list
     */
    if (res != NULL)
	xmlXPathFreeObject(res);

    ctxt->nodeList = oldList;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->doc = oldXDocPtr;
    ctxt->document = oldCDocPtr;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;

    ctxt->node = oldNode;
    ctxt->mode = oldMode;
    ctxt->modeURI = oldModeURI;
}


/**
 * xsltChoose:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt choose node
 * @comp:  precomputed information
 *
 * Process the xslt choose node on the source node
 */
void
xsltChoose(xsltTransformContextPtr ctxt, xmlNodePtr node,
	   xmlNodePtr inst, xsltStylePreCompPtr comp ATTRIBUTE_UNUSED)
{
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr replacement, when;
    int doit = 1;
    int oldProximityPosition, oldContextSize;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;

    /* 
     * Check the when's
     */
    replacement = inst->children;
    if (replacement == NULL) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:choose: empty content not allowed\n");
	goto error;
    }
#ifdef XSLT_REFACTORED
    if (((!IS_XSLT_ELEM_FAST(replacement)) ||
#else
    if (((!IS_XSLT_ELEM(replacement)) ||
#endif
	(!IS_XSLT_NAME(replacement, "when")))
	    && (!xmlIsBlankNode(replacement))) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:choose: xsl:when expected first\n");
	goto error;
    }
#ifdef XSLT_REFACTORED
    while ((IS_XSLT_ELEM_FAST(replacement) &&
#else
    while ((IS_XSLT_ELEM(replacement) &&
#endif    
	(IS_XSLT_NAME(replacement, "when")))
	    || xmlIsBlankNode(replacement)) {
#ifdef XSLT_REFACTORED
	xsltStyleItemWhenPtr wcomp =
	    (xsltStyleItemWhenPtr) replacement->psvi;
#else
	xsltStylePreCompPtr wcomp = replacement->psvi;
#endif

	if (xmlIsBlankNode(replacement)) {
	    replacement = replacement->next;
	    continue;
	}
	
	if ((wcomp == NULL) || (wcomp->test == NULL) || (wcomp->comp == NULL)) {
	    xsltTransformError(ctxt, NULL, inst,
		 "xsl:choose: compilation failed !\n");
	    goto error;
	}
	when = replacement;


#ifdef WITH_DEBUGGER
        if (xslDebugStatus != XSLT_DEBUG_NONE)
#ifdef XSLT_REFACTORED
            xslHandleDebugger(when, node, NULL, ctxt);
#else
	    /* TODO: Isn't comp->templ always NULL for xsl:choose? */
            xslHandleDebugger(when, node, comp->templ, ctxt);
#endif
#endif

#ifdef WITH_XSLT_DEBUG_PROCESS
	XSLT_TRACE(ctxt,XSLT_TRACE_CHOOSE,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltChoose: test %s\n", wcomp->test));
#endif

	oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
	oldContextSize = ctxt->xpathCtxt->contextSize;
	oldNsNr = ctxt->xpathCtxt->nsNr;
	oldNamespaces = ctxt->xpathCtxt->namespaces;
  	ctxt->xpathCtxt->node = node;
#ifdef XSLT_REFACTORED
	if (wcomp->inScopeNs != NULL) {
	    ctxt->xpathCtxt->namespaces = wcomp->inScopeNs->list;
	    ctxt->xpathCtxt->nsNr = wcomp->inScopeNs->number;
	} else {
	    ctxt->xpathCtxt->namespaces = NULL;
	    ctxt->xpathCtxt->nsNr = 0;
	}
#else
	ctxt->xpathCtxt->namespaces = wcomp->nsList;
	ctxt->xpathCtxt->nsNr = wcomp->nsNr;
#endif
  	res = xmlXPathCompiledEval(wcomp->comp, ctxt->xpathCtxt);
	ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	ctxt->xpathCtxt->contextSize = oldContextSize;
	ctxt->xpathCtxt->nsNr = oldNsNr;
	ctxt->xpathCtxt->namespaces = oldNamespaces;
	if (res != NULL) {
	    if (res->type != XPATH_BOOLEAN)
		res = xmlXPathConvertBoolean(res);
	    if (res->type == XPATH_BOOLEAN)
		doit = res->boolval;
	    else {
#ifdef WITH_XSLT_DEBUG_PROCESS
		XSLT_TRACE(ctxt,XSLT_TRACE_CHOOSE,xsltGenericDebug(xsltGenericDebugContext,
		    "xsltChoose: test didn't evaluate to a boolean\n"));
#endif
		goto error;
	    }
	} else {
	    ctxt->state = XSLT_STATE_STOPPED;
	}

#ifdef WITH_XSLT_DEBUG_PROCESS
	XSLT_TRACE(ctxt,XSLT_TRACE_CHOOSE,xsltGenericDebug(xsltGenericDebugContext,
	    "xsltChoose: test evaluate to %d\n", doit));
#endif
	if (doit) {
	    xsltApplyOneTemplateInt(ctxt, ctxt->node, when->children,
		                 NULL, NULL, 0);
	    goto done;
	}
	if (res != NULL)
	    xmlXPathFreeObject(res);
	res = NULL;
	replacement = replacement->next;
    }
#ifdef XSLT_REFACTORED
    if (IS_XSLT_ELEM_FAST(replacement) &&
#else
    if (IS_XSLT_ELEM(replacement) &&
#endif	
	(IS_XSLT_NAME(replacement, "otherwise"))) {
#ifdef WITH_DEBUGGER
        if (xslDebugStatus != XSLT_DEBUG_NONE)
#ifdef XSLT_REFACTORED
            xslHandleDebugger(replacement, node, NULL, ctxt);
#else
	    /* TODO: Isn't comp->templ always NULL for xsl:otherwise? */
            xslHandleDebugger(replacement, node, comp->templ, ctxt);
#endif
#endif

#ifdef WITH_XSLT_DEBUG_PROCESS
	XSLT_TRACE(ctxt,XSLT_TRACE_CHOOSE,xsltGenericDebug(xsltGenericDebugContext,
			 "evaluating xsl:otherwise\n"));
#endif
	xsltApplyOneTemplateInt(ctxt, ctxt->node, replacement->children,
		             NULL, NULL, 0);
	replacement = replacement->next;
    }
    while (xmlIsBlankNode(replacement)) {
	replacement = replacement->next;
    }
    if (replacement != NULL) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:choose: unexpected content %s\n", replacement->name);
	goto error;
    }

done:
error:
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltIf:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt if node
 * @comp:  precomputed information
 *
 * Process the xslt if node on the source node
 */
void
xsltIf(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr castedComp){
#ifdef XSLT_REFACTORED
	xsltStyleItemIfPtr comp = (xsltStyleItemIfPtr) castedComp;
#else
	xsltStylePreCompPtr comp = castedComp;
#endif
    xmlXPathObjectPtr res = NULL;
    int doit = 1;
    int oldContextSize, oldProximityPosition;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;
    if ((comp == NULL) || (comp->test == NULL) || (comp->comp == NULL)) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:if : compilation failed\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    XSLT_TRACE(ctxt,XSLT_TRACE_IF,xsltGenericDebug(xsltGenericDebugContext,
	 "xsltIf: test %s\n", comp->test));
#endif

    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    ctxt->xpathCtxt->node = node;
#ifdef XSLT_REFACTORED
    if (comp->inScopeNs != NULL) {
	ctxt->xpathCtxt->namespaces = comp->inScopeNs->list;
	ctxt->xpathCtxt->nsNr = comp->inScopeNs->number;
    } else {
	ctxt->xpathCtxt->namespaces = NULL;
	ctxt->xpathCtxt->nsNr = 0;
    }
#else
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
#endif
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    if (res != NULL) {
	if (res->type != XPATH_BOOLEAN)
	    res = xmlXPathConvertBoolean(res);
	if (res->type == XPATH_BOOLEAN)
	    doit = res->boolval;
	else {
#ifdef WITH_XSLT_DEBUG_PROCESS
	    XSLT_TRACE(ctxt,XSLT_TRACE_IF,xsltGenericDebug(xsltGenericDebugContext,
		"xsltIf: test didn't evaluate to a boolean\n"));
#endif
	    goto error;
	}
    } else {
	ctxt->state = XSLT_STATE_STOPPED;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    XSLT_TRACE(ctxt,XSLT_TRACE_IF,xsltGenericDebug(xsltGenericDebugContext,
	"xsltIf: test evaluate to %d\n", doit));
#endif
    if (doit) {
	xsltApplyOneTemplateInt(ctxt, node, inst->children, NULL, NULL, 0);
    }

error:
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/**
 * xsltForEach:
 * @ctxt:  a XSLT process context
 * @node:  the node in the source tree.
 * @inst:  the xslt for-each node
 * @comp:  precomputed information
 *
 * Process the xslt for-each node on the source node
 */
void
xsltForEach(xsltTransformContextPtr ctxt, xmlNodePtr node,
	           xmlNodePtr inst, xsltStylePreCompPtr castedComp)
{
#ifdef XSLT_REFACTORED
	xsltStyleItemForEachPtr comp = (xsltStyleItemForEachPtr) castedComp;
#else
	xsltStylePreCompPtr comp = castedComp;
#endif
    xmlXPathObjectPtr res = NULL;
    xmlNodePtr replacement;
    xmlNodeSetPtr list = NULL, oldList;
    int i, oldProximityPosition, oldContextSize;
    xmlNodePtr oldNode;
    int nbsorts = 0;
    xmlNodePtr sorts[XSLT_MAX_SORT];
    xmlDocPtr oldXDocPtr;
    xsltDocumentPtr oldCDocPtr;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (node == NULL) || (inst == NULL))
	return;
    if ((comp == NULL) || (comp->select == NULL) || (comp->comp == NULL)) {
	xsltTransformError(ctxt, NULL, inst,
	     "xsl:for-each : compilation failed\n");
	return;
    }
    oldNode = ctxt->node;

#ifdef WITH_XSLT_DEBUG_PROCESS
    XSLT_TRACE(ctxt,XSLT_TRACE_FOR_EACH,xsltGenericDebug(xsltGenericDebugContext,
	 "xsltForEach: select %s\n", comp->select));
#endif

    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    ctxt->xpathCtxt->node = node;
#ifdef XSLT_REFACTORED
    if (comp->inScopeNs != NULL) {
	ctxt->xpathCtxt->namespaces = comp->inScopeNs->list;
	ctxt->xpathCtxt->nsNr = comp->inScopeNs->number;
    } else {
	ctxt->xpathCtxt->namespaces = NULL;
	ctxt->xpathCtxt->nsNr = 0;
    }
#else
    ctxt->xpathCtxt->namespaces = comp->nsList;
    ctxt->xpathCtxt->nsNr = comp->nsNr;
#endif   
    oldCDocPtr = ctxt->document;
    oldXDocPtr = ctxt->xpathCtxt->doc;
    res = xmlXPathCompiledEval(comp->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    if (res != NULL) {
	if (res->type == XPATH_NODESET)
	    list = res->nodesetval;
    } else {
	ctxt->state = XSLT_STATE_STOPPED;
    }
    if (list == NULL) {
#ifdef WITH_XSLT_DEBUG_PROCESS
	XSLT_TRACE(ctxt,XSLT_TRACE_FOR_EACH,xsltGenericDebug(xsltGenericDebugContext,
	    "xsltForEach: select didn't evaluate to a node list\n"));
#endif
	goto error;
    }

#ifdef WITH_XSLT_DEBUG_PROCESS
    XSLT_TRACE(ctxt,XSLT_TRACE_FOR_EACH,xsltGenericDebug(xsltGenericDebugContext,
	"xsltForEach: select evaluates to %d nodes\n", list->nodeNr));
#endif

    oldList = ctxt->nodeList;
    ctxt->nodeList = list;
    oldContextSize = ctxt->xpathCtxt->contextSize;
    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
    ctxt->xpathCtxt->contextSize = list->nodeNr;

    /* 
     * handle and skip the xsl:sort
     */
    replacement = inst->children;
#ifdef XSLT_REFACTORED
    while (IS_XSLT_ELEM_FAST(replacement) &&
#else
    while (IS_XSLT_ELEM(replacement) &&
#endif
	(IS_XSLT_NAME(replacement, "sort"))) {
	if (nbsorts >= XSLT_MAX_SORT) {
	    xsltGenericError(xsltGenericErrorContext,
		"xsl:for-each: too many sorts\n");
	} else {
	    sorts[nbsorts++] = replacement;
	}
#ifdef WITH_DEBUGGER
        if (xslDebugStatus != XSLT_DEBUG_NONE)
            xslHandleDebugger(replacement, node, NULL, ctxt);
#endif
	replacement = replacement->next;
    }

    if (nbsorts > 0) {
	xsltDoSortFunction(ctxt, sorts, nbsorts);
    }


    for (i = 0;i < list->nodeNr;i++) {
	ctxt->node = list->nodeTab[i];
	ctxt->xpathCtxt->proximityPosition = i + 1;
	/* For a 'select' nodeset, need to check if document has changed */
	if ((IS_XSLT_REAL_NODE(list->nodeTab[i])) &&
	    (list->nodeTab[i]->doc!=NULL) &&
	    (list->nodeTab[i]->doc->doc!=NULL) &&
	    (list->nodeTab[i]->doc->doc)!=ctxt->xpathCtxt->doc) {	  
	    /* The nodeset is from another document, so must change */
	    ctxt->xpathCtxt->doc=list->nodeTab[i]->doc->doc;
	    if ((list->nodeTab[i]->doc->name != NULL) ||
		(list->nodeTab[i]->doc->URL != NULL)) {
		ctxt->document = xsltFindDocument(ctxt,
			            list->nodeTab[i]->doc->doc);
		if (ctxt->document == NULL) {
		    /* restore the previous context */
		    ctxt->document = oldCDocPtr;
		}
		ctxt->xpathCtxt->node = list->nodeTab[i];
#ifdef WITH_XSLT_DEBUG_PROCESS
		if ((ctxt->document != NULL) &&
		    (ctxt->document->doc != NULL)) {
		    XSLT_TRACE(ctxt,XSLT_TRACE_FOR_EACH,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltForEach: Changing document - context doc %s, xpathdoc %s\n",
		 ctxt->document->doc->URL, ctxt->xpathCtxt->doc->URL));
		} else {
		    XSLT_TRACE(ctxt,XSLT_TRACE_FOR_EACH,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltForEach: Changing document - Return tree fragment\n"));
		}
#endif
	    }
	}
	xsltApplyOneTemplateInt(ctxt, list->nodeTab[i], replacement, NULL, NULL, 0);
    }
    ctxt->document = oldCDocPtr;
    ctxt->nodeList = oldList;
    ctxt->node = oldNode;
    ctxt->xpathCtxt->doc = oldXDocPtr;
    ctxt->xpathCtxt->contextSize = oldContextSize;
    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;

error:
    if (res != NULL)
	xmlXPathFreeObject(res);
}

/************************************************************************
 *									*
 *			Generic interface				*
 *									*
 ************************************************************************/

#ifdef XSLT_GENERATE_HTML_DOCTYPE
typedef struct xsltHTMLVersion {
    const char *version;
    const char *public;
    const char *system;
} xsltHTMLVersion;

static xsltHTMLVersion xsltHTMLVersions[] = {
    { "4.01frame", "-//W3C//DTD HTML 4.01 Frameset//EN",
      "http://www.w3.org/TR/1999/REC-html401-19991224/frameset.dtd"},
    { "4.01strict", "-//W3C//DTD HTML 4.01//EN",
      "http://www.w3.org/TR/1999/REC-html401-19991224/strict.dtd"},
    { "4.01trans", "-//W3C//DTD HTML 4.01 Transitional//EN",
      "http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd"},
    { "4.01", "-//W3C//DTD HTML 4.01 Transitional//EN",
      "http://www.w3.org/TR/1999/REC-html401-19991224/loose.dtd"},
    { "4.0strict", "-//W3C//DTD HTML 4.01//EN",
      "http://www.w3.org/TR/html4/strict.dtd"},
    { "4.0trans", "-//W3C//DTD HTML 4.01 Transitional//EN",
      "http://www.w3.org/TR/html4/loose.dtd"},
    { "4.0frame", "-//W3C//DTD HTML 4.01 Frameset//EN",
      "http://www.w3.org/TR/html4/frameset.dtd"},
    { "4.0", "-//W3C//DTD HTML 4.01 Transitional//EN",
      "http://www.w3.org/TR/html4/loose.dtd"},
    { "3.2", "-//W3C//DTD HTML 3.2//EN", NULL }
};

/**
 * xsltGetHTMLIDs:
 * @version:  the version string
 * @publicID:  used to return the public ID
 * @systemID:  used to return the system ID
 *
 * Returns -1 if not found, 0 otherwise and the system and public
 *         Identifier for this given verion of HTML
 */
static int
xsltGetHTMLIDs(const xmlChar *version, const xmlChar **publicID,
	            const xmlChar **systemID) {
    unsigned int i;
    if (version == NULL)
	return(-1);
    for (i = 0;i < (sizeof(xsltHTMLVersions)/sizeof(xsltHTMLVersions[1]));
	 i++) {
	if (!xmlStrcasecmp(version,
		           (const xmlChar *) xsltHTMLVersions[i].version)) {
	    if (publicID != NULL)
		*publicID = (const xmlChar *) xsltHTMLVersions[i].public;
	    if (systemID != NULL)
		*systemID = (const xmlChar *) xsltHTMLVersions[i].system;
	    return(0);
	}
    }
    return(-1);
}
#endif

/**
 * xsltApplyStripSpaces:
 * @ctxt:  a XSLT process context
 * @node:  the root of the XML tree
 *
 * Strip the unwanted ignorable spaces from the input tree
 */
void
xsltApplyStripSpaces(xsltTransformContextPtr ctxt, xmlNodePtr node) {
    xmlNodePtr current;
#ifdef WITH_XSLT_DEBUG_PROCESS
    int nb = 0;
#endif


    current = node;
    while (current != NULL) {
	/*
	 * Cleanup children empty nodes if asked for
	 */
	if ((IS_XSLT_REAL_NODE(current)) &&
	    (current->children != NULL) &&
	    (xsltFindElemSpaceHandling(ctxt, current))) {
	    xmlNodePtr delete = NULL, cur = current->children;

	    while (cur != NULL) {
		if (IS_BLANK_NODE(cur))
		    delete = cur;
		
		cur = cur->next;
		if (delete != NULL) {
		    xmlUnlinkNode(delete);
		    xmlFreeNode(delete);
		    delete = NULL;
#ifdef WITH_XSLT_DEBUG_PROCESS
		    nb++;
#endif
		}
	    }
	}

	/*
	 * Skip to next node in document order.
	 */
	if (node->type == XML_ENTITY_REF_NODE) {
	    /* process deep in entities */
	    xsltApplyStripSpaces(ctxt, node->children);
	}
	if ((current->children != NULL) &&
            (current->type != XML_ENTITY_REF_NODE)) {
	    current = current->children;
	} else if (current->next != NULL) {
	    current = current->next;
	} else {
	    do {
		current = current->parent;
		if (current == NULL)
		    break;
		if (current == node)
		    goto done;
		if (current->next != NULL) {
		    current = current->next;
		    break;
		}
	    } while (current != NULL);
	}
    }

done:
#ifdef WITH_XSLT_DEBUG_PROCESS
    XSLT_TRACE(ctxt,XSLT_TRACE_STRIP_SPACES,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltApplyStripSpaces: removed %d ignorable blank node\n", nb));
#endif
    return;
}

#ifdef XSLT_REFACTORED_KEYCOMP
static int
xsltCountKeys(xsltTransformContextPtr ctxt)
{
    xsltStylesheetPtr style;
    xsltKeyDefPtr keyd;

    if (ctxt == NULL)
	return(-1);    

    /*
    * Do we have those nastly templates with a key() in the match pattern?
    */
    ctxt->hasTemplKeyPatterns = 0;
    style = ctxt->style;
    while (style != NULL) {
	if (style->keyMatch != NULL) {
	    ctxt->hasTemplKeyPatterns = 1;
	    break;
	}
	style = xsltNextImport(style);
    }
    /*
    * Count number of key declarations.
    */
    ctxt->nbKeys = 0;
    style = ctxt->style;
    while (style != NULL) {
	keyd = style->keys;
	while (keyd) {
	    ctxt->nbKeys++;
	    keyd = keyd->next;
	}
	style = xsltNextImport(style);
    }        
    return(ctxt->nbKeys);
}
#endif /* XSLT_REFACTORED_KEYCOMP */

/**
 * xsltApplyStylesheetInternal:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated array of parameters names/values tuples
 * @output:  the targetted output
 * @profile:  profile FILE * output or NULL
 * @user:  user provided parameter
 *
 * Apply the stylesheet to the document
 * NOTE: This may lead to a non-wellformed output XML wise !
 *
 * Returns the result document or NULL in case of error
 */
static xmlDocPtr
xsltApplyStylesheetInternal(xsltStylesheetPtr style, xmlDocPtr doc,
                            const char **params, const char *output,
                            FILE * profile, xsltTransformContextPtr userCtxt)
{
    xmlDocPtr res = NULL;
    xsltTransformContextPtr ctxt = NULL;
    xmlNodePtr root, node;
    const xmlChar *method;
    const xmlChar *doctypePublic;
    const xmlChar *doctypeSystem;
    const xmlChar *version;
    xsltStackElemPtr variables;
    xsltStackElemPtr vptr;

    if ((style == NULL) || (doc == NULL))
        return (NULL);

    if (style->internalized == 0) {
#ifdef WITH_XSLT_DEBUG
	xsltGenericDebug(xsltGenericDebugContext,
			 "Stylesheet was not fully internalized !\n");
#endif
    }
    if (doc->intSubset != NULL) {
	/*
	 * Avoid hitting the DTD when scanning nodes
	 * but keep it linked as doc->intSubset
	 */
	xmlNodePtr cur = (xmlNodePtr) doc->intSubset;
	if (cur->next != NULL)
	    cur->next->prev = cur->prev;
	if (cur->prev != NULL)
	    cur->prev->next = cur->next;
	if (doc->children == cur)
	    doc->children = cur->next;
	if (doc->last == cur)
	    doc->last = cur->prev;
	cur->prev = cur->next = NULL;
    }

    /*
     * Check for XPath document order availability
     */
    root = xmlDocGetRootElement(doc);
    if (root != NULL) {
	if (((long) root->content) >= 0 && (xslDebugStatus == XSLT_DEBUG_NONE))
	    xmlXPathOrderDocElems(doc);
    }

    if (userCtxt != NULL)
	ctxt = userCtxt;
    else
	ctxt = xsltNewTransformContext(style, doc);

    if (ctxt == NULL)
        return (NULL);

    if (profile != NULL)
        ctxt->profile = 1;

    if (output != NULL)
        ctxt->outputFile = output;
    else
        ctxt->outputFile = NULL;

    /*
     * internalize the modes if needed
     */
    if (ctxt->dict != NULL) {
        if (ctxt->mode != NULL)
	    ctxt->mode = xmlDictLookup(ctxt->dict, ctxt->mode, -1);
        if (ctxt->modeURI != NULL)
	    ctxt->modeURI = xmlDictLookup(ctxt->dict, ctxt->modeURI, -1);
    }

    XSLT_GET_IMPORT_PTR(method, style, method)
        XSLT_GET_IMPORT_PTR(doctypePublic, style, doctypePublic)
        XSLT_GET_IMPORT_PTR(doctypeSystem, style, doctypeSystem)
        XSLT_GET_IMPORT_PTR(version, style, version)

        if ((method != NULL) &&
            (!xmlStrEqual(method, (const xmlChar *) "xml"))) {
        if (xmlStrEqual(method, (const xmlChar *) "html")) {
            ctxt->type = XSLT_OUTPUT_HTML;
            if (((doctypePublic != NULL) || (doctypeSystem != NULL))) {
                res = htmlNewDoc(doctypeSystem, doctypePublic);
	    } else {
                if (version == NULL) {
		    xmlDtdPtr dtd;

		    res = htmlNewDoc(NULL, NULL);
		    /*
		     * Make sure no DTD node is generated in this case
		     */
		    if (res != NULL) {
			dtd = xmlGetIntSubset(res);
			if (dtd != NULL) {
			    xmlUnlinkNode((xmlNodePtr) dtd);
			    xmlFreeDtd(dtd);
			}
			res->intSubset = NULL;
			res->extSubset = NULL;
		    }
		} else {
#ifdef XSLT_GENERATE_HTML_DOCTYPE
		    xsltGetHTMLIDs(version, &doctypePublic, &doctypeSystem);
#endif
		    res = htmlNewDoc(doctypeSystem, doctypePublic);
		}
            }
            if (res == NULL)
                goto error;
	    res->dict = ctxt->dict;
	    xmlDictReference(res->dict);
#ifdef WITH_XSLT_DEBUG
	    xsltGenericDebug(xsltGenericDebugContext,
			 "reusing transformation dict for output\n");
#endif
        } else if (xmlStrEqual(method, (const xmlChar *) "xhtml")) {
	    xsltTransformError(ctxt, NULL, (xmlNodePtr) doc,
     "xsltApplyStylesheetInternal: unsupported method xhtml, using html\n",
			 style->method);
            ctxt->type = XSLT_OUTPUT_HTML;
            res = htmlNewDoc(doctypeSystem, doctypePublic);
            if (res == NULL)
                goto error;
	    res->dict = ctxt->dict;
	    xmlDictReference(res->dict);
#ifdef WITH_XSLT_DEBUG
	    xsltGenericDebug(xsltGenericDebugContext,
			 "reusing transformation dict for output\n");
#endif
        } else if (xmlStrEqual(method, (const xmlChar *) "text")) {
            ctxt->type = XSLT_OUTPUT_TEXT;
            res = xmlNewDoc(style->version);
            if (res == NULL)
                goto error;
	    res->dict = ctxt->dict;
	    xmlDictReference(res->dict);
#ifdef WITH_XSLT_DEBUG
	    xsltGenericDebug(xsltGenericDebugContext,
			 "reusing transformation dict for output\n");
#endif
        } else {
	    xsltTransformError(ctxt, NULL, (xmlNodePtr) doc,
		     "xsltApplyStylesheetInternal: unsupported method %s\n",
                             style->method);
            goto error;
        }
    } else {
        ctxt->type = XSLT_OUTPUT_XML;
        res = xmlNewDoc(style->version);
        if (res == NULL)
            goto error;
	res->dict = ctxt->dict;
	xmlDictReference(ctxt->dict);
#ifdef WITH_XSLT_DEBUG
	xsltGenericDebug(xsltGenericDebugContext,
			 "reusing transformation dict for output\n");
#endif
    }
    res->charset = XML_CHAR_ENCODING_UTF8;
    if (style->encoding != NULL)
        res->encoding = xmlStrdup(style->encoding);
    variables = style->variables;

    /*
     * Start the evaluation, evaluate the params, the stylesheets globals
     * and start by processing the top node.
     */
    if (xsltNeedElemSpaceHandling(ctxt))
	xsltApplyStripSpaces(ctxt, xmlDocGetRootElement(doc));
    ctxt->output = res;
    ctxt->insert = (xmlNodePtr) res;
    if (ctxt->globalVars == NULL)
	ctxt->globalVars = xmlHashCreate(20);
    if (params != NULL)
        xsltEvalUserParams(ctxt, params);
    xsltEvalGlobalVariables(ctxt);
#ifdef XSLT_REFACTORED_KEYCOMP    
    xsltCountKeys(ctxt);
#endif
    ctxt->node = (xmlNodePtr) doc;
    varsPush(ctxt, NULL);
    ctxt->varsBase = ctxt->varsNr - 1;
    xsltProcessOneNode(ctxt, ctxt->node, NULL);
    xsltFreeStackElemList(varsPop(ctxt));
    xsltShutdownCtxtExts(ctxt);

    xsltCleanupTemplates(style); /* TODO: <- style should be read only */

    /*
     * Now cleanup our variables so stylesheet can be re-used
     *
     * TODO: this is not needed anymore global variables are copied
     *       and not evaluated directly anymore, keep this as a check
     */
    if (style->variables != variables) {
        vptr = style->variables;
        while (vptr->next != variables)
            vptr = vptr->next;
        vptr->next = NULL;
        xsltFreeStackElemList(style->variables);
        style->variables = variables;
    }
    vptr = style->variables;
    while (vptr != NULL) {
        if (vptr->computed) {
            if (vptr->value != NULL) {
                xmlXPathFreeObject(vptr->value);
                vptr->value = NULL;
                vptr->computed = 0;
            }
        }
        vptr = vptr->next;
    }


    /*
     * Do some post processing work depending on the generated output
     */
    root = xmlDocGetRootElement(res);
    if (root != NULL) {
        const xmlChar *doctype = NULL;

        if ((root->ns != NULL) && (root->ns->prefix != NULL))
	    doctype = xmlDictQLookup(ctxt->dict, root->ns->prefix, root->name);
	if (doctype == NULL)
	    doctype = root->name;

        /*
         * Apply the default selection of the method
         */
        if ((method == NULL) &&
            (root->ns == NULL) &&
            (!xmlStrcasecmp(root->name, (const xmlChar *) "html"))) {
            xmlNodePtr tmp;

            tmp = res->children;
            while ((tmp != NULL) && (tmp != root)) {
                if (tmp->type == XML_ELEMENT_NODE)
                    break;
                if ((tmp->type == XML_TEXT_NODE) && (!xmlIsBlankNode(tmp)))
                    break;
		tmp = tmp->next;
            }
            if (tmp == root) {
                ctxt->type = XSLT_OUTPUT_HTML;
		/*
		* REVISIT TODO: XML_HTML_DOCUMENT_NODE is set after the
		*  transformation on the doc, but functions like
		*/
                res->type = XML_HTML_DOCUMENT_NODE;
                if (((doctypePublic != NULL) || (doctypeSystem != NULL))) {
                    res->intSubset = xmlCreateIntSubset(res, doctype,
                                                        doctypePublic,
                                                        doctypeSystem);
#ifdef XSLT_GENERATE_HTML_DOCTYPE
		} else if (version != NULL) {
                    xsltGetHTMLIDs(version, &doctypePublic,
                                   &doctypeSystem);
                    if (((doctypePublic != NULL) || (doctypeSystem != NULL)))
                        res->intSubset =
                            xmlCreateIntSubset(res, doctype,
                                               doctypePublic,
                                               doctypeSystem);
#endif
                }
            }

        }
        if (ctxt->type == XSLT_OUTPUT_XML) {
            XSLT_GET_IMPORT_PTR(doctypePublic, style, doctypePublic)
            XSLT_GET_IMPORT_PTR(doctypeSystem, style, doctypeSystem)
            if (((doctypePublic != NULL) || (doctypeSystem != NULL))) {
	        xmlNodePtr last;
		/* Need a small "hack" here to assure DTD comes before
		   possible comment nodes */
		node = res->children;
		last = res->last;
		res->children = NULL;
		res->last = NULL;
                res->intSubset = xmlCreateIntSubset(res, doctype,
                                                    doctypePublic,
                                                    doctypeSystem);
		if (res->children != NULL) {
		    res->children->next = node;
		    node->prev = res->children;
		    res->last = last;
		} else {
		    res->children = node;
		    res->last = last;
		}
	    }
        }
    }
    xmlXPathFreeNodeSet(ctxt->nodeList);
    if (profile != NULL) {
        xsltSaveProfiling(ctxt, profile);
    }

    /*
     * Be pedantic.
     */
    if ((ctxt != NULL) && (ctxt->state == XSLT_STATE_ERROR)) {
	xmlFreeDoc(res);
	res = NULL;
    }
    if ((res != NULL) && (ctxt != NULL) && (output != NULL)) {
	int ret;

	ret = xsltCheckWrite(ctxt->sec, ctxt, (const xmlChar *) output);
	if (ret == 0) {
	    xsltTransformError(ctxt, NULL, NULL,
		     "xsltApplyStylesheet: forbidden to save to %s\n",
			       output);
	} else if (ret < 0) {
	    xsltTransformError(ctxt, NULL, NULL,
		     "xsltApplyStylesheet: saving to %s may not be possible\n",
			       output);
	}
    }

    if ((ctxt != NULL) && (userCtxt == NULL))
	xsltFreeTransformContext(ctxt);

    return (res);

error:
    if (res != NULL)
        xmlFreeDoc(res);
    if ((ctxt != NULL) && (userCtxt == NULL))
        xsltFreeTransformContext(ctxt);
    return (NULL);
}

/**
 * xsltApplyStylesheet:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated arry of parameters names/values tuples
 *
 * Apply the stylesheet to the document
 * NOTE: This may lead to a non-wellformed output XML wise !
 *
 * Returns the result document or NULL in case of error
 */
xmlDocPtr
xsltApplyStylesheet(xsltStylesheetPtr style, xmlDocPtr doc,
                    const char **params)
{
    return (xsltApplyStylesheetInternal(style, doc, params, NULL, NULL, NULL));
}

/**
 * xsltProfileStylesheet:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated arry of parameters names/values tuples
 * @output:  a FILE * for the profiling output
 *
 * Apply the stylesheet to the document and dump the profiling to
 * the given output.
 *
 * Returns the result document or NULL in case of error
 */
xmlDocPtr
xsltProfileStylesheet(xsltStylesheetPtr style, xmlDocPtr doc,
                      const char **params, FILE * output)
{
    xmlDocPtr res;

    res = xsltApplyStylesheetInternal(style, doc, params, NULL, output, NULL);
    return (res);
}

/**
 * xsltApplyStylesheetUser:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated array of parameters names/values tuples
 * @output:  the targetted output
 * @profile:  profile FILE * output or NULL
 * @userCtxt:  user provided transform context
 *
 * Apply the stylesheet to the document and allow the user to provide
 * its own transformation context.
 *
 * Returns the result document or NULL in case of error
 */
xmlDocPtr
xsltApplyStylesheetUser(xsltStylesheetPtr style, xmlDocPtr doc,
                            const char **params, const char *output,
                            FILE * profile, xsltTransformContextPtr userCtxt)
{
    xmlDocPtr res;

    res = xsltApplyStylesheetInternal(style, doc, params, output,
	                              profile, userCtxt);
    return (res);
}

/**
 * xsltRunStylesheetUser:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated array of parameters names/values tuples
 * @output:  the URL/filename ot the generated resource if available
 * @SAX:  a SAX handler for progressive callback output (not implemented yet)
 * @IObuf:  an output buffer for progressive output (not implemented yet)
 * @profile:  profile FILE * output or NULL
 * @userCtxt:  user provided transform context
 *
 * Apply the stylesheet to the document and generate the output according
 * to @output @SAX and @IObuf. It's an error to specify both @SAX and @IObuf.
 *
 * NOTE: This may lead to a non-wellformed output XML wise !
 * NOTE: This may also result in multiple files being generated
 * NOTE: using IObuf, the result encoding used will be the one used for
 *       creating the output buffer, use the following macro to read it
 *       from the stylesheet
 *       XSLT_GET_IMPORT_PTR(encoding, style, encoding)
 * NOTE: using SAX, any encoding specified in the stylesheet will be lost
 *       since the interface uses only UTF8
 *
 * Returns the number of by written to the main resource or -1 in case of
 *         error.
 */
int
xsltRunStylesheetUser(xsltStylesheetPtr style, xmlDocPtr doc,
                  const char **params, const char *output,
                  xmlSAXHandlerPtr SAX, xmlOutputBufferPtr IObuf,
		  FILE * profile, xsltTransformContextPtr userCtxt)
{
    xmlDocPtr tmp;
    int ret;

    if ((output == NULL) && (SAX == NULL) && (IObuf == NULL))
        return (-1);
    if ((SAX != NULL) && (IObuf != NULL))
        return (-1);

    /* unsupported yet */
    if (SAX != NULL) {
        XSLT_TODO   /* xsltRunStylesheet xmlSAXHandlerPtr SAX */
	return (-1);
    }

    tmp = xsltApplyStylesheetInternal(style, doc, params, output, profile,
	                              userCtxt);
    if (tmp == NULL) {
	xsltTransformError(NULL, NULL, (xmlNodePtr) doc,
                         "xsltRunStylesheet : run failed\n");
        return (-1);
    }
    if (IObuf != NULL) {
        /* TODO: incomplete, IObuf output not progressive */
        ret = xsltSaveResultTo(IObuf, tmp, style);
    } else {
        ret = xsltSaveResultToFilename(output, tmp, style, 0);
    }
    xmlFreeDoc(tmp);
    return (ret);
}

/**
 * xsltRunStylesheet:
 * @style:  a parsed XSLT stylesheet
 * @doc:  a parsed XML document
 * @params:  a NULL terminated array of parameters names/values tuples
 * @output:  the URL/filename ot the generated resource if available
 * @SAX:  a SAX handler for progressive callback output (not implemented yet)
 * @IObuf:  an output buffer for progressive output (not implemented yet)
 *
 * Apply the stylesheet to the document and generate the output according
 * to @output @SAX and @IObuf. It's an error to specify both @SAX and @IObuf.
 *
 * NOTE: This may lead to a non-wellformed output XML wise !
 * NOTE: This may also result in multiple files being generated
 * NOTE: using IObuf, the result encoding used will be the one used for
 *       creating the output buffer, use the following macro to read it
 *       from the stylesheet
 *       XSLT_GET_IMPORT_PTR(encoding, style, encoding)
 * NOTE: using SAX, any encoding specified in the stylesheet will be lost
 *       since the interface uses only UTF8
 *
 * Returns the number of bytes written to the main resource or -1 in case of
 *         error.
 */
int
xsltRunStylesheet(xsltStylesheetPtr style, xmlDocPtr doc,
                  const char **params, const char *output,
                  xmlSAXHandlerPtr SAX, xmlOutputBufferPtr IObuf)
{
    return(xsltRunStylesheetUser(style, doc, params, output, SAX, IObuf,
		                 NULL, NULL));
}

/**
 * xsltRegisterAllElement:
 * @ctxt:  the XPath context
 *
 * Registers all default XSLT elements in this context
 */
void
xsltRegisterAllElement(xsltTransformContextPtr ctxt)
{
    xsltRegisterExtElement(ctxt, (const xmlChar *) "apply-templates",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltApplyTemplates);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "apply-imports",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltApplyImports);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "call-template",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltCallTemplate);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "element",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltElement);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "attribute",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltAttribute);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "text",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltText);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "processing-instruction",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltProcessingInstruction);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "comment",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltComment);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "copy",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltCopy);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "value-of",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltValueOf);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "number",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltNumber);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "for-each",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltForEach);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "if",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltIf);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "choose",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltChoose);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "sort",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltSort);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "copy-of",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltCopyOf);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "message",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltMessage);

    /*
     * Those don't have callable entry points but are registered anyway
     */
    xsltRegisterExtElement(ctxt, (const xmlChar *) "variable",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "param",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "with-param",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "decimal-format",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "when",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "otherwise",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltDebug);
    xsltRegisterExtElement(ctxt, (const xmlChar *) "fallback",
                           XSLT_NAMESPACE,
			   (xsltTransformFunction) xsltDebug);

}
