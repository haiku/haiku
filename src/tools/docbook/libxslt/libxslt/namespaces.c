/*
 * namespaces.c: Implementation of the XSLT namespaces handling
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

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif
#ifdef HAVE_FLOAT_H
#include <float.h>
#endif
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_NAN_H
#include <nan.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/tree.h>
#include <libxml/hash.h>
#include <libxml/xmlerror.h>
#include <libxml/uri.h>
#include "xslt.h"
#include "xsltInternals.h"
#include "xsltutils.h"
#include "namespaces.h"
#include "imports.h"



/************************************************************************
 *									*
 *			Module interfaces				*
 *									*
 ************************************************************************/

#ifdef XSLT_REFACTORED  
static xsltNsAliasPtr
xsltNewNsAlias(xsltCompilerCtxtPtr cctxt)
{
    xsltNsAliasPtr ret;

    if (cctxt == NULL)
	return(NULL);

    ret = (xsltNsAliasPtr) xmlMalloc(sizeof(xsltNsAlias));
    if (ret == NULL) {
	xsltTransformError(NULL, cctxt->style, NULL,
	    "Internal error in xsltNewNsAlias(): Memory allocation failed.\n");
	cctxt->style->errors++;
	return(NULL);
    }
    memset(ret, 0, sizeof(xsltNsAlias));    
    /*
    * TODO: Store the item at current stylesheet-level.
    */
    ret->next = cctxt->nsAliases;
    cctxt->nsAliases = ret;       

    return(ret);
}
#endif /* XSLT_REFACTORED */
/**
 * xsltNamespaceAlias:
 * @style:  the XSLT stylesheet
 * @node:  the xsl:namespace-alias node
 *
 * Read the stylesheet-prefix and result-prefix attributes, register
 * them as well as the corresponding namespace.
 */
void
xsltNamespaceAlias(xsltStylesheetPtr style, xmlNodePtr node)
{
    xmlChar *resultPrefix = NULL;
    xmlChar *stylePrefix = NULL;
    xmlNsPtr literalNs = NULL;
    xmlNsPtr targetNs = NULL;
 
#ifdef XSLT_REFACTORED 
    xsltNsAliasPtr alias;

    if ((style == NULL) || (node == NULL))
	return;

    /*
    * SPEC XSLT 1.0:
    *  "If a namespace URI is declared to be an alias for multiple
    *  different namespace URIs, then the declaration with the highest
    *  import precedence is used. It is an error if there is more than
    *  one such declaration. An XSLT processor may signal the error;
    *  if it does not signal the error, it must recover by choosing,
    *  from amongst the declarations with the highest import precedence,
    *  the one that occurs last in the stylesheet."
    *
    * SPEC TODO: Check for the errors mentioned above.
    */
    /*
    * NOTE that the XSLT 2.0 also *does* use the NULL namespace if
    *  "#default" is used and there's no default namespace is scope.
    *  I.e., this is *not* an error. 
    *  Most XSLT 1.0 implementations work this way.
    *  The XSLT 1.0 spec has nothing to say on the subject. 
    */
    /*
    * Attribute "stylesheet-prefix".
    */
    stylePrefix = xmlGetNsProp(node, (const xmlChar *)"stylesheet-prefix", NULL);
    if (stylePrefix == NULL) {
	xsltTransformError(NULL, style, node,
	    "The attribute 'stylesheet-prefix' is missing.\n");
	return;
    }
    if (xmlStrEqual(stylePrefix, (const xmlChar *)"#default"))
	literalNs = xmlSearchNs(node->doc, node, NULL);	
    else {
	literalNs = xmlSearchNs(node->doc, node, stylePrefix);
	if (literalNs == NULL) {
	    xsltTransformError(NULL, style, node,
	        "Attribute 'stylesheet-prefix': There's no namespace "
		"declaration in scope for the prefix '%s'.\n",
		    stylePrefix);
	    goto error;
	}
    }
    /*
    * Attribute "result-prefix".
    */
    resultPrefix = xmlGetNsProp(node, (const xmlChar *)"result-prefix", NULL);
    if (resultPrefix == NULL) {
	xsltTransformError(NULL, style, node,
	    "The attribute 'result-prefix' is missing.\n");
	goto error;
    }        
    if (xmlStrEqual(resultPrefix, (const xmlChar *)"#default"))
	targetNs = xmlSearchNs(node->doc, node, NULL);
    else {
	targetNs = xmlSearchNs(node->doc, node, resultPrefix);

        if (targetNs == NULL) {
	   xsltTransformError(NULL, style, node,
	        "Attribute 'result-prefix': There's no namespace "
		"declaration in scope for the prefix '%s'.\n",
		    stylePrefix);
	    goto error;
	}
    }
    /*
     *
     * Same alias for multiple different target namespace URIs:
     *  TODO: The one with the highest import precedence is used.
     *  Example:
     *  <xsl:namespace-alias stylesheet-prefix="foo"
     *                       result-prefix="bar"/>
     *
     *  <xsl:namespace-alias stylesheet-prefix="foo"
     *                       result-prefix="zar"/>
     *
     * Same target namespace URI for multiple different aliases:
     *  All alias-definitions will be used.
     *  Example:
     *  <xsl:namespace-alias stylesheet-prefix="bar"
     *                       result-prefix="foo"/>
     *
     *  <xsl:namespace-alias stylesheet-prefix="zar"
     *                       result-prefix="foo"/>
     * Cases using #default:
     *  <xsl:namespace-alias stylesheet-prefix="#default"
     *                       result-prefix="#default"/>
     *  TODO: Has this an effect at all?
     *
     *  <xsl:namespace-alias stylesheet-prefix="foo"
     *                       result-prefix="#default"/>
     *  From namespace to no namespace.
     *
     *  <xsl:namespace-alias stylesheet-prefix="#default"
     *                       result-prefix="foo"/>
     *  From no namespace to namespace.
     */
    
	
	/*
	* Store the ns-node in the alias-object.
	*/
	alias = xsltNewNsAlias(XSLT_CCTXT(style));
	if (alias == NULL)
	    return;
	alias->literalNs = literalNs;
	alias->targetNs = targetNs;
	XSLT_CCTXT(style)->hasNsAliases = 1;


#else /* XSLT_REFACTORED */
    const xmlChar *literalNsName;
    const xmlChar *targetNsName;
    

    if ((style == NULL) || (node == NULL))
	return;

    stylePrefix = xmlGetNsProp(node, (const xmlChar *)"stylesheet-prefix", NULL);
    if (stylePrefix == NULL) {
	xsltTransformError(NULL, style, node,
	    "namespace-alias: stylesheet-prefix attribute missing\n");
	return;
    }
    resultPrefix = xmlGetNsProp(node, (const xmlChar *)"result-prefix", NULL);
    if (resultPrefix == NULL) {
	xsltTransformError(NULL, style, node,
	    "namespace-alias: result-prefix attribute missing\n");
	goto error;
    }
    
    if (xmlStrEqual(stylePrefix, (const xmlChar *)"#default")) {
	literalNs = xmlSearchNs(node->doc, node, NULL);
	if (literalNs == NULL) {
	    literalNsName = NULL;
	} else
	    literalNsName = literalNs->href; /* Yes - set for nsAlias table */
    } else {
	literalNs = xmlSearchNs(node->doc, node, stylePrefix);
 
	if ((literalNs == NULL) || (literalNs->href == NULL)) {
	    xsltTransformError(NULL, style, node,
	        "namespace-alias: prefix %s not bound to any namespace\n",
					stylePrefix);
	    goto error;
	} else
	    literalNsName = literalNs->href;
    }

    /*
     * When "#default" is used for result, if a default namespace has not
     * been explicitly declared the special value UNDEFINED_DEFAULT_NS is
     * put into the nsAliases table
     */
    if (xmlStrEqual(resultPrefix, (const xmlChar *)"#default")) {
	targetNs = xmlSearchNs(node->doc, node, NULL);
	if (targetNs == NULL) {
	    targetNsName = UNDEFINED_DEFAULT_NS;
	} else
	    targetNsName = targetNs->href;
    } else {
	targetNs = xmlSearchNs(node->doc, node, resultPrefix);

        if ((targetNs == NULL) || (targetNs->href == NULL)) {
	    xsltTransformError(NULL, style, node,
	        "namespace-alias: prefix %s not bound to any namespace\n",
					resultPrefix);
	    goto error;
	} else
	    targetNsName = targetNs->href;
    }
    /*
     * Special case: if #default is used for
     *  the stylesheet-prefix (literal namespace) and there's no default
     *  namespace in scope, we'll use style->defaultAlias for this.
     */   
    if (literalNsName == NULL) {
        if (targetNs != NULL) {
	    /*
	    * BUG TODO: Is it not sufficient to have only 1 field for
	    *  this, since subsequently alias declarations will
	    *  overwrite this.	    
	    *  Example:
	    *   <xsl:namespace-alias result-prefix="foo"
	    *                        stylesheet-prefix="#default"/>
	    *   <xsl:namespace-alias result-prefix="bar"
	    *                        stylesheet-prefix="#default"/>
	    *  The mapping for "foo" won't be visible anymore.
	    */
            style->defaultAlias = targetNs->href;
	}
    } else {
        if (style->nsAliases == NULL)
	    style->nsAliases = xmlHashCreate(10);
        if (style->nsAliases == NULL) {
	    xsltTransformError(NULL, style, node,
	        "namespace-alias: cannot create hash table\n");
	    goto error;
        }
	xmlHashAddEntry((xmlHashTablePtr) style->nsAliases,
	    literalNsName, (void *) targetNsName);
    }
#endif /* else of XSLT_REFACTORED */

error:
    if (stylePrefix != NULL)
	xmlFree(stylePrefix);
    if (resultPrefix != NULL)
	xmlFree(resultPrefix);
}

/**
 * xsltNsInScope:
 * @doc:  the document
 * @node:  the current node
 * @ancestor:  the ancestor carrying the namespace
 * @prefix:  the namespace prefix
 *
 * Copy of xmlNsInScope which is not public ...
 * 
 * Returns 1 if true, 0 if false and -1 in case of error.
 */
static int
xsltNsInScope(xmlDocPtr doc ATTRIBUTE_UNUSED, xmlNodePtr node,
             xmlNodePtr ancestor, const xmlChar * prefix)
{
    xmlNsPtr tst;

    while ((node != NULL) && (node != ancestor)) {
        if ((node->type == XML_ENTITY_REF_NODE) ||
            (node->type == XML_ENTITY_NODE) ||
            (node->type == XML_ENTITY_DECL))
            return (-1);
        if (node->type == XML_ELEMENT_NODE) {
            tst = node->nsDef;
            while (tst != NULL) {
                if ((tst->prefix == NULL)
                    && (prefix == NULL))
                    return (0);
                if ((tst->prefix != NULL)
                    && (prefix != NULL)
                    && (xmlStrEqual(tst->prefix, prefix)))
                    return (0);
                tst = tst->next;
            }
        }
        node = node->parent;
    }
    if (node != ancestor)
        return (-1);
    return (1);
}

/**
 * xsltSearchPlainNsByHref:
 * @doc:  the document
 * @node:  the current node
 * @href:  the namespace value
 *
 * Search a Ns aliasing a given URI and without a NULL prefix.
 * Recurse on the parents until it finds
 * the defined namespace or return NULL otherwise.
 * Returns the namespace pointer or NULL.
 */
static xmlNsPtr
xsltSearchPlainNsByHref(xmlDocPtr doc, xmlNodePtr node, const xmlChar * href)
{
    xmlNsPtr cur;
    xmlNodePtr orig = node;

    if ((node == NULL) || (href == NULL))
        return (NULL);

    while (node != NULL) {
        if ((node->type == XML_ENTITY_REF_NODE) ||
            (node->type == XML_ENTITY_NODE) ||
            (node->type == XML_ENTITY_DECL))
            return (NULL);
        if (node->type == XML_ELEMENT_NODE) {
            cur = node->nsDef;
            while (cur != NULL) {
                if ((cur->href != NULL) && (cur->prefix != NULL) &&
		    (href != NULL) && (xmlStrEqual(cur->href, href))) {
		    if (xsltNsInScope(doc, orig, node, cur->href) == 1)
			return (cur);
                }
                cur = cur->next;
            }
            if (orig != node) {
                cur = node->ns;
                if (cur != NULL) {
                    if ((cur->href != NULL) && (cur->prefix != NULL) &&
		        (href != NULL) && (xmlStrEqual(cur->href, href))) {
			if (xsltNsInScope(doc, orig, node, cur->href) == 1)
			    return (cur);
                    }
                }
            }    
        }
        node = node->parent;
    }
    return (NULL);
}

/**
 * xsltGetPlainNamespace:
 * @ctxt:  a transformation context
 * @cur:  the input node
 * @ns:  the namespace
 * @out:  the output node (or its parent)
 *
 * Find the right namespace value for this prefix, if needed create
 * and add a new namespace decalaration on the node
 * Handle namespace aliases and make sure the prefix is not NULL, this
 * is needed for attributes.
 * Called from:
 *   xsltAttrTemplateProcess() (templates.c)
 *   xsltCopyProp() (transform.c)
 *
 * Returns the namespace node to use or NULL
 */
xmlNsPtr
xsltGetPlainNamespace(xsltTransformContextPtr ctxt, xmlNodePtr cur,
                      xmlNsPtr ns, xmlNodePtr out) {    
    xmlNsPtr ret;
    const xmlChar *URI = NULL; /* the replacement URI */

    if ((ctxt == NULL) || (cur == NULL) || (out == NULL) || (ns == NULL))
	return(NULL);

#ifdef XSLT_REFACTORED
    /*
    * Namespace exclusion and ns-aliasing is performed at
    * compilation-time in the refactored code.
    */
    URI = ns->href;
#else
    {
	xsltStylesheetPtr style;
	style = ctxt->style;
	while (style != NULL) {
	    if (style->nsAliases != NULL)
		URI = (const xmlChar *) xmlHashLookup(style->nsAliases, ns->href);
	    if (URI != NULL)
		break;
	    
	    style = xsltNextImport(style);
	}
    }

    if (URI == UNDEFINED_DEFAULT_NS) {
        xmlNsPtr dflt;
	dflt = xmlSearchNs(cur->doc, cur, NULL);
        if (dflt == NULL)
	    return NULL;
	else
	    URI = dflt->href;
    }

    if (URI == NULL)
	URI = ns->href;
#endif

    if ((out->parent != NULL) &&
	(out->parent->type == XML_ELEMENT_NODE) &&
	(out->parent->ns != NULL) &&
	(out->parent->ns->prefix != NULL) &&
	(xmlStrEqual(out->parent->ns->href, URI)))
	ret = out->parent->ns;
    else {
	if (ns->prefix != NULL) {
	    ret = xmlSearchNs(out->doc, out, ns->prefix);
	    if ((ret == NULL) || (!xmlStrEqual(ret->href, URI)) ||
	        (ret->prefix == NULL)) {
		ret = xsltSearchPlainNsByHref(out->doc, out, URI);
	    }
	} else {
	    ret = xsltSearchPlainNsByHref(out->doc, out, URI);
	}
    }

    if (ret == NULL) {
	if (out->type == XML_ELEMENT_NODE)
	    ret = xmlNewNs(out, URI, ns->prefix);
    }
    return(ret);
}

/**
 * xsltGetSpecialNamespace:
 * @ctxt:  a transformation context
 * @cur:  the input node
 * @URI:  the namespace URI
 * @prefix:  the suggested prefix
 * @out:  the output node (or its parent)
 *
 * Find the right namespace value for this URI, if needed create
 * and add a new namespace decalaration on the node
 *
 * Returns the namespace node to use or NULL
 */
xmlNsPtr
xsltGetSpecialNamespace(xsltTransformContextPtr ctxt, xmlNodePtr cur,
		const xmlChar *URI, const xmlChar *prefix, xmlNodePtr out) {
    xmlNsPtr ret;
    static int prefixno = 1;
    char nprefix[10];

    if ((ctxt == NULL) || (cur == NULL) || (out == NULL) || (URI == NULL))
	return(NULL);

    if ((prefix == NULL) && (URI[0] == 0)) {
	/*
	* This tries to "undeclare" a default namespace.
	* This fixes a part of bug #302020:
	*  1) Added a check whether the queried ns-decl
	*     is already an "undeclaration" of the default
	*     namespace.
	*  2) This fires an error if the default namespace
	*     couldn't be "undeclared".	
	*/
	ret = xmlSearchNs(out->doc, out, NULL);
	if ((ret == NULL) ||
	    (ret->href == NULL) || (ret->href[0] == 0))
	    return(ret);

	if (ret != NULL) {
	    xmlNsPtr newns;

	    newns = xmlNewNs(out, URI, prefix);
	    if (newns == NULL) {
		xsltTransformError(ctxt, NULL, cur,
		    "Namespace fixup error: Failed to undeclare "
		    "the default namespace '%s'.\n",
		    ret->href);
	    }
	    /*
	    * TODO: Why does this try to return an xmlns="" at all?
	    */
	    return(newns);
	}
	return(NULL);
    }

    if ((out->parent != NULL) &&
	(out->parent->type == XML_ELEMENT_NODE) &&
	(out->parent->ns != NULL) &&
	(xmlStrEqual(out->parent->ns->href, URI)))
    {
	ret = out->parent->ns;
    } else 
	ret = xmlSearchNsByHref(out->doc, out, URI);

    if ((ret == NULL) || (ret->prefix == NULL)) {
	if (prefix == NULL) {
	    do {
		sprintf(nprefix, "ns%d", prefixno++);
		ret = xmlSearchNs(out->doc, out, (xmlChar *)nprefix);
	    } while (ret != NULL);
	    prefix = (const xmlChar *) &nprefix[0];
	} else if ((ret != NULL) && (ret->prefix == NULL)) {
	    /* found ns but no prefix - search for the prefix */
	    ret = xmlSearchNs(out->doc, out, prefix);
	    if (ret != NULL)
	        return(ret);	/* found it */
	}
	if (out->type == XML_ELEMENT_NODE)
	    ret = xmlNewNs(out, URI, prefix);
    }
    return(ret);
}

/**
 * xsltGetNamespace:
 * @ctxt:  a transformation context
 * @cur:  the input node
 * @ns:  the namespace
 * @out:  the output node (or its parent)
 *
 * REFACTORED NOTE: Won't be used anymore in the refactored code
 *  for literal result elements/attributes.
 *
 * Find the right namespace value for this prefix, if needed create
 * and add a new namespace decalaration on the node
 * Handle namespace aliases
 *
 * Returns the namespace node to use or NULL
 */
xmlNsPtr
xsltGetNamespace(xsltTransformContextPtr ctxt, xmlNodePtr cur, xmlNsPtr ns,
	         xmlNodePtr out) {    
    xmlNsPtr ret;
    const xmlChar *URI = NULL; /* the replacement URI */

    if ((ctxt == NULL) || (cur == NULL) || (out == NULL) || (ns == NULL))
	return(NULL);
    
#ifdef XSLT_REFACTORED
    /*
    * Namespace exclusion and ns-aliasing is performed at
    * compilation-time in the refactored code.    
    */
    URI = ns->href;
#else
    {
	xsltStylesheetPtr style;
	style = ctxt->style;
	while (style != NULL) {
	    if (style->nsAliases != NULL)
		URI = (const xmlChar *) 
		xmlHashLookup(style->nsAliases, ns->href);
	    if (URI != NULL)
		break;
	    
	    style = xsltNextImport(style);
	}
    }

    if (URI == UNDEFINED_DEFAULT_NS) {
        xmlNsPtr dflt;
	/*
	*/
	dflt = xmlSearchNs(cur->doc, cur, NULL);
	if (dflt != NULL)
	    URI = dflt->href;
	else
	    return NULL;
    } else if (URI == NULL)
	URI = ns->href;
#endif
    /*
     * If the parent is an XML_ELEMENT_NODE, and has the "equivalent"
     * namespace as ns (either both default, or both with a prefix
     * with the same href) then return the parent's ns record
     */
    if ((out->parent != NULL) &&
	(out->parent->type == XML_ELEMENT_NODE) &&
	(out->parent->ns != NULL) &&
	(((out->parent->ns->prefix == NULL) && (ns->prefix == NULL)) ||
	 ((out->parent->ns->prefix != NULL) && (ns->prefix != NULL))) &&
	(xmlStrEqual(out->parent->ns->href, URI)))
	ret = out->parent->ns;
    else {
        /*
	 * do a standard namespace search for ns in the output doc
	 */
        ret = xmlSearchNs(out->doc, out, ns->prefix);
	if ((ret != NULL) && (!xmlStrEqual(ret->href, URI)))
	    ret = NULL;

	/*
	 * if the search fails and it's not for the default prefix
	 * do a search by href
	 */
	if ((ret == NULL) && (ns->prefix != NULL))
	    ret = xmlSearchNsByHref(out->doc, out, URI);
    }

    /*
     * For an element node, if we don't find it, or it's the default
     * and this element already defines a default (bug 165560), we need to
     * create it.
     */
    if (out->type == XML_ELEMENT_NODE) {
	if ((ret == NULL) || ((ret->prefix == NULL) && (out->ns == NULL) &&
	    (out->nsDef != NULL) && (!xmlStrEqual(URI, out->nsDef->href)))) {
	        ret = xmlNewNs(out, URI, ns->prefix);
	}
    }
    return(ret);
}

/**
 * xsltCopyNamespaceList:
 * @ctxt:  a transformation context
 * @node:  the target node
 * @cur:  the first namespace
 *
 * Do a copy of an namespace list. If @node is non-NULL the
 * new namespaces are added automatically. This handles namespaces
 * aliases.
 * This function is intended only for *internal* use at
 * transformation-time. Use it *only* for copying ns-decls of
 * literal result elements.
 * 
 * Called by:
 *   xsltCopyTree() (transform.c)
 *   xsltCopyNode() (transform.c)
 *
 * Returns: a new xmlNsPtr, or NULL in case of error.
 */
xmlNsPtr
xsltCopyNamespaceList(xsltTransformContextPtr ctxt, xmlNodePtr node,
	              xmlNsPtr cur) {
    xmlNsPtr ret = NULL, tmp;
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
	 * Avoid duplicating namespace declarations in the tree if
	 * a matching declaration is in scope.
	 */
	if (node != NULL) {
	    if ((node->ns != NULL) &&
		(xmlStrEqual(node->ns->prefix, cur->prefix)) &&
        	(xmlStrEqual(node->ns->href, cur->href))) {
		cur = cur->next;
		continue;
	    }
	    tmp = xmlSearchNs(node->doc, node, cur->prefix);
	    if ((tmp != NULL) && (xmlStrEqual(tmp->href, cur->href))) {
		cur = cur->next;
		continue;
	    }
	}
#ifdef XSLT_REFACTORED
	/*
	* Namespace exclusion and ns-aliasing is performed at
	* compilation-time in the refactored code.
	*/
	q = xmlNewNs(node, cur->href, cur->prefix);
	if (p == NULL) {
	    ret = p = q;
	} else {
	    p->next = q;
	    p = q;
	}
#else
	if (!xmlStrEqual(cur->href, XSLT_NAMESPACE)) {
	    const xmlChar *URI;
	    /* TODO apply cascading */
	    URI = (const xmlChar *) xmlHashLookup(ctxt->style->nsAliases,
		                                  cur->href);
	    if (URI == UNDEFINED_DEFAULT_NS)
	        continue;
	    if (URI != NULL) {
		q = xmlNewNs(node, URI, cur->prefix);
	    } else {
		q = xmlNewNs(node, cur->href, cur->prefix);
	    }
	    if (p == NULL) {
		ret = p = q;
	    } else {
		p->next = q;
		p = q;
	    }
	}
#endif
	cur = cur->next;
    }
    return(ret);
}

/**
 * xsltCopyNamespace:
 * @ctxt:  a transformation context
 * @node:  the target node
 * @cur:  the namespace node
 *
 * Do a copy of an namespace node. If @node is non-NULL the
 * new namespaces are added automatically. This handles namespaces
 * aliases
 *
 * Returns: a new xmlNsPtr, or NULL in case of error.
 */
xmlNsPtr
xsltCopyNamespace(xsltTransformContextPtr ctxt, xmlNodePtr node,
	          xmlNsPtr cur) {
    xmlNsPtr ret = NULL;
    
    if (cur == NULL)
	return(NULL);
    if (cur->type != XML_NAMESPACE_DECL)
	return(NULL);

    /*
     * One can add namespaces only on element nodes
     */
    if ((node != NULL) && (node->type != XML_ELEMENT_NODE))
	node = NULL;

#ifdef XSLT_REFACTORED
    /*
    * Namespace exclusion and ns-aliasing is performed at
    * compilation-time in the refactored code.
    */
    ret = xmlNewNs(node, cur->href, cur->prefix);	
#else
    if (!xmlStrEqual(cur->href, XSLT_NAMESPACE)) {
	const xmlChar *URI;

	URI = (const xmlChar *) xmlHashLookup(ctxt->style->nsAliases,
					      cur->href);
	if (URI == UNDEFINED_DEFAULT_NS)
	    return(NULL);
	if (URI != NULL) {
	    ret = xmlNewNs(node, URI, cur->prefix);
	} else {
	    ret = xmlNewNs(node, cur->href, cur->prefix);
	}
    }
#endif
    return(ret);
}


/**
 * xsltFreeNamespaceAliasHashes:
 * @style: an XSLT stylesheet
 *
 * Free up the memory used by namespaces aliases
 */
void
xsltFreeNamespaceAliasHashes(xsltStylesheetPtr style) {
    if (style->nsAliases != NULL)
	xmlHashFree((xmlHashTablePtr) style->nsAliases, NULL);
    style->nsAliases = NULL;
}
