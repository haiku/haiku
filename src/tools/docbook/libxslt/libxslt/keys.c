/*
 * keys.c: Implemetation of the keys support
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */

#define IN_LIBXSLT
#include "libxslt.h"

#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/valid.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/parserInternals.h>
#include <libxml/xpathInternals.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "imports.h"
#include "templates.h"
#include "keys.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_KEYS
#endif


/************************************************************************
 * 									*
 * 			Type functions 					*
 * 									*
 ************************************************************************/

/**
 * xsltNewKeyDef:
 * @name:  the key name or NULL
 * @nameURI:  the name URI or NULL
 *
 * Create a new XSLT KeyDef
 *
 * Returns the newly allocated xsltKeyDefPtr or NULL in case of error
 */
static xsltKeyDefPtr
xsltNewKeyDef(const xmlChar *name, const xmlChar *nameURI) {
    xsltKeyDefPtr cur;

    cur = (xsltKeyDefPtr) xmlMalloc(sizeof(xsltKeyDef));
    if (cur == NULL) {
	xsltTransformError(NULL, NULL, NULL,
		"xsltNewKeyDef : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltKeyDef));
    if (name != NULL)
	cur->name = xmlStrdup(name);
    if (nameURI != NULL)
	cur->nameURI = xmlStrdup(nameURI);
    cur->nsList = NULL;
    return(cur);
}

/**
 * xsltFreeKeyDef:
 * @keyd:  an XSLT key definition
 *
 * Free up the memory allocated by @keyd
 */
static void
xsltFreeKeyDef(xsltKeyDefPtr keyd) {
    if (keyd == NULL)
	return;
    if (keyd->comp != NULL)
	xmlXPathFreeCompExpr(keyd->comp);
    if (keyd->usecomp != NULL)
	xmlXPathFreeCompExpr(keyd->usecomp);
    if (keyd->name != NULL)
	xmlFree(keyd->name);
    if (keyd->nameURI != NULL)
	xmlFree(keyd->nameURI);
    if (keyd->match != NULL)
	xmlFree(keyd->match);
    if (keyd->use != NULL)
	xmlFree(keyd->use);
    if (keyd->nsList != NULL)
        xmlFree(keyd->nsList);
    memset(keyd, -1, sizeof(xsltKeyDef));
    xmlFree(keyd);
}

/**
 * xsltFreeKeyDefList:
 * @keyd:  an XSLT key definition list
 *
 * Free up the memory allocated by all the elements of @keyd
 */
static void
xsltFreeKeyDefList(xsltKeyDefPtr keyd) {
    xsltKeyDefPtr cur;

    while (keyd != NULL) {
	cur = keyd;
	keyd = keyd->next;
	xsltFreeKeyDef(cur);
    }
}

/**
 * xsltNewKeyTable:
 * @name:  the key name or NULL
 * @nameURI:  the name URI or NULL
 *
 * Create a new XSLT KeyTable
 *
 * Returns the newly allocated xsltKeyTablePtr or NULL in case of error
 */
static xsltKeyTablePtr
xsltNewKeyTable(const xmlChar *name, const xmlChar *nameURI) {
    xsltKeyTablePtr cur;

    cur = (xsltKeyTablePtr) xmlMalloc(sizeof(xsltKeyTable));
    if (cur == NULL) {
	xsltTransformError(NULL, NULL, NULL,
		"xsltNewKeyTable : malloc failed\n");
	return(NULL);
    }
    memset(cur, 0, sizeof(xsltKeyTable));
    if (name != NULL)
	cur->name = xmlStrdup(name);
    if (nameURI != NULL)
	cur->nameURI = xmlStrdup(nameURI);
    cur->keys = xmlHashCreate(0);
    return(cur);
}

/**
 * xsltFreeKeyTable:
 * @keyt:  an XSLT key table
 *
 * Free up the memory allocated by @keyt
 */
static void
xsltFreeKeyTable(xsltKeyTablePtr keyt) {
    if (keyt == NULL)
	return;
    if (keyt->name != NULL)
	xmlFree(keyt->name);
    if (keyt->nameURI != NULL)
	xmlFree(keyt->nameURI);
    if (keyt->keys != NULL)
	xmlHashFree(keyt->keys, 
		    (xmlHashDeallocator) xmlXPathFreeNodeSet);
    memset(keyt, -1, sizeof(xsltKeyTable));
    xmlFree(keyt);
}

/**
 * xsltFreeKeyTableList:
 * @keyt:  an XSLT key table list
 *
 * Free up the memory allocated by all the elements of @keyt
 */
static void
xsltFreeKeyTableList(xsltKeyTablePtr keyt) {
    xsltKeyTablePtr cur;

    while (keyt != NULL) {
	cur = keyt;
	keyt = keyt->next;
	xsltFreeKeyTable(cur);
    }
}

/************************************************************************
 * 									*
 * 		The interpreter for the precompiled patterns		*
 * 									*
 ************************************************************************/


/**
 * xsltFreeKeys:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by XSLT keys in a stylesheet
 */
void
xsltFreeKeys(xsltStylesheetPtr style) {
    if (style->keys)
	xsltFreeKeyDefList((xsltKeyDefPtr) style->keys);
}

/**
 * skipString:
 * @cur: the current pointer
 * @end: the current offset
 *
 * skip a string delimited by " or '
 *
 * Returns the byte after the string or -1 in case of error
 */
static int
skipString(const xmlChar *cur, int end) {
    xmlChar limit;

    if ((cur == NULL) || (end < 0)) return(-1);
    if ((cur[end] == '\'') || (cur[end] == '"')) limit = cur[end];
    else return(end);
    end++;
    while (cur[end] != 0) {
        if (cur[end] == limit)
	    return(end + 1);
	end++;
    }
    return(-1);
}

/**
 * skipPredicate:
 * @cur: the current pointer
 * @end: the current offset
 *
 * skip a predicate
 *
 * Returns the byte after the predicate or -1 in case of error
 */
static int
skipPredicate(const xmlChar *cur, int end) {
    if ((cur == NULL) || (end < 0)) return(-1);
    if (cur[end] != '[') return(end);
    end++;
    while (cur[end] != 0) {
        if ((cur[end] == '\'') || (cur[end] == '"')) {
	    end = skipString(cur, end);
	    if (end <= 0)
	        return(-1);
	    continue;
	} else if (cur[end] == '[') {
	    end = skipPredicate(cur, end);
	    if (end <= 0)
	        return(-1);
	    continue;
	} else if (cur[end] == ']')
	    return(end + 1);
	end++;
    }
    return(-1);
}

/**
 * xsltAddKey:
 * @style: an XSLT stylesheet
 * @name:  the key name or NULL
 * @nameURI:  the name URI or NULL
 * @match:  the match value
 * @use:  the use value
 * @inst: the key instruction
 *
 * add a key definition to a stylesheet
 *
 * Returns 0 in case of success, and -1 in case of failure.
 */
int	
xsltAddKey(xsltStylesheetPtr style, const xmlChar *name,
	   const xmlChar *nameURI, const xmlChar *match,
	   const xmlChar *use, xmlNodePtr inst) {
    xsltKeyDefPtr key;
    xmlChar *pattern = NULL;
    int current, end, start, i = 0;

    if ((style == NULL) || (name == NULL) || (match == NULL) || (use == NULL))
	return(-1);

#ifdef WITH_XSLT_DEBUG_KEYS
    xsltGenericDebug(xsltGenericDebugContext,
	"Add key %s, match %s, use %s\n", name, match, use);
#endif

    key = xsltNewKeyDef(name, nameURI);
    key->match = xmlStrdup(match);
    key->use = xmlStrdup(use);
    key->inst = inst;
    key->nsList = xmlGetNsList(inst->doc, inst);
    if (key->nsList != NULL) {
        while (key->nsList[i] != NULL)
	    i++;
    }
    key->nsNr = i;

    /*
     * Split the | and register it as as many keys
     */
    current = end = 0;
    while (match[current] != 0) {
	start = current;
	while (IS_BLANK_CH(match[current]))
	    current++;
	end = current;
	while ((match[end] != 0) && (match[end] != '|')) {
	    if (match[end] == '[') {
	        end = skipPredicate(match, end);
		if (end <= 0) {
		    xsltTransformError(NULL, style, inst,
		                       "key pattern is malformed: %s",
				       key->match);
		    if (style != NULL) style->errors++;
		    goto error;
		}
	    } else
		end++;
	}
	if (current == end) {
	    xsltTransformError(NULL, style, inst,
			       "key pattern is empty\n");
	    if (style != NULL) style->errors++;
	    goto error;
	}
	if (match[start] != '/') {
	    pattern = xmlStrcat(pattern, (xmlChar *)"//");
	    if (pattern == NULL) {
		if (style != NULL) style->errors++;
		goto error;
	    }
	}
	pattern = xmlStrncat(pattern, &match[start], end - start);
	if (pattern == NULL) {
	    if (style != NULL) style->errors++;
	    goto error;
	}

	if (match[end] == '|') {
	    pattern = xmlStrcat(pattern, (xmlChar *)"|");
	    end++;
	}
	current = end;
    }
#ifdef WITH_XSLT_DEBUG_KEYS
    xsltGenericDebug(xsltGenericDebugContext,
	"   resulting pattern %s\n", pattern);
#endif
    /*    
    * XSLT-1: "It is an error for the value of either the use
    *  attribute or the match attribute to contain a
    *  VariableReference."
    * TODO: We should report a variable-reference at compile-time.
    *   Maybe a search for "$", if it occurs outside of quotation
    *   marks, could be sufficient.
    */
    key->comp = xsltXPathCompile(style, pattern);
    if (key->comp == NULL) {
	xsltTransformError(NULL, style, inst,
		"xsl:key : XPath pattern compilation failed '%s'\n",
		         pattern);
	if (style != NULL) style->errors++;
    }
    key->usecomp = xsltXPathCompile(style, use);
    if (key->usecomp == NULL) {
	xsltTransformError(NULL, style, inst,
		"xsl:key : XPath pattern compilation failed '%s'\n",
		         use);
	if (style != NULL) style->errors++;
    }
    key->next = style->keys;
    style->keys = key;
error:
    if (pattern != NULL)
	xmlFree(pattern);
    return(0);
}

/**
 * xsltGetKey:
 * @ctxt: an XSLT transformation context
 * @name:  the key name or NULL
 * @nameURI:  the name URI or NULL
 * @value:  the key value to look for
 *
 * Lookup a key
 *
 * Returns the nodeset resulting from the query or NULL
 */
xmlNodeSetPtr
xsltGetKey(xsltTransformContextPtr ctxt, const xmlChar *name,
	   const xmlChar *nameURI, const xmlChar *value) {
    xmlNodeSetPtr ret;
    xsltKeyTablePtr table;
#ifdef XSLT_REFACTORED_KEYCOMP
    int found = 0;
#endif

    if ((ctxt == NULL) || (name == NULL) || (value == NULL) ||
	(ctxt->document == NULL))
	return(NULL);

#ifdef WITH_XSLT_DEBUG_KEYS
    xsltGenericDebug(xsltGenericDebugContext,
	"Get key %s, value %s\n", name, value);
#endif
    table = (xsltKeyTablePtr) ctxt->document->keys;
    while (table != NULL) {
	if (((nameURI != NULL) == (table->nameURI != NULL)) &&
	    xmlStrEqual(table->name, name) &&
	    xmlStrEqual(table->nameURI, nameURI))
	{
#ifdef XSLT_REFACTORED_KEYCOMP
	    found = 1;
#endif
	    ret = (xmlNodeSetPtr)xmlHashLookup(table->keys, value);
	    return(ret);
	}
	table = table->next;
    }
#ifdef XSLT_REFACTORED_KEYCOMP
    if (! found) {
	xsltStylesheetPtr style = ctxt->style;	
	xsltKeyDefPtr keyd;
	/*
	* This might be the first call to the key with the specified
	* name and the specified document.
	* Find all keys with a matching name and compute them for the
	* current tree.
	*/
	found = 0;
	while (style != NULL) {
	    keyd = (xsltKeyDefPtr) style->keys;
	    while (keyd != NULL) {
		if (((nameURI != NULL) == (keyd->nameURI != NULL)) &&
		    xmlStrEqual(keyd->name, name) &&
		    xmlStrEqual(keyd->nameURI, nameURI))
		{
		    found = 1;
		    xsltInitCtxtKey(ctxt, ctxt->document, keyd);
		}
		keyd = keyd->next;		
	    }	    
	    style = xsltNextImport(style);
	}
	if (found) {
	    /*
	    * The key was computed, so look it up.
	    */
	    table = (xsltKeyTablePtr) ctxt->document->keys;
	    while (table != NULL) {
		if (((nameURI != NULL) == (table->nameURI != NULL)) &&
		    xmlStrEqual(table->name, name) &&
		    xmlStrEqual(table->nameURI, nameURI))
		{
		    ret = (xmlNodeSetPtr)xmlHashLookup(table->keys, value);
		    return(ret);
		}
		table = table->next;
	    }

	}
    }
#endif
    return(NULL);
}

/**
 * xsltEvalXPathKeys:
 * @ctxt:  the XSLT transformation context
 * @comp:  the compiled XPath expression
 *
 * Process the expression using XPath to get the list of keys
 *
 * Returns the array of computed string value or NULL, must be deallocated
 *         by the caller.
 */
static xmlChar **
xsltEvalXPathKeys(xsltTransformContextPtr ctxt, xmlXPathCompExprPtr comp,
                  xsltKeyDefPtr keyd) {
    xmlChar **ret = NULL;
    xmlXPathObjectPtr res;
    xmlNodePtr oldInst;
    xmlNodePtr oldNode;
    int	oldPos, oldSize;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    oldInst = ctxt->inst;
    oldNode = ctxt->node;
    oldPos = ctxt->xpathCtxt->proximityPosition;
    oldSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;

    ctxt->xpathCtxt->node = ctxt->node;
    ctxt->xpathCtxt->namespaces = keyd->nsList;
    ctxt->xpathCtxt->nsNr = keyd->nsNr;
    res = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
    if (res != NULL) {
	if (res->type == XPATH_NODESET) {
	    int len, i, j;

	    if (res->nodesetval != NULL)
		len = res->nodesetval->nodeNr;
	    else
		len = 0;
	    if (len != 0) {
		ret = (xmlChar **) xmlMalloc((len + 1) * sizeof(xmlChar *));
		if (ret != NULL) {
		    for (i = 0,j = 0;i < len;i++) {
			ret[j] = xmlXPathCastNodeToString(
				res->nodesetval->nodeTab[i]);
			if (ret[j] != NULL)
			    j++;
		    }
		    ret[j] = NULL;
		}
	    }
	} else {
	    if (res->type != XPATH_STRING)
		res = xmlXPathConvertString(res);
	    if (res->type == XPATH_STRING) {
		ret = (xmlChar **) xmlMalloc(2 * sizeof(xmlChar *));
		if (ret != NULL) {
		    ret[0] = res->stringval;
		    ret[1] = NULL;
		    res->stringval = NULL;
		}
	    } else {
		xsltTransformError(ctxt, NULL, NULL,
		     "xpath : string() function didn't return a String\n");
	    }
	}
	xmlXPathFreeObject(res);
    } else {
	ctxt->state = XSLT_STATE_STOPPED;
    }
#ifdef WITH_XSLT_DEBUG_TEMPLATES
    xsltGenericDebug(xsltGenericDebugContext,
	 "xsltEvalXPathString: returns %s\n", ret);
#endif
    ctxt->inst = oldInst;
    ctxt->node = oldNode;
    ctxt->xpathCtxt->contextSize = oldSize;
    ctxt->xpathCtxt->proximityPosition = oldPos;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    return(ret);
}

/**
 * xsltInitCtxtKey:
 * @ctxt: an XSLT transformation context
 * @doc:  an XSLT document
 * @keyd: the key definition
 *
 * Computes the key tables this key and for the current input document.
 */
int
xsltInitCtxtKey(xsltTransformContextPtr ctxt, xsltDocumentPtr doc,
	        xsltKeyDefPtr keyd) {
    int i;
    xmlNodeSetPtr nodelist = NULL, keylist;
    xmlXPathObjectPtr res = NULL;
    xmlChar *str, **list;
    xsltKeyTablePtr table;
    int	oldPos, oldSize;
    xmlNodePtr oldInst;
    xmlNodePtr oldNode;
    xsltDocumentPtr oldDoc;
    xmlDocPtr oldXDoc;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    doc->nbKeysComputed++;
    /*
     * Evaluate the nodelist
     */

    oldXDoc= ctxt->xpathCtxt->doc;
    oldPos = ctxt->xpathCtxt->proximityPosition;
    oldSize = ctxt->xpathCtxt->contextSize;
    oldNsNr = ctxt->xpathCtxt->nsNr;
    oldNamespaces = ctxt->xpathCtxt->namespaces;
    oldInst = ctxt->inst;
    oldDoc = ctxt->document;
    oldNode = ctxt->node;

    if (keyd->comp == NULL)
	goto error;
    if (keyd->usecomp == NULL)
	goto error;

    ctxt->document = doc;
    ctxt->xpathCtxt->doc = doc->doc;
    ctxt->xpathCtxt->node = (xmlNodePtr) doc->doc;
    ctxt->node = (xmlNodePtr) doc->doc;
    /* TODO : clarify the use of namespaces in keys evaluation */
    ctxt->xpathCtxt->namespaces = keyd->nsList;
    ctxt->xpathCtxt->nsNr = keyd->nsNr;
    ctxt->inst = keyd->inst;
    res = xmlXPathCompiledEval(keyd->comp, ctxt->xpathCtxt);
    ctxt->xpathCtxt->contextSize = oldSize;
    ctxt->xpathCtxt->proximityPosition = oldPos;
    ctxt->inst = oldInst;

    if (res != NULL) {
	if (res->type == XPATH_NODESET) {
	    nodelist = res->nodesetval;
#ifdef WITH_XSLT_DEBUG_KEYS
	    if (nodelist != NULL)
		XSLT_TRACE(ctxt,XSLT_TRACE_KEYS,xsltGenericDebug(xsltGenericDebugContext,
		     "xsltInitCtxtKey: %s evaluates to %d nodes\n",
				 keyd->match, nodelist->nodeNr));
#endif
	} else {
#ifdef WITH_XSLT_DEBUG_KEYS
	    XSLT_TRACE(ctxt,XSLT_TRACE_KEYS,xsltGenericDebug(xsltGenericDebugContext,
		 "xsltInitCtxtKey: %s is not a node set\n", keyd->match));
#endif
	    goto error;
	}
    } else {
#ifdef WITH_XSLT_DEBUG_KEYS
	XSLT_TRACE(ctxt,XSLT_TRACE_KEYS,xsltGenericDebug(xsltGenericDebugContext,
	     "xsltInitCtxtKey: %s evaluation failed\n", keyd->match));
#endif
	ctxt->state = XSLT_STATE_STOPPED;
	goto error;
    }

    /*
     * for each node in the list evaluate the key and insert the node
     */
    if ((nodelist == NULL) || (nodelist->nodeNr <= 0))
	goto error;

    /**
     * Multiple key definitions for the same name are allowed, so
     * we must check if the key is already present for this doc
     */
    table = (xsltKeyTablePtr) doc->keys;
    while (table != NULL) {
        if (xmlStrEqual(table->name, keyd->name) &&
	    (((keyd->nameURI == NULL) && (table->nameURI == NULL)) ||
	     ((keyd->nameURI != NULL) && (table->nameURI != NULL) &&
	      (xmlStrEqual(table->nameURI, keyd->nameURI)))))
	    break;
	table = table->next;
    }
    /**
     * If the key was not previously defined, create it now and
     * chain it to the list of keys for the doc
     */
    if (table == NULL) {
        table = xsltNewKeyTable(keyd->name, keyd->nameURI);
        if (table == NULL)
	    goto error;
        table->next = doc->keys;
        doc->keys = table;
    }

    for (i = 0;i < nodelist->nodeNr;i++) {
	if (IS_XSLT_REAL_NODE(nodelist->nodeTab[i])) {
	    ctxt->node = nodelist->nodeTab[i];

	    list = xsltEvalXPathKeys(ctxt, keyd->usecomp, keyd);
	    if (list != NULL) {
		int ix = 0;

		str = list[ix++];
		while (str != NULL) {
#ifdef WITH_XSLT_DEBUG_KEYS
		    XSLT_TRACE(ctxt,XSLT_TRACE_KEYS,xsltGenericDebug(xsltGenericDebugContext,
			 "xsl:key : node associated to(%s,%s)\n",
				     keyd->name, str));
#endif
		    keylist = xmlHashLookup(table->keys, str);
		    if (keylist == NULL) {
			keylist = xmlXPathNodeSetCreate(nodelist->nodeTab[i]);
			xmlHashAddEntry(table->keys, str, keylist);
		    } else {
			xmlXPathNodeSetAdd(keylist, nodelist->nodeTab[i]);
		    }
		    switch (nodelist->nodeTab[i]->type) {
                        case XML_ELEMENT_NODE:
                        case XML_TEXT_NODE:
                        case XML_CDATA_SECTION_NODE:
                        case XML_PI_NODE:
                        case XML_COMMENT_NODE:
			    nodelist->nodeTab[i]->psvi = keyd;
			    break;
                        case XML_ATTRIBUTE_NODE: {
			    xmlAttrPtr attr = (xmlAttrPtr) 
			                      nodelist->nodeTab[i];
			    attr->psvi = keyd;
			    break;
			}
                        case XML_DOCUMENT_NODE:
                        case XML_HTML_DOCUMENT_NODE: {
			    xmlDocPtr kdoc = (xmlDocPtr) 
			                    nodelist->nodeTab[i];
			    kdoc->psvi = keyd;
			    break;
			}
			default:
			    break;
		    }
		    xmlFree(str);
		    str = list[ix++];
		}
		xmlFree(list);
#ifdef WITH_XSLT_DEBUG_KEYS
	    } else {
		XSLT_TRACE(ctxt,XSLT_TRACE_KEYS,xsltGenericDebug(xsltGenericDebugContext,
		     "xsl:key : use %s failed to return strings\n",
				 keyd->use));
#endif
	    }
	}
    }

error:
    ctxt->document = oldDoc;
    ctxt->xpathCtxt->doc = oldXDoc;
    ctxt->xpathCtxt->nsNr = oldNsNr;
    ctxt->xpathCtxt->namespaces = oldNamespaces;
    ctxt->node = oldNode;
    if (res != NULL)
	xmlXPathFreeObject(res);
    return(0);
}

/**
 * xsltInitCtxtKeys:
 * @ctxt:  an XSLT transformation context
 * @doc:  an XSLT document
 *
 * Computes all the keys tables for the current input document.
 * Should be done before global varibales are initialized.
 * NOTE: Not used anymore in the refactored code.
 */
void
xsltInitCtxtKeys(xsltTransformContextPtr ctxt, xsltDocumentPtr doc) {
    xsltStylesheetPtr style;
    xsltKeyDefPtr keyd;

    if ((ctxt == NULL) || (doc == NULL))
	return;
#ifdef WITH_XSLT_DEBUG_KEYS
    if ((doc->doc != NULL) && (doc->doc->URL != NULL))
	XSLT_TRACE(ctxt,XSLT_TRACE_KEYS,xsltGenericDebug(xsltGenericDebugContext, "Initializing keys on %s\n",
		     doc->doc->URL));
#endif
    style = ctxt->style;
    while (style != NULL) {
	keyd = (xsltKeyDefPtr) style->keys;
	while (keyd != NULL) {
	    xsltInitCtxtKey(ctxt, doc, keyd);

	    keyd = keyd->next;
	}

	style = xsltNextImport(style);
    }
}

/**
 * xsltFreeDocumentKeys:
 * @doc: a XSLT document
 *
 * Free the keys associated to a document
 */
void	
xsltFreeDocumentKeys(xsltDocumentPtr doc) {
    if (doc != NULL)
        xsltFreeKeyTableList(doc->keys);
}

