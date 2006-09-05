/*
 * variables.c: Implementation of the variable storage and lookup
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
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parserInternals.h>
#include <libxml/dict.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "variables.h"
#include "transform.h"
#include "imports.h"
#include "preproc.h"
#include "keys.h"

#ifdef WITH_XSLT_DEBUG
#define WITH_XSLT_DEBUG_VARIABLE
#endif

#ifdef XSLT_REFACTORED
const xmlChar *xsltDocFragFake = (const xmlChar *) " fake node libxslt";
#endif

/************************************************************************
 *									*
 *  Result Value Tree (Result Tree Fragment) interfaces			*
 *									*
 ************************************************************************/
/**
 * xsltCreateRVT:
 * @ctxt:  an XSLT transformation context
 *
 * Create a Result Value Tree
 * (the XSLT 1.0 term for this is "Result Tree Fragment") 
 *
 * Returns the result value tree or NULL in case of error
 */
xmlDocPtr
xsltCreateRVT(xsltTransformContextPtr ctxt)
{
    xmlDocPtr container;

    /*
    * Question: Why is this function public?
    * Answer: It is called by the EXSLT module.
    */    
    if (ctxt == NULL) return(NULL);

    container = xmlNewDoc(NULL);
    if (container == NULL)
	return(NULL);
    container->dict = ctxt->dict;
    xmlDictReference(container->dict);
#ifdef WITH_XSLT_DEBUG
    xsltGenericDebug(xsltGenericDebugContext,
                     "reusing transformation dict for RVT\n");
#endif

    XSLT_MARK_RES_TREE_FRAG(container);
    container->doc = container;
    container->parent = NULL;
    return(container);
}

/**
 * xsltRegisterTmpRVT:
 * @ctxt:  an XSLT transformation context
 * @RVT:  a result value tree (Result Tree Fragment)
 *
 * Register the result value tree (XSLT 1.0 term: Result Tree Fragment)
 * for destruction at the end of the context
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int
xsltRegisterTmpRVT(xsltTransformContextPtr ctxt, xmlDocPtr RVT)
{
    if ((ctxt == NULL) || (RVT == NULL)) return(-1);

    RVT->next = (xmlNodePtr) ctxt->tmpRVT;
    if (ctxt->tmpRVT != NULL)
	ctxt->tmpRVT->prev = (xmlNodePtr) RVT;
    ctxt->tmpRVT = RVT;
    return(0);
}

/**
 * xsltRegisterPersistRVT:
 * @ctxt:  an XSLT transformation context
 * @RVT:  a result value tree (Result Tree Fragment)
 *
 * Register the result value tree (XSLT 1.0 term: Result Tree Fragment)
 * for destruction at the end of the processing
 *
 * Returns 0 in case of success and -1 in case of error.
 */
int
xsltRegisterPersistRVT(xsltTransformContextPtr ctxt, xmlDocPtr RVT)
{
    if ((ctxt == NULL) || (RVT == NULL)) return(-1);

    RVT->next = (xmlNodePtr) ctxt->persistRVT;
    if (ctxt->persistRVT != NULL)
	ctxt->persistRVT->prev = (xmlNodePtr) RVT;
    ctxt->persistRVT = RVT;
    return(0);
}

/**
 * xsltFreeRVTs:
 * @ctxt:  an XSLT transformation context
 *
 * Free all the registered result value tree (Result Tree Fragment)
 * of the transformation
 */
void
xsltFreeRVTs(xsltTransformContextPtr ctxt)
{
    xmlDocPtr cur, next;

    if (ctxt == NULL) return;

    cur = ctxt->tmpRVT;
    while (cur != NULL) {
        next = (xmlDocPtr) cur->next;
	if (cur->_private != NULL) {
	    xsltFreeDocumentKeys(cur->_private);
	    xmlFree(cur->_private);
	}
	xmlFreeDoc(cur);
	cur = next;
    }
    cur = ctxt->persistRVT;
    while (cur != NULL) {
        next = (xmlDocPtr) cur->next;
	if (cur->_private != NULL) {
	    xsltFreeDocumentKeys(cur->_private);
	    xmlFree(cur->_private);
	}
	xmlFreeDoc(cur);
	cur = next;
    }
}

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltNewStackElem:
 *
 * Create a new XSLT ParserContext
 *
 * Returns the newly allocated xsltParserStackElem or NULL in case of error
 */
static xsltStackElemPtr
xsltNewStackElem(void) {
    xsltStackElemPtr cur;

    cur = (xsltStackElemPtr) xmlMalloc(sizeof(xsltStackElem));
    if (cur == NULL) {
	xsltTransformError(NULL, NULL, NULL,
		"xsltNewStackElem : malloc failed\n");
	return(NULL);
    }
    cur->computed = 0;
    cur->name = NULL;
    cur->nameURI = NULL;
    cur->select = NULL;
    cur->tree = NULL;
    cur->value = NULL;
    cur->comp = NULL;
    return(cur);
}

/**
 * xsltCopyStackElem:
 * @elem:  an XSLT stack element
 *
 * Makes a copy of the stack element
 *
 * Returns the copy of NULL
 */
static xsltStackElemPtr
xsltCopyStackElem(xsltStackElemPtr elem) {
    xsltStackElemPtr cur;

    cur = (xsltStackElemPtr) xmlMalloc(sizeof(xsltStackElem));
    if (cur == NULL) {
	xsltTransformError(NULL, NULL, NULL,
		"xsltCopyStackElem : malloc failed\n");
	return(NULL);
    }
    cur->name = elem->name;
    cur->nameURI = elem->nameURI;
    cur->select = elem->select;
    cur->tree = elem->tree;
    cur->comp = elem->comp;
    cur->computed = 0;
    cur->value = NULL;
    return(cur);
}

/**
 * xsltFreeStackElem:
 * @elem:  an XSLT stack element
 *
 * Free up the memory allocated by @elem
 */
static void
xsltFreeStackElem(xsltStackElemPtr elem) {
    if (elem == NULL)
	return;
    if (elem->value != NULL)
	xmlXPathFreeObject(elem->value);

    xmlFree(elem);
}

/**
 * xsltFreeStackElemList:
 * @elem:  an XSLT stack element
 *
 * Free up the memory allocated by @elem
 */
void
xsltFreeStackElemList(xsltStackElemPtr elem) {
    xsltStackElemPtr next;

    while(elem != NULL) {
	next = elem->next;
	xsltFreeStackElem(elem);
	elem = next;
    }
}

/**
 * xsltStackLookup:
 * @ctxt:  an XSLT transformation context
 * @name:  the local part of the name
 * @nameURI:  the URI part of the name
 *
 * Locate an element in the stack based on its name.
 */
static int stack_addr = 0;
static int stack_cmp = 0;
static xsltStackElemPtr
xsltStackLookup(xsltTransformContextPtr ctxt, const xmlChar *name,
	        const xmlChar *nameURI) {
    int i;
    xsltStackElemPtr cur;

    if ((ctxt == NULL) || (name == NULL) || (ctxt->varsNr == 0))
	return(NULL);

    /*
     * Do the lookup from the top of the stack, but
     * don't use params being computed in a call-param
     * First lookup expects the variable name and URI to
     * come from the disctionnary and hence get equality
     */
    for (i = ctxt->varsNr; i > ctxt->varsBase; i--) {
	cur = ctxt->varsTab[i-1];
	while (cur != NULL) {
	    if (cur->name == name) {
		if (nameURI == NULL) {
		    if (cur->nameURI == NULL) {
		        stack_addr++;
			return(cur);
		    }
		} else {
		    if ((cur->nameURI != NULL) &&
			(cur->nameURI == nameURI)) {
		        stack_addr++;
			return(cur);
		    }
		}

	    }
	    cur = cur->next;
	}
    }

    /*
     * Redo the lookup with interned string compares
     * to avoid string compares.
     */
    name = xmlDictLookup(ctxt->dict, name, -1);
    if (nameURI != NULL)
        nameURI = xmlDictLookup(ctxt->dict, nameURI, -1);
    else
        nameURI = NULL;
    for (i = ctxt->varsNr; i > ctxt->varsBase; i--) {
	cur = ctxt->varsTab[i-1];
	while (cur != NULL) {
	    if (cur->name == name) {
		if (nameURI == NULL) {
		    if (cur->nameURI == NULL) {
		        stack_cmp++;
			return(cur);
		    }
		} else {
		    if ((cur->nameURI != NULL) &&
			(cur->nameURI == nameURI)) {
		        stack_cmp++;
			return(cur);
		    }
		}

	    }
	    cur = cur->next;
	}
    }

    return(NULL);
}

/**
 * xsltCheckStackElem:
 * @ctxt:  xn XSLT transformation context
 * @name:  the variable name
 * @nameURI:  the variable namespace URI
 *
 * check wether the variable or param is already defined
 *
 * Returns 1 if variable is present, 2 if param is present, 3 if this
 *         is an inherited param, 0 if not found, -1 in case of failure.
 */
static int
xsltCheckStackElem(xsltTransformContextPtr ctxt, const xmlChar *name,
	           const xmlChar *nameURI) {
    xsltStackElemPtr cur;

    if ((ctxt == NULL) || (name == NULL))
	return(-1);

    cur = xsltStackLookup(ctxt, name, nameURI);
    if (cur == NULL)
        return(0);
    if (cur->comp != NULL) {
        if (cur->comp->type == XSLT_FUNC_WITHPARAM)
	    return(3);
	else if (cur->comp->type == XSLT_FUNC_PARAM)
	    return(2);
    }
    
    return(1);
}

/**
 * xsltAddStackElem:
 * @ctxt:  xn XSLT transformation context
 * @elem:  a stack element
 *
 * add a new element at this level of the stack.
 *
 * Returns 0 in case of success, -1 in case of failure.
 */
static int
xsltAddStackElem(xsltTransformContextPtr ctxt, xsltStackElemPtr elem) {
    if ((ctxt == NULL) || (elem == NULL))
	return(-1);

    elem->next = ctxt->varsTab[ctxt->varsNr - 1];
    ctxt->varsTab[ctxt->varsNr - 1] = elem;
    ctxt->vars = elem;
    return(0);
}

/**
 * xsltAddStackElemList:
 * @ctxt:  xn XSLT transformation context
 * @elems:  a stack element list
 *
 * add the new element list at this level of the stack.
 *
 * Returns 0 in case of success, -1 in case of failure.
 */
int
xsltAddStackElemList(xsltTransformContextPtr ctxt, xsltStackElemPtr elems) {
    xsltStackElemPtr cur;

    if ((ctxt == NULL) || (elems == NULL))
	return(-1);

    /* TODO: check doublons */
    if (ctxt->varsTab[ctxt->varsNr - 1] != NULL) {
	cur = ctxt->varsTab[ctxt->varsNr - 1];
	while (cur->next != NULL)
	    cur = cur->next;
	cur->next = elems;
    } else {
	elems->next = ctxt->varsTab[ctxt->varsNr - 1];
	ctxt->varsTab[ctxt->varsNr - 1] = elems;
	ctxt->vars = elems;
    }
    return(0);
}

/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

/**
 * xsltEvalVariable:
 * @ctxt:  the XSLT transformation context
 * @elem:  the variable or parameter.
 * @precomp: pointer to precompiled data
 *
 * Evaluate a variable value.
 *
 * Returns the XPath Object value or NULL in case of error
 */
static xmlXPathObjectPtr
xsltEvalVariable(xsltTransformContextPtr ctxt, xsltStackElemPtr elem,
	         xsltStylePreCompPtr castedComp)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemVariablePtr precomp =
	(xsltStyleItemVariablePtr) castedComp;
#else
    xsltStylePreCompPtr precomp = castedComp;
#endif   
    xmlXPathObjectPtr result = NULL;
    int oldProximityPosition, oldContextSize;
    xmlNodePtr oldInst, oldNode;
    xsltDocumentPtr oldDoc;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;

    if ((ctxt == NULL) || (elem == NULL))
	return(NULL);

#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	"Evaluating variable %s\n", elem->name));
#endif
    if (elem->select != NULL) {
	xmlXPathCompExprPtr comp = NULL;

	if ((precomp != NULL) && (precomp->comp != NULL)) {
	    comp = precomp->comp;
	} else {
	    comp = xmlXPathCompile(elem->select);
	}
	if (comp == NULL)
	    return(NULL);
	oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
	oldContextSize = ctxt->xpathCtxt->contextSize;
	ctxt->xpathCtxt->node = (xmlNodePtr) ctxt->node;
	oldDoc = ctxt->document;
	oldNode = ctxt->node;
	oldInst = ctxt->inst;
	oldNsNr = ctxt->xpathCtxt->nsNr;
	oldNamespaces = ctxt->xpathCtxt->namespaces;
	if (precomp != NULL) {
	    ctxt->inst = precomp->inst;
#ifdef XSLT_REFACTORED
	    if (precomp->inScopeNs != NULL) {
		ctxt->xpathCtxt->namespaces = precomp->inScopeNs->list;
		ctxt->xpathCtxt->nsNr = precomp->inScopeNs->number;
	    } else {
		ctxt->xpathCtxt->namespaces = NULL;
		ctxt->xpathCtxt->nsNr = 0;
	    }
#else
	    ctxt->xpathCtxt->namespaces = precomp->nsList;
	    ctxt->xpathCtxt->nsNr = precomp->nsNr;
#endif
	} else {
	    ctxt->inst = NULL;
	    ctxt->xpathCtxt->namespaces = NULL;
	    ctxt->xpathCtxt->nsNr = 0;
	}
	result = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
	ctxt->xpathCtxt->contextSize = oldContextSize;
	ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	ctxt->xpathCtxt->nsNr = oldNsNr;
	ctxt->xpathCtxt->namespaces = oldNamespaces;
	ctxt->inst = oldInst;
	ctxt->node = oldNode;
	ctxt->document = oldDoc;
	if ((precomp == NULL) || (precomp->comp == NULL))
	    xmlXPathFreeCompExpr(comp);
	if (result == NULL) {
	    if (precomp == NULL)
		xsltTransformError(ctxt, NULL, NULL,
		    "Evaluating variable %s failed\n", elem->name);
	    else
		xsltTransformError(ctxt, NULL, precomp->inst,
		    "Evaluating variable %s failed\n", elem->name);
	    ctxt->state = XSLT_STATE_STOPPED;
#ifdef WITH_XSLT_DEBUG_VARIABLE
#ifdef LIBXML_DEBUG_ENABLED
	} else {
	    if ((xsltGenericDebugContext == stdout) ||
		(xsltGenericDebugContext == stderr))
		xmlXPathDebugDumpObject((FILE *)xsltGenericDebugContext,
					result, 0);
#endif
#endif
	}
    } else {
	if (elem->tree == NULL) {
	    result = xmlXPathNewCString("");
	} else {
	    /*
	     * This is a result tree fragment.
	     */
	    xmlDocPtr container;
	    xmlNodePtr oldInsert;
	    xmlDocPtr  oldoutput;

	    container = xsltCreateRVT(ctxt);
	    if (container == NULL)
		return(NULL);
	    /*
	     * Tag the subtree for removal once consumed
	     */
	    xsltRegisterTmpRVT(ctxt, container);
	    oldoutput = ctxt->output;
	    ctxt->output = container;
	    oldInsert = ctxt->insert;
	    ctxt->insert = (xmlNodePtr) container;
	    xsltApplyOneTemplate(ctxt, ctxt->node, elem->tree, NULL, NULL);
	    ctxt->insert = oldInsert;
	    ctxt->output = oldoutput;

	    result = xmlXPathNewValueTree((xmlNodePtr) container);
	    if (result == NULL) {
		result = xmlXPathNewCString("");
	    } else {
	        result->boolval = 0; /* Freeing is not handled there anymore */
	    }
#ifdef WITH_XSLT_DEBUG_VARIABLE
#ifdef LIBXML_DEBUG_ENABLED
	    if ((xsltGenericDebugContext == stdout) ||
		(xsltGenericDebugContext == stderr))
		xmlXPathDebugDumpObject((FILE *)xsltGenericDebugContext,
					result, 0);
#endif
#endif
	}
    }
    return(result);
}

/**
 * xsltEvalGlobalVariable:
 * @elem:  the variable or parameter.
 * @ctxt:  the XSLT transformation context
 *
 * Evaluate a global variable value.
 *
 * Returns the XPath Object value or NULL in case of error
 */
static xmlXPathObjectPtr
xsltEvalGlobalVariable(xsltStackElemPtr elem, xsltTransformContextPtr ctxt)
{
    xmlXPathObjectPtr result = NULL;
#ifdef XSLT_REFACTORED
    xsltStyleBasicItemVariablePtr precomp;
#else
    xsltStylePreCompPtr precomp;
#endif    
    int oldProximityPosition, oldContextSize;
    xmlDocPtr oldDoc;
    xmlNodePtr oldInst;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;
    const xmlChar *name;

    if ((ctxt == NULL) || (elem == NULL))
	return(NULL);
    if (elem->computed)
	return(elem->value);


#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	"Evaluating global variable %s\n", elem->name));
#endif

#ifdef WITH_DEBUGGER
    if ((ctxt->debugStatus != XSLT_DEBUG_NONE) &&
        elem->comp && elem->comp->inst)
        xslHandleDebugger(elem->comp->inst, NULL, NULL, ctxt);
#endif

    name = elem->name;
    elem->name = BAD_CAST "  being computed ... ";

#ifdef XSLT_REFACTORED
    precomp = (xsltStyleBasicItemVariablePtr) elem->comp;
#else
    precomp = elem->comp;
#endif

    /*
    * OPTIMIZE TODO: We should consider if instantiating global vars/params
    *  on a on-demand basis would be better. The vars/params don't
    *  need to be evaluated if never called; and in the case of
    *  global params, if values for such params are provided by the
    *  user.
    */
    if (elem->select != NULL) {
	xmlXPathCompExprPtr comp = NULL;

	if ((precomp != NULL) && (precomp->comp != NULL)) {
	    comp = precomp->comp;
	} else {
	    comp = xmlXPathCompile(elem->select);
	}
	if (comp == NULL) {
	    elem->name = name;
	    return(NULL);
	}
	oldDoc = ctxt->xpathCtxt->doc;
	oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
	oldContextSize = ctxt->xpathCtxt->contextSize;
	oldInst = ctxt->inst;
	oldNsNr = ctxt->xpathCtxt->nsNr;
	oldNamespaces = ctxt->xpathCtxt->namespaces;
	
	if (precomp != NULL) {
	    ctxt->inst = precomp->inst;
#ifdef XSLT_REFACTORED
	    if (precomp->inScopeNs != NULL) {
		ctxt->xpathCtxt->namespaces = precomp->inScopeNs->list;
		ctxt->xpathCtxt->nsNr = precomp->inScopeNs->number;
	    } else {
		ctxt->xpathCtxt->namespaces = NULL;
		ctxt->xpathCtxt->nsNr = 0;
	    }
#else
	    ctxt->xpathCtxt->namespaces = precomp->nsList;
	    ctxt->xpathCtxt->nsNr = precomp->nsNr;
#endif
	} else {
	    ctxt->inst = NULL;
	    ctxt->xpathCtxt->namespaces = NULL;
	    ctxt->xpathCtxt->nsNr = 0;
	}
	ctxt->xpathCtxt->doc = ctxt->tmpDoc;
	ctxt->xpathCtxt->node = (xmlNodePtr) ctxt->tmpDoc;
	result = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);

	ctxt->xpathCtxt->doc = oldDoc;
	ctxt->xpathCtxt->contextSize = oldContextSize;
	ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	ctxt->inst = oldInst;
	ctxt->xpathCtxt->nsNr = oldNsNr;
	ctxt->xpathCtxt->namespaces = oldNamespaces;
	if ((precomp == NULL) || (precomp->comp == NULL))
	    xmlXPathFreeCompExpr(comp);
	if (result == NULL) {
	    if (precomp == NULL)
		xsltTransformError(ctxt, NULL, NULL,
		    "Evaluating global variable %s failed\n", elem->name);
	    else
		xsltTransformError(ctxt, NULL, precomp->inst,
		    "Evaluating global variable %s failed\n", elem->name);
	    ctxt->state = XSLT_STATE_STOPPED;
#ifdef WITH_XSLT_DEBUG_VARIABLE
#ifdef LIBXML_DEBUG_ENABLED
	} else {
	    if ((xsltGenericDebugContext == stdout) ||
		(xsltGenericDebugContext == stderr))
		xmlXPathDebugDumpObject((FILE *)xsltGenericDebugContext,
					result, 0);
#endif
#endif
	}
    } else {
	if (elem->tree == NULL) {
	    result = xmlXPathNewCString("");
	} else {
	    /*
	     * This is a result tree fragment.
	     */
	    xmlDocPtr container;
	    xmlNodePtr oldInsert;
	    xmlDocPtr  oldoutput;

	    container = xsltCreateRVT(ctxt);
	    if (container == NULL)
		return(NULL);
	    /*
	     * Tag the subtree for removal once consumed
	     */
	    xsltRegisterTmpRVT(ctxt, container);
	    /*
	     * Save a pointer to the global variable for later cleanup
	     */
	    container->psvi = elem;
	    oldoutput = ctxt->output;
	    ctxt->output = container;
	    oldInsert = ctxt->insert;
	    ctxt->insert = (xmlNodePtr) container;
	    xsltApplyOneTemplate(ctxt, ctxt->node, elem->tree, NULL, NULL);
	    ctxt->insert = oldInsert;
	    ctxt->output = oldoutput;

	    result = xmlXPathNewValueTree((xmlNodePtr) container);
	    if (result == NULL) {
		result = xmlXPathNewCString("");
	    } else {
	        result->boolval = 0; /* Freeing is not handled there anymore */
	    }
#ifdef WITH_XSLT_DEBUG_VARIABLE
#ifdef LIBXML_DEBUG_ENABLED
	    if ((xsltGenericDebugContext == stdout) ||
		(xsltGenericDebugContext == stderr))
		xmlXPathDebugDumpObject((FILE *)xsltGenericDebugContext,
					result, 0);
#endif
#endif
	}
    }
    if (result != NULL) {
	elem->value = result;
	elem->computed = 1;
    }
    elem->name = name;
    return(result);
}

/**
 * xsltEvalGlobalVariables:
 * @ctxt:  the XSLT transformation context
 *
 * Evaluate the global variables of a stylesheet. This need to be
 * done on parsed stylesheets before starting to apply transformations
 *
 * Returns 0 in case of success, -1 in case of error
 */
int
xsltEvalGlobalVariables(xsltTransformContextPtr ctxt) {
    xsltStackElemPtr elem;
    xsltStylesheetPtr style;

    if ((ctxt == NULL) || (ctxt->document == NULL))
	return(-1);
 
#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	"Registering global variables\n"));
#endif

    ctxt->tmpDoc = ctxt->document->doc;
    ctxt->node = (xmlNodePtr) ctxt->document->doc;
    ctxt->xpathCtxt->contextSize = 1;
    ctxt->xpathCtxt->proximityPosition = 1;

    /*
     * Walk the list from the stylesheets and populate the hash table
     */
    style = ctxt->style;
    while (style != NULL) {
	elem = style->variables;
	
#ifdef WITH_XSLT_DEBUG_VARIABLE
	if ((style->doc != NULL) && (style->doc->URL != NULL)) {
	    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
			     "Registering global variables from %s\n",
		             style->doc->URL));
	}
#endif

	while (elem != NULL) {
	    xsltStackElemPtr def;

	    /*
	     * Global variables are stored in the variables pool.
	     */
	    def = (xsltStackElemPtr) 
		    xmlHashLookup2(ctxt->globalVars,
		                 elem->name, elem->nameURI);
	    if (def == NULL) {

		def = xsltCopyStackElem(elem);
		xmlHashAddEntry2(ctxt->globalVars,
				 elem->name, elem->nameURI, def);
	    } else if ((elem->comp != NULL) &&
		       (elem->comp->type == XSLT_FUNC_VARIABLE)) {
		/*
		 * Redefinition of variables from a different stylesheet
		 * should not generate a message.
		 */
		if ((elem->comp->inst != NULL) &&
		    (def->comp != NULL) && (def->comp->inst != NULL) &&
		    (elem->comp->inst->doc == def->comp->inst->doc)) {
		    xsltTransformError(ctxt, style, elem->comp->inst,
			"Global variable %s already defined\n", elem->name);
		    if (style != NULL) style->errors++;
		}
	    }
	    elem = elem->next;
	}

	style = xsltNextImport(style);
    }

    /*
     * This part does the actual evaluation
     */
    ctxt->node = (xmlNodePtr) ctxt->document->doc;
    ctxt->xpathCtxt->contextSize = 1;
    ctxt->xpathCtxt->proximityPosition = 1;
    xmlHashScan(ctxt->globalVars,
	        (xmlHashScanner) xsltEvalGlobalVariable, ctxt);

    return(0);
}

/**
 * xsltRegisterGlobalVariable:
 * @style:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 * @sel:  the expression which need to be evaluated to generate a value
 * @tree:  the subtree if sel is NULL
 * @comp:  the precompiled value
 * @value:  the string value if available
 *
 * Register a new variable value. If @value is NULL it unregisters
 * the variable
 *
 * Returns 0 in case of success, -1 in case of error
 */
static int
xsltRegisterGlobalVariable(xsltStylesheetPtr style, const xmlChar *name,
		     const xmlChar *ns_uri, const xmlChar *sel,
		     xmlNodePtr tree, xsltStylePreCompPtr comp,
		     const xmlChar *value) {
    xsltStackElemPtr elem, tmp;
    if (style == NULL)
	return(-1);
    if (name == NULL)
	return(-1);
    if (comp == NULL)
	return(-1);

#ifdef WITH_XSLT_DEBUG_VARIABLE
    if (comp->type == XSLT_FUNC_PARAM)
	xsltGenericDebug(xsltGenericDebugContext,
			 "Defining global param %s\n", name);
    else
	xsltGenericDebug(xsltGenericDebugContext,
			 "Defining global variable %s\n", name);
#endif

    elem = xsltNewStackElem();
    if (elem == NULL)
	return(-1);
    elem->comp = comp;
    elem->name = xmlDictLookup(style->dict, name, -1);
    elem->select = xmlDictLookup(style->dict, sel, -1);
    if (ns_uri)
	elem->nameURI = xmlDictLookup(style->dict, ns_uri, -1);
    elem->tree = tree;
    tmp = style->variables;
    if (tmp == NULL) {
	elem->next = NULL;
	style->variables = elem;
    } else {
	while (tmp != NULL) {
	    if ((elem->comp->type == XSLT_FUNC_VARIABLE) &&
		(tmp->comp->type == XSLT_FUNC_VARIABLE) &&
		(xmlStrEqual(elem->name, tmp->name)) &&
		((elem->nameURI == tmp->nameURI) ||
		 (xmlStrEqual(elem->nameURI, tmp->nameURI)))) {
		xsltTransformError(NULL, style, comp->inst,
		"redefinition of global variable %s\n", elem->name);
		if (style != NULL) style->errors++;
	    }
	    if (tmp->next == NULL)
	        break;
	    tmp = tmp->next;
	}
	elem->next = NULL;
	tmp->next = elem;
    }
    if (value != NULL) {
	elem->computed = 1;
	elem->value = xmlXPathNewString(value);
    }
    return(0);
}

/**
 * xsltProcessUserParamInternal
 *
 * @ctxt:  the XSLT transformation context
 * @name:  a null terminated parameter name
 * @value: a null terminated value (may be an XPath expression)
 * @eval:  0 to treat the value literally, else evaluate as XPath expression
 *
 * If @eval is 0 then @value is treated literally and is stored in the global
 * parameter/variable table without any change.
 *
 * Uf @eval is 1 then @value is treated as an XPath expression and is
 * evaluated.  In this case, if you want to pass a string which will be
 * interpreted literally then it must be enclosed in single or double quotes.
 * If the string contains single quotes (double quotes) then it cannot be
 * enclosed single quotes (double quotes).  If the string which you want to
 * be treated literally contains both single and double quotes (e.g. Meet
 * at Joe's for "Twelfth Night" at 7 o'clock) then there is no suitable
 * quoting character.  You cannot use &apos; or &quot; inside the string
 * because the replacement of character entities with their equivalents is
 * done at a different stage of processing.  The solution is to call
 * xsltQuoteUserParams or xsltQuoteOneUserParam.
 *
 * This needs to be done on parsed stylesheets before starting to apply
 * transformations.  Normally this will be called (directly or indirectly)
 * only from xsltEvalUserParams, xsltEvalOneUserParam, xsltQuoteUserParams,
 * or xsltQuoteOneUserParam.
 *
 * Returns 0 in case of success, -1 in case of error
 */

static
int
xsltProcessUserParamInternal(xsltTransformContextPtr ctxt,
		             const xmlChar * name,
			     const xmlChar * value,
			     int eval) {

    xsltStylesheetPtr style;
    const xmlChar *prefix;
    const xmlChar *href;
    xmlXPathCompExprPtr comp;
    xmlXPathObjectPtr result;
    int oldProximityPosition;
    int oldContextSize;
    int oldNsNr;
    xmlNsPtr *oldNamespaces;
    xsltStackElemPtr elem;
    int res;
    void *res_ptr;

    if (ctxt == NULL)
	return(-1);
    if (name == NULL)
	return(0);
    if (value == NULL)
	return(0);

    style = ctxt->style;

#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	    "Evaluating user parameter %s=%s\n", name, value));
#endif

    /*
     * Name lookup
     */

    name = xsltSplitQName(ctxt->dict, name, &prefix);
    href = NULL;
    if (prefix != NULL) {
	xmlNsPtr ns;

	ns = xmlSearchNs(style->doc, xmlDocGetRootElement(style->doc),
			 prefix);
	if (ns == NULL) {
	    xsltTransformError(ctxt, style, NULL,
	    "user param : no namespace bound to prefix %s\n", prefix);
	    href = NULL;
	} else {
	    href = ns->href;
	}
    }

    if (name == NULL)
	return (-1);

    res_ptr = xmlHashLookup2(ctxt->globalVars, name, href);
    if (res_ptr != 0) {
	xsltTransformError(ctxt, style, NULL,
	    "Global parameter %s already defined\n", name);
    }
    if (ctxt->globalVars == NULL)
	ctxt->globalVars = xmlHashCreate(20);

    /*
     * do not overwrite variables with parameters from the command line
     */
    while (style != NULL) {
        elem = ctxt->style->variables;
	while (elem != NULL) {
	    if ((elem->comp != NULL) &&
	        (elem->comp->type == XSLT_FUNC_VARIABLE) &&
		(xmlStrEqual(elem->name, name)) &&
		(xmlStrEqual(elem->nameURI, href))) {
		return(0);
	    }
            elem = elem->next;
	}
        style = xsltNextImport(style);
    }
    style = ctxt->style;
    elem = NULL;

    /*
     * Do the evaluation if @eval is non-zero.
     */

    result = NULL;
    if (eval != 0) {
        comp = xmlXPathCompile(value);
	if (comp != NULL) {
	    oldProximityPosition = ctxt->xpathCtxt->proximityPosition;
	    oldContextSize = ctxt->xpathCtxt->contextSize;
	    ctxt->xpathCtxt->node = (xmlNodePtr) ctxt->node;

	    /* 
	     * There is really no in scope namespace for parameters on the
	     * command line.
	     */

	    oldNsNr = ctxt->xpathCtxt->nsNr;
	    oldNamespaces = ctxt->xpathCtxt->namespaces;
	    ctxt->xpathCtxt->namespaces = NULL;
	    ctxt->xpathCtxt->nsNr = 0;
	    result = xmlXPathCompiledEval(comp, ctxt->xpathCtxt);
	    ctxt->xpathCtxt->contextSize = oldContextSize;
	    ctxt->xpathCtxt->proximityPosition = oldProximityPosition;
	    ctxt->xpathCtxt->nsNr = oldNsNr;
	    ctxt->xpathCtxt->namespaces = oldNamespaces;
	    xmlXPathFreeCompExpr(comp);
	}
	if (result == NULL) {
	    xsltTransformError(ctxt, style, NULL,
		"Evaluating user parameter %s failed\n", name);
	    ctxt->state = XSLT_STATE_STOPPED;
	    return(-1);
	}
    }

    /* 
     * If @eval is 0 then @value is to be taken literally and result is NULL
     * 
     * If @eval is not 0, then @value is an XPath expression and has been
     * successfully evaluated and result contains the resulting value and
     * is not NULL.
     *
     * Now create an xsltStackElemPtr for insertion into the context's
     * global variable/parameter hash table.
     */

#ifdef WITH_XSLT_DEBUG_VARIABLE
#ifdef LIBXML_DEBUG_ENABLED
    if ((xsltGenericDebugContext == stdout) ||
        (xsltGenericDebugContext == stderr))
	    xmlXPathDebugDumpObject((FILE *)xsltGenericDebugContext,
				    result, 0);
#endif
#endif

    elem = xsltNewStackElem();
    if (elem != NULL) {
	elem->name = name;
	elem->select = xmlDictLookup(ctxt->dict, value, -1);
	if (href != NULL)
	    elem->nameURI = xmlDictLookup(ctxt->dict, href, -1);
	elem->tree = NULL;
	elem->computed = 1;
	if (eval == 0) {
	    elem->value = xmlXPathNewString(value);
	} 
	else {
	    elem->value = result;
	}
    }

    /*
     * Global parameters are stored in the XPath context variables pool.
     */

    res = xmlHashAddEntry2(ctxt->globalVars, name, href, elem);
    if (res != 0) {
	xsltFreeStackElem(elem);
	xsltTransformError(ctxt, style, NULL,
	    "Global parameter %s already defined\n", name);
    }
    return(0);
}

/**
 * xsltEvalUserParams:
 *
 * @ctxt:  the XSLT transformation context
 * @params:  a NULL terminated array of parameters name/value tuples
 *
 * Evaluate the global variables of a stylesheet. This needs to be
 * done on parsed stylesheets before starting to apply transformations.
 * Each of the parameters is evaluated as an XPath expression and stored
 * in the global variables/parameter hash table.  If you want your
 * parameter used literally, use xsltQuoteUserParams.
 *
 * Returns 0 in case of success, -1 in case of error
 */
 
int
xsltEvalUserParams(xsltTransformContextPtr ctxt, const char **params) {
    int indx = 0;
    const xmlChar *name;
    const xmlChar *value;

    if (params == NULL)
	return(0);
    while (params[indx] != NULL) {
	name = (const xmlChar *) params[indx++];
	value = (const xmlChar *) params[indx++];
    	if (xsltEvalOneUserParam(ctxt, name, value) != 0) 
	    return(-1);
    }
    return 0;
}

/**
 * xsltQuoteUserParams:
 *
 * @ctxt:  the XSLT transformation context
 * @params:  a NULL terminated arry of parameters names/values tuples
 *
 * Similar to xsltEvalUserParams, but the values are treated literally and
 * are * *not* evaluated as XPath expressions. This should be done on parsed
 * stylesheets before starting to apply transformations.
 *
 * Returns 0 in case of success, -1 in case of error.
 */
 
int
xsltQuoteUserParams(xsltTransformContextPtr ctxt, const char **params) {
    int indx = 0;
    const xmlChar *name;
    const xmlChar *value;

    if (params == NULL)
	return(0);
    while (params[indx] != NULL) {
	name = (const xmlChar *) params[indx++];
	value = (const xmlChar *) params[indx++];
    	if (xsltQuoteOneUserParam(ctxt, name, value) != 0) 
	    return(-1);
    }
    return 0;
}

/**
 * xsltEvalOneUserParam:
 * @ctxt:  the XSLT transformation context
 * @name:  a null terminated string giving the name of the parameter
 * @value:  a null terminated string giving the XPath expression to be evaluated
 *
 * This is normally called from xsltEvalUserParams to process a single
 * parameter from a list of parameters.  The @value is evaluated as an
 * XPath expression and the result is stored in the context's global
 * variable/parameter hash table.
 *
 * To have a parameter treated literally (not as an XPath expression)
 * use xsltQuoteUserParams (or xsltQuoteOneUserParam).  For more
 * details see description of xsltProcessOneUserParamInternal.
 *
 * Returns 0 in case of success, -1 in case of error.
 */

int
xsltEvalOneUserParam(xsltTransformContextPtr ctxt,
    		     const xmlChar * name,
		     const xmlChar * value) {
    return xsltProcessUserParamInternal(ctxt, name, value,
		                        1 /* xpath eval ? */);
}

/**
 * xsltQuoteOneUserParam:
 * @ctxt:  the XSLT transformation context
 * @name:  a null terminated string giving the name of the parameter
 * @value:  a null terminated string giving the parameter value
 *
 * This is normally called from xsltQuoteUserParams to process a single
 * parameter from a list of parameters.  The @value is stored in the
 * context's global variable/parameter hash table.
 *
 * Returns 0 in case of success, -1 in case of error.
 */

int
xsltQuoteOneUserParam(xsltTransformContextPtr ctxt,
			 const xmlChar * name,
			 const xmlChar * value) {
    return xsltProcessUserParamInternal(ctxt, name, value,
					0 /* xpath eval ? */);
}

/**
 * xsltBuildVariable:
 * @ctxt:  the XSLT transformation context
 * @comp:  the precompiled form
 * @tree:  the tree if select is NULL
 *
 * Computes a new variable value.
 *
 * Returns the xsltStackElemPtr or NULL in case of error
 */
static xsltStackElemPtr
xsltBuildVariable(xsltTransformContextPtr ctxt,
		  xsltStylePreCompPtr castedComp,
		  xmlNodePtr tree)
{
#ifdef XSLT_REFACTORED
    xsltStyleBasicItemVariablePtr comp =
	(xsltStyleBasicItemVariablePtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif 
    xsltStackElemPtr elem;

#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
		     "Building variable %s", comp->name));
    if (comp->select != NULL)
	XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
			 " select %s", comp->select));
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext, "\n"));
#endif

    elem = xsltNewStackElem();
    if (elem == NULL)
	return(NULL);
    elem->comp = (xsltStylePreCompPtr) comp;
    elem->name = comp->name;
    if (comp->select != NULL)
	elem->select = comp->select;
    else
	elem->select = NULL;
    if (comp->ns)
	elem->nameURI = comp->ns;
    elem->tree = tree;
    if (elem->computed == 0) {
	elem->value = xsltEvalVariable(ctxt, elem,
	    (xsltStylePreCompPtr) comp);
	if (elem->value != NULL)
	    elem->computed = 1;
    }
    return(elem);
}

/**
 * xsltRegisterVariable:
 * @ctxt:  the XSLT transformation context
 * @comp:  pointer to precompiled data
 * @tree:  the tree if select is NULL
 * @param:  this is a parameter actually
 *
 * Computes and register a new variable value.
 * TODO: Is this intended for xsl:param as well?
 *
 * Returns 0 in case of success, -1 in case of error
 */
static int
xsltRegisterVariable(xsltTransformContextPtr ctxt,
		     xsltStylePreCompPtr castedComp,
		     xmlNodePtr tree, int param)
{
#ifdef XSLT_REFACTORED
    xsltStyleBasicItemVariablePtr comp =
	(xsltStyleBasicItemVariablePtr) castedComp;
#else
    xsltStylePreCompPtr comp = castedComp;
#endif
    xsltStackElemPtr elem;
    int present;

    present = xsltCheckStackElem(ctxt, comp->name, comp->ns);
    if (param == 0) {
	if ((present != 0) && (present != 3)) {
	    xsltTransformError(ctxt, NULL, comp->inst,
		"xsl:variable : redefining %s\n", comp->name);
	    return(0);
	}
    } else if (present != 0) {
	if ((present == 1) || (present == 2)) {
	    xsltTransformError(ctxt, NULL, comp->inst,
		"xsl:param : redefining %s\n", comp->name);
	    return(0);
	}
#ifdef WITH_XSLT_DEBUG_VARIABLE
	XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
		 "param %s defined by caller\n", comp->name));
#endif
	return(0);
    }
    elem = xsltBuildVariable(ctxt, (xsltStylePreCompPtr) comp, tree);
    xsltAddStackElem(ctxt, elem);
    return(0);
}

/**
 * xsltGlobalVariableLookup:
 * @ctxt:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 *
 * Search in the Variable array of the context for the given
 * variable value.
 *
 * Returns the value or NULL if not found
 */
static xmlXPathObjectPtr
xsltGlobalVariableLookup(xsltTransformContextPtr ctxt, const xmlChar *name,
		         const xmlChar *ns_uri) {
    xsltStackElemPtr elem;
    xmlXPathObjectPtr ret = NULL;

    /*
     * Lookup the global variables in XPath global variable hash table
     */
    if ((ctxt->xpathCtxt == NULL) || (ctxt->globalVars == NULL))
	return(NULL);
    elem = (xsltStackElemPtr)
	    xmlHashLookup2(ctxt->globalVars, name, ns_uri);
    if (elem == NULL) {
#ifdef WITH_XSLT_DEBUG_VARIABLE
	XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
			 "global variable not found %s\n", name));
#endif
	return(NULL);
    }
    if (elem->computed == 0) {
	if (xmlStrEqual(elem->name, BAD_CAST "  being computed ... ")) {
	    xsltTransformError(ctxt, NULL, elem->comp->inst,
		"Recursive definition of %s\n", name);
	    return(NULL);
	}
	ret = xsltEvalGlobalVariable(elem, ctxt);
    } else
	ret = elem->value;
    return(xmlXPathObjectCopy(ret));
}

/**
 * xsltVariableLookup:
 * @ctxt:  the XSLT transformation context
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 *
 * Search in the Variable array of the context for the given
 * variable value.
 *
 * Returns the value or NULL if not found
 */
xmlXPathObjectPtr
xsltVariableLookup(xsltTransformContextPtr ctxt, const xmlChar *name,
		   const xmlChar *ns_uri) {
    xsltStackElemPtr elem;

    if (ctxt == NULL)
	return(NULL);

    elem = xsltStackLookup(ctxt, name, ns_uri);
    if (elem == NULL) {
	return(xsltGlobalVariableLookup(ctxt, name, ns_uri));
    }
    if (elem->computed == 0) {
#ifdef WITH_XSLT_DEBUG_VARIABLE
	XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
		         "uncomputed variable %s\n", name));
#endif
        elem->value = xsltEvalVariable(ctxt, elem, NULL);
	elem->computed = 1;
    }
    if (elem->value != NULL)
	return(xmlXPathObjectCopy(elem->value));
#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
		     "variable not found %s\n", name));
#endif
    return(NULL);
}

/**
 * xsltParseStylesheetCallerParam:
 * @ctxt:  the XSLT transformation context
 * @cur:  the "xsl:with-param" element
 *
 * parse an XSLT transformation param declaration, compute
 * its value but doesn't record it.
 * NOTE that this is also called with an *xsl:param* element
 * from exsltFuncFunctionFunction(). 
 *
 * Returns the new xsltStackElemPtr or NULL
 */

xsltStackElemPtr
xsltParseStylesheetCallerParam(xsltTransformContextPtr ctxt, xmlNodePtr cur)
{
#ifdef XSLT_REFACTORED
    xsltStyleBasicItemVariablePtr comp;
#else
    xsltStylePreCompPtr comp;
#endif
    xmlNodePtr tree = NULL;
    xsltStackElemPtr elem = NULL;
    
    if ((cur == NULL) || (ctxt == NULL))
	return(NULL);
#ifdef XSLT_REFACTORED
    comp = (xsltStyleBasicItemVariablePtr) cur->psvi;
#else
    comp = (xsltStylePreCompPtr) cur->psvi;
#endif     
    if (comp == NULL) {
	xsltTransformError(ctxt, NULL, cur,
	    "xsl:with-param : compilation error\n");
	return(NULL);
    }

    if (comp->name == NULL) {
	xsltTransformError(ctxt, NULL, cur,
	    "xsl:with-param : missing name attribute\n");
	return(NULL);
    }

#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	    "Handling xsl:with-param %s\n", comp->name));
#endif

    if (comp->select == NULL) {
	tree = cur->children;
    } else {
#ifdef WITH_XSLT_DEBUG_VARIABLE
	XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	    "        select %s\n", comp->select));
#endif
	tree = cur;
    }

    elem = xsltBuildVariable(ctxt, (xsltStylePreCompPtr) comp, tree);

    return(elem);
}

/**
 * xsltParseGlobalVariable:
 * @style:  the XSLT stylesheet
 * @cur:  the "variable" element
 *
 * parse an XSLT transformation variable declaration and record
 * its value.
 */

void
xsltParseGlobalVariable(xsltStylesheetPtr style, xmlNodePtr cur)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemVariablePtr comp;
#else
    xsltStylePreCompPtr comp;
#endif

    if ((cur == NULL) || (style == NULL))
	return;
    
#ifdef XSLT_REFACTORED
    /*
    * Note that xsltStylePreCompute() will be called from
    * xslt.c only.
    */
    comp = (xsltStyleItemVariablePtr) cur->psvi;
#else
    xsltStylePreCompute(style, cur);
    comp = (xsltStylePreCompPtr) cur->psvi;
#endif
    if (comp == NULL) {
	xsltTransformError(NULL, style, cur,
	     "xsl:variable : compilation failed\n");
	return;
    }

    if (comp->name == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:variable : missing name attribute\n");
	return;
    }

    /*
    * Parse the content (a sequence constructor) of xsl:variable.
    */
    if (cur->children != NULL) {
#ifdef XSLT_REFACTORED	
        xsltParseSequenceConstructor(XSLT_CCTXT(style), cur->children);
#else
        xsltParseTemplateContent(style, cur);
#endif
    }
#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Registering global variable %s\n", comp->name);
#endif

    xsltRegisterGlobalVariable(style, comp->name, comp->ns,
	comp->select, cur->children, (xsltStylePreCompPtr) comp,
	NULL);
}

/**
 * xsltParseGlobalParam:
 * @style:  the XSLT stylesheet
 * @cur:  the "param" element
 *
 * parse an XSLT transformation param declaration and record
 * its value.
 */

void
xsltParseGlobalParam(xsltStylesheetPtr style, xmlNodePtr cur) {
#ifdef XSLT_REFACTORED
    xsltStyleItemParamPtr comp;
#else
    xsltStylePreCompPtr comp;
#endif

    if ((cur == NULL) || (style == NULL))
	return;
    
#ifdef XSLT_REFACTORED
    /*
    * Note that xsltStylePreCompute() will be called from
    * xslt.c only.
    */
    comp = (xsltStyleItemParamPtr) cur->psvi;
#else
    xsltStylePreCompute(style, cur);
    comp = (xsltStylePreCompPtr) cur->psvi;
#endif    
    if (comp == NULL) {
	xsltTransformError(NULL, style, cur,
	     "xsl:param : compilation failed\n");
	return;
    }

    if (comp->name == NULL) {
	xsltTransformError(NULL, style, cur,
	    "xsl:param : missing name attribute\n");
	return;
    }

    /*
    * Parse the content (a sequence constructor) of xsl:param.
    */
    if (cur->children != NULL) {
#ifdef XSLT_REFACTORED	
        xsltParseSequenceConstructor(XSLT_CCTXT(style), cur->children);
#else
        xsltParseTemplateContent(style, cur);
#endif
    }

#ifdef WITH_XSLT_DEBUG_VARIABLE
    xsltGenericDebug(xsltGenericDebugContext,
	"Registering global param %s\n", comp->name);
#endif

    xsltRegisterGlobalVariable(style, comp->name, comp->ns,
	comp->select, cur->children, (xsltStylePreCompPtr) comp,
	NULL);
}

/**
 * xsltParseStylesheetVariable:
 * @ctxt:  the XSLT transformation context
 * @cur:  the "variable" element
 *
 * parse an XSLT transformation variable declaration and record
 * its value.
 */

void
xsltParseStylesheetVariable(xsltTransformContextPtr ctxt, xmlNodePtr cur)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemVariablePtr comp;
#else
    xsltStylePreCompPtr comp;
#endif

    if ((cur == NULL) || (ctxt == NULL))
	return;

#ifdef XSLT_REFACTORED
    comp = (xsltStyleItemVariablePtr) cur->psvi;
#else
    comp = (xsltStylePreCompPtr) cur->psvi;
#endif   
    if (comp == NULL) {
	xsltTransformError(ctxt, NULL, cur,
	     "xsl:variable : compilation failed\n");
	return;
    }

    if (comp->name == NULL) {
	xsltTransformError(ctxt, NULL, cur,
	    "xsl:variable : missing name attribute\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	"Registering variable %s\n", comp->name));
#endif

    xsltRegisterVariable(ctxt, (xsltStylePreCompPtr) comp, cur->children, 0);
}

/**
 * xsltParseStylesheetParam:
 * @ctxt:  the XSLT transformation context
 * @cur:  the "param" element
 *
 * parse an XSLT transformation param declaration and record
 * its value.
 */

void
xsltParseStylesheetParam(xsltTransformContextPtr ctxt, xmlNodePtr cur)
{
#ifdef XSLT_REFACTORED
    xsltStyleItemParamPtr comp;
#else
    xsltStylePreCompPtr comp;
#endif

    if ((cur == NULL) || (ctxt == NULL))
	return;
#ifdef XSLT_REFACTORED
    comp = (xsltStyleItemParamPtr) cur->psvi;
#else
    comp = (xsltStylePreCompPtr) cur->psvi;
#endif 
    if (comp == NULL) {
	xsltTransformError(ctxt, NULL, cur,
	     "xsl:param : compilation failed\n");
	return;
    }

    if (comp->name == NULL) {
	xsltTransformError(ctxt, NULL, cur,
	    "xsl:param : missing name attribute\n");
	return;
    }

#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(ctxt,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	"Registering param %s\n", comp->name));
#endif

    xsltRegisterVariable(ctxt, (xsltStylePreCompPtr) comp, cur->children, 1);
}

/**
 * xsltFreeGlobalVariables:
 * @ctxt:  the XSLT transformation context
 *
 * Free up the data associated to the global variables
 * its value.
 */

void
xsltFreeGlobalVariables(xsltTransformContextPtr ctxt) {
    xmlHashFree(ctxt->globalVars, (xmlHashDeallocator) xsltFreeStackElem);
}

/**
 * xsltXPathVariableLookup:
 * @ctxt:  a void * but the the XSLT transformation context actually
 * @name:  the variable name
 * @ns_uri:  the variable namespace URI
 *
 * This is the entry point when a varibale is needed by the XPath
 * interpretor.
 *
 * Returns the value or NULL if not found
 */
xmlXPathObjectPtr
xsltXPathVariableLookup(void *ctxt, const xmlChar *name,
	                const xmlChar *ns_uri) {
    xsltTransformContextPtr context;
    xmlXPathObjectPtr ret;

    if ((ctxt == NULL) || (name == NULL))
	return(NULL);

#ifdef WITH_XSLT_DEBUG_VARIABLE
    XSLT_TRACE(((xsltTransformContextPtr)ctxt),XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	    "Lookup variable %s\n", name));
#endif
    context = (xsltTransformContextPtr) ctxt;
    ret = xsltVariableLookup(context, name, ns_uri);
    if (ret == NULL) {
	xsltTransformError(ctxt, NULL, NULL,
	    "unregistered variable %s\n", name);
    }
#ifdef WITH_XSLT_DEBUG_VARIABLE
    if (ret != NULL)
	XSLT_TRACE(context,XSLT_TRACE_VARIABLES,xsltGenericDebug(xsltGenericDebugContext,
	    "found variable %s\n", name));
#endif
    return(ret);
}


