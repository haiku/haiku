/* Generated */

#include <libxslt/xsltconfig.h>
#include "libxslt_wrap.h"
#include "libxslt-py.h"

PyObject *
libxslt_xsltTransformGetInstruction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetInstruction", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->inst;
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltParseStylesheetImportedDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseStylesheetImportedDoc", &pyobj_doc, &pyobj_style))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = xsltParseStylesheetImportedDoc(doc, style);
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltGenerateIdFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltGenerateIdFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltGenerateIdFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltApplyAttributeSet(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNodePtr inst;
    PyObject *pyobj_inst;
    xmlChar * attributes;

    if (!PyArg_ParseTuple(args, (char *)"OOOz:xsltApplyAttributeSet", &pyobj_ctxt, &pyobj_node, &pyobj_inst, &attributes))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    inst = (xmlNodePtr) PyxmlNode_Get(pyobj_inst);

    xsltApplyAttributeSet(ctxt, node, inst, attributes);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltAttrListTemplateProcess(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr target;
    PyObject *pyobj_target;
    xmlAttrPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xsltAttrListTemplateProcess", &pyobj_ctxt, &pyobj_target, &pyobj_cur))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    target = (xmlNodePtr) PyxmlNode_Get(pyobj_target);
    cur = (xmlAttrPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xsltAttrListTemplateProcess(ctxt, target, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltParseStylesheetAttributeSet(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseStylesheetAttributeSet", &pyobj_style, &pyobj_cur))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xsltParseStylesheetAttributeSet(style, cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltUnparsedEntityURIFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltUnparsedEntityURIFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltUnparsedEntityURIFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltMessage(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNodePtr inst;
    PyObject *pyobj_inst;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xsltMessage", &pyobj_ctxt, &pyobj_node, &pyobj_inst))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    inst = (xmlNodePtr) PyxmlNode_Get(pyobj_inst);

    xsltMessage(ctxt, node, inst);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltCreateRVT(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltCreateRVT", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltCreateRVT(ctxt);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltParseStylesheetProcess(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xsltStylesheetPtr ret;
    PyObject *pyobj_ret;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseStylesheetProcess", &pyobj_ret, &pyobj_doc))
        return(NULL);
    ret = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_ret);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xsltParseStylesheetProcess(ret, doc);
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltGetDebuggerStatus(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    int c_retval;

    c_retval = xsltGetDebuggerStatus();
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltStylesheetGetVersion(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetVersion", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->version;
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltXPathGetTransformContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltTransformContextPtr c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltXPathGetTransformContext", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = ctxt->extra;
    py_retval = libxslt_xsltTransformContextPtrWrap((xsltTransformContextPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltStylesheetGetImports(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetImports", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->imports;
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltAttrTemplateValueProcess(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * str;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xsltAttrTemplateValueProcess", &pyobj_ctxt, &str))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltAttrTemplateValueProcess(ctxt, str);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltNextImport(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xsltStylesheetPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltNextImport", &pyobj_cur))
        return(NULL);
    cur = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_cur);

    c_retval = xsltNextImport(cur);
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltFreeCtxtExts(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeCtxtExts", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    xsltFreeCtxtExts(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltNamespaceAlias(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltNamespaceAlias", &pyobj_style, &pyobj_node))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    xsltNamespaceAlias(style, node);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltTransformGetModeURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetModeURI", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->modeURI;
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltFreeDocuments(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeDocuments", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    xsltFreeDocuments(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltDocumentFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltDocumentFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltDocumentFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltNewStylesheet(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;

    c_retval = xsltNewStylesheet();
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltSetDebuggerStatus(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    int value;

    if (!PyArg_ParseTuple(args, (char *)"i:xsltSetDebuggerStatus", &value))
        return(NULL);

    xsltSetDebuggerStatus(value);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltCopyNamespaceList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNsPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xsltCopyNamespaceList", &pyobj_ctxt, &pyobj_node, &pyobj_cur))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    cur = (xmlNsPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xsltCopyNamespaceList(ctxt, node, cur);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltEvalGlobalVariables(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltEvalGlobalVariables", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltEvalGlobalVariables(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltCleanupGlobals(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xsltCleanupGlobals();
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltParseTemplateContent(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr templ;
    PyObject *pyobj_templ;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseTemplateContent", &pyobj_style, &pyobj_templ))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    templ = (xmlNodePtr) PyxmlNode_Get(pyobj_templ);

    xsltParseTemplateContent(style, templ);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltEvalOneUserParam(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xsltEvalOneUserParam", &pyobj_ctxt, &name, &value))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltEvalOneUserParam(ctxt, name, value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltUnregisterExtModuleFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * name;
    xmlChar * URI;

    if (!PyArg_ParseTuple(args, (char *)"zz:xsltUnregisterExtModuleFunction", &name, &URI))
        return(NULL);

    c_retval = xsltUnregisterExtModuleFunction(name, URI);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltFreeGlobalVariables(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeGlobalVariables", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    xsltFreeGlobalVariables(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltSaveResultToFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    FILE * file;
    PyObject *pyobj_file;
    xmlDocPtr result;
    PyObject *pyobj_result;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xsltSaveResultToFile", &pyobj_file, &pyobj_result, &pyobj_style))
        return(NULL);
    file = (FILE *) PyFile_Get(pyobj_file);
    result = (xmlDocPtr) PyxmlNode_Get(pyobj_result);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = xsltSaveResultToFile(file, result, style);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltParseGlobalVariable(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseGlobalVariable", &pyobj_style, &pyobj_cur))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xsltParseGlobalVariable(style, cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltShutdownExts(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltShutdownExts", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltShutdownExts(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFindElemSpaceHandling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltFindElemSpaceHandling", &pyobj_ctxt, &pyobj_node))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xsltFindElemSpaceHandling(ctxt, node);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltStylesheetGetParent(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetParent", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->parent;
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltSaveResultToFd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int fd;
    xmlDocPtr result;
    PyObject *pyobj_result;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"iOO:xsltSaveResultToFd", &fd, &pyobj_result, &pyobj_style))
        return(NULL);
    result = (xmlDocPtr) PyxmlNode_Get(pyobj_result);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = xsltSaveResultToFd(fd, result, style);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltCopyNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNsPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xsltCopyNamespace", &pyobj_ctxt, &pyobj_node, &pyobj_cur))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    cur = (xmlNsPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xsltCopyNamespace(ctxt, node, cur);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltCalibrateAdjust(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    long delta;

    if (!PyArg_ParseTuple(args, (char *)"i:xsltCalibrateAdjust", &delta))
        return(NULL);

    xsltCalibrateAdjust(delta);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltSaveProfiling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    FILE * output;
    PyObject *pyobj_output;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltSaveProfiling", &pyobj_ctxt, &pyobj_output))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    output = (FILE *) PyFile_Get(pyobj_output);

    xsltSaveProfiling(ctxt, output);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltStylesheetGetMethod(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetMethod", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->method;
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltFunctionNodeSet(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltFunctionNodeSet", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltFunctionNodeSet(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltCleanupTemplates(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltCleanupTemplates", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltCleanupTemplates(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltCheckExtPrefix(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlChar * prefix;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xsltCheckExtPrefix", &pyobj_style, &prefix))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = xsltCheckExtPrefix(style, prefix);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltParseStylesheetFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xmlChar * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xsltParseStylesheetFile", &filename))
        return(NULL);

    c_retval = xsltParseStylesheetFile(filename);
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltElementAvailableFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltElementAvailableFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltElementAvailableFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltRegisterExtras(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltRegisterExtras", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    xsltRegisterExtras(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFunctionAvailableFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltFunctionAvailableFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltFunctionAvailableFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFormatNumberFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltFormatNumberFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltFormatNumberFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltEvalTemplateString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNodePtr parent;
    PyObject *pyobj_parent;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xsltEvalTemplateString", &pyobj_ctxt, &pyobj_node, &pyobj_parent))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    parent = (xmlNodePtr) PyxmlNode_Get(pyobj_parent);

    c_retval = xsltEvalTemplateString(ctxt, node, parent);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltGetNsProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;
    xmlChar * nameSpace;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xsltGetNsProp", &pyobj_node, &name, &nameSpace))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xsltGetNsProp(node, name, nameSpace);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltAddKey(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlChar * name;
    xmlChar * nameURI;
    xmlChar * match;
    xmlChar * use;
    xmlNodePtr inst;
    PyObject *pyobj_inst;

    if (!PyArg_ParseTuple(args, (char *)"OzzzzO:xsltAddKey", &pyobj_style, &name, &nameURI, &match, &use, &pyobj_inst))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    inst = (xmlNodePtr) PyxmlNode_Get(pyobj_inst);

    c_retval = xsltAddKey(style, name, nameURI, match, use, inst);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltRegisterAllFunctions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltRegisterAllFunctions", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    xsltRegisterAllFunctions(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltAttrTemplateValueProcessNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * str;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"OzO:xsltAttrTemplateValueProcessNode", &pyobj_ctxt, &str, &pyobj_node))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xsltAttrTemplateValueProcessNode(ctxt, str, node);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltSystemPropertyFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltSystemPropertyFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltSystemPropertyFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltUnregisterExtModuleTopLevel(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * name;
    xmlChar * URI;

    if (!PyArg_ParseTuple(args, (char *)"zz:xsltUnregisterExtModuleTopLevel", &name, &URI))
        return(NULL);

    c_retval = xsltUnregisterExtModuleTopLevel(name, URI);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltParseStylesheetImport(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseStylesheetImport", &pyobj_style, &pyobj_cur))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xsltParseStylesheetImport(style, cur);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltXPathParserGetContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathContextPtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltXPathParserGetContext", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = ctxt->context;
    py_retval = libxml_xmlXPathContextPtrWrap((xmlXPathContextPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltAllocateExtraCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltAllocateExtraCtxt", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltAllocateExtraCtxt(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltTransformGetContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathContextPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetContext", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->xpathCtxt;
    py_retval = libxml_xmlXPathContextPtrWrap((xmlXPathContextPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltRegisterTestModule(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xsltRegisterTestModule();
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFreeKeys(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeKeys", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltFreeKeys(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltParseStylesheetVariable(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseStylesheetVariable", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xsltParseStylesheetVariable(ctxt, cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltGetSpecialNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlChar * URI;
    xmlChar * prefix;
    xmlNodePtr out;
    PyObject *pyobj_out;

    if (!PyArg_ParseTuple(args, (char *)"OOzzO:xsltGetSpecialNamespace", &pyobj_ctxt, &pyobj_cur, &URI, &prefix, &pyobj_out))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);
    out = (xmlNodePtr) PyxmlNode_Get(pyobj_out);

    c_retval = xsltGetSpecialNamespace(ctxt, cur, URI, prefix, out);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltLoadStylesheetPI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltLoadStylesheetPI", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xsltLoadStylesheetPI(doc);
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltStylesheetGetDoctypePublic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetDoctypePublic", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->doctypePublic;
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltAllocateExtra(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltAllocateExtra", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = xsltAllocateExtra(style);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltAttrTemplateProcess(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr target;
    PyObject *pyobj_target;
    xmlAttrPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xsltAttrTemplateProcess", &pyobj_ctxt, &pyobj_target, &pyobj_cur))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    target = (xmlNodePtr) PyxmlNode_Get(pyobj_target);
    cur = (xmlAttrPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xsltAttrTemplateProcess(ctxt, target, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltNeedElemSpaceHandling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltNeedElemSpaceHandling", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltNeedElemSpaceHandling(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltTransformGetOutputDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetOutputDoc", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->output;
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltInitCtxtExts(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltInitCtxtExts", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltInitCtxtExts(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltStylesheetGetEncoding(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetEncoding", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->encoding;
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltTransformGetMode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetMode", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->mode;
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltTransformGetOutputURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetOutputURI", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->outputFile;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltFreeExts(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeExts", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltFreeExts(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltParseGlobalParam(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseGlobalParam", &pyobj_style, &pyobj_cur))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xsltParseGlobalParam(style, cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltParseStylesheetParam(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseStylesheetParam", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xsltParseStylesheetParam(ctxt, cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltParseStylesheetOutput(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseStylesheetOutput", &pyobj_style, &pyobj_cur))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xsltParseStylesheetOutput(style, cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFreeStylePreComps(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeStylePreComps", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltFreeStylePreComps(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFreeNamespaceAliasHashes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeNamespaceAliasHashes", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltFreeNamespaceAliasHashes(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltShutdownCtxtExts(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltShutdownCtxtExts", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    xsltShutdownCtxtExts(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltGetPlainNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlNodePtr out;
    PyObject *pyobj_out;

    if (!PyArg_ParseTuple(args, (char *)"OOOO:xsltGetPlainNamespace", &pyobj_ctxt, &pyobj_cur, &pyobj_ns, &pyobj_out))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);
    out = (xmlNodePtr) PyxmlNode_Get(pyobj_out);

    c_retval = xsltGetPlainNamespace(ctxt, cur, ns, out);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltQuoteOneUserParam(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xsltQuoteOneUserParam", &pyobj_ctxt, &name, &value))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltQuoteOneUserParam(ctxt, name, value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltUnregisterExtModule(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * URI;

    if (!PyArg_ParseTuple(args, (char *)"z:xsltUnregisterExtModule", &URI))
        return(NULL);

    c_retval = xsltUnregisterExtModule(URI);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltTransformGetInsertNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetInsertNode", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->insert;
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltStylesheetGetNext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetNext", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->next;
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltUnregisterExtModuleElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * name;
    xmlChar * URI;

    if (!PyArg_ParseTuple(args, (char *)"zz:xsltUnregisterExtModuleElement", &name, &URI))
        return(NULL);

    c_retval = xsltUnregisterExtModuleElement(name, URI);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltTimestamp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    long c_retval;

    c_retval = xsltTimestamp();
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltParseStylesheetInclude(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltParseStylesheetInclude", &pyobj_style, &pyobj_cur))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xsltParseStylesheetInclude(style, cur);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltFreeTemplateHashes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeTemplateHashes", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltFreeTemplateHashes(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltSaveResultToFilename(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * URL;
    xmlDocPtr result;
    PyObject *pyobj_result;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    int compression;

    if (!PyArg_ParseTuple(args, (char *)"zOOi:xsltSaveResultToFilename", &URL, &pyobj_result, &pyobj_style, &compression))
        return(NULL);
    result = (xmlDocPtr) PyxmlNode_Get(pyobj_result);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = xsltSaveResultToFilename(URL, result, style, compression);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltSetCtxtParseOptions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltSetCtxtParseOptions", &pyobj_ctxt, &options))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltSetCtxtParseOptions(ctxt, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltGetProfileInformation(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltGetProfileInformation", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltGetProfileInformation(ctxt);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltFreeStyleDocuments(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeStyleDocuments", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltFreeStyleDocuments(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltRegisterPersistRVT(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr RVT;
    PyObject *pyobj_RVT;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltRegisterPersistRVT", &pyobj_ctxt, &pyobj_RVT))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    RVT = (xmlDocPtr) PyxmlNode_Get(pyobj_RVT);

    c_retval = xsltRegisterPersistRVT(ctxt, RVT);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltResolveStylesheetAttributeSet(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltResolveStylesheetAttributeSet", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltResolveStylesheetAttributeSet(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltGetNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlNodePtr out;
    PyObject *pyobj_out;

    if (!PyArg_ParseTuple(args, (char *)"OOOO:xsltGetNamespace", &pyobj_ctxt, &pyobj_cur, &pyobj_ns, &pyobj_out))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);
    out = (xmlNodePtr) PyxmlNode_Get(pyobj_out);

    c_retval = xsltGetNamespace(ctxt, cur, ns, out);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltEvalAttrValueTemplate(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;
    xmlChar * ns;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xsltEvalAttrValueTemplate", &pyobj_ctxt, &pyobj_node, &name, &ns))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xsltEvalAttrValueTemplate(ctxt, node, name, ns);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltTransformGetCurrent(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetCurrent", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->node;
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltRegisterTmpRVT(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr RVT;
    PyObject *pyobj_RVT;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltRegisterTmpRVT", &pyobj_ctxt, &pyobj_RVT))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    RVT = (xmlDocPtr) PyxmlNode_Get(pyobj_RVT);

    c_retval = xsltRegisterTmpRVT(ctxt, RVT);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltPrintErrorContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xsltPrintErrorContext", &pyobj_ctxt, &pyobj_style, &pyobj_node))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    xsltPrintErrorContext(ctxt, style, node);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltStylesheetGetDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetDoc", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->doc;
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltGetCNsProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;
    xmlChar * nameSpace;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xsltGetCNsProp", &pyobj_style, &pyobj_node, &name, &nameSpace))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xsltGetCNsProp(style, node, name, nameSpace);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltDebugDumpExtensions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltDebugDumpExtensions", &pyobj_output))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);

    xsltDebugDumpExtensions(output);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltStylePreCompute(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlNodePtr inst;
    PyObject *pyobj_inst;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltStylePreCompute", &pyobj_style, &pyobj_inst))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    inst = (xmlNodePtr) PyxmlNode_Get(pyobj_inst);

    xsltStylePreCompute(style, inst);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFreeStylesheet(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr sheet;
    PyObject *pyobj_sheet;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeStylesheet", &pyobj_sheet))
        return(NULL);
    sheet = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_sheet);

    xsltFreeStylesheet(sheet);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFreeAttributeSetsHashes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeAttributeSetsHashes", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    xsltFreeAttributeSetsHashes(style);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltRegisterAllExtras(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xsltRegisterAllExtras();
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltTransformGetStyle(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltTransformGetStyle", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->style;
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltCompileAttr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlAttrPtr attr;
    PyObject *pyobj_attr;

    if (!PyArg_ParseTuple(args, (char *)"OO:xsltCompileAttr", &pyobj_style, &pyobj_attr))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);
    attr = (xmlAttrPtr) PyxmlNode_Get(pyobj_attr);

    xsltCompileAttr(style, attr);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltFreeRVTs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltFreeRVTs", &pyobj_ctxt))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    xsltFreeRVTs(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltIsBlank(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * str;

    if (!PyArg_ParseTuple(args, (char *)"z:xsltIsBlank", &str))
        return(NULL);

    c_retval = xsltIsBlank(str);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltStylesheetGetDoctypeSystem(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetDoctypeSystem", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->doctypeSystem;
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltDebug(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNodePtr inst;
    PyObject *pyobj_inst;
    xsltStylePreCompPtr comp;
    PyObject *pyobj_comp;

    if (!PyArg_ParseTuple(args, (char *)"OOOO:xsltDebug", &pyobj_ctxt, &pyobj_node, &pyobj_inst, &pyobj_comp))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    inst = (xmlNodePtr) PyxmlNode_Get(pyobj_inst);
    comp = (xsltStylePreCompPtr) PycompiledStyle_Get(pyobj_comp);

    xsltDebug(ctxt, node, inst, comp);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltStylesheetGetMethodURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltStylesheetGetMethodURI", &pyobj_style))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = style->methodURI;
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltParseStylesheetDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xsltStylesheetPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xsltParseStylesheetDoc", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xsltParseStylesheetDoc(doc);
    py_retval = libxslt_xsltStylesheetPtrWrap((xsltStylesheetPtr) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xsltKeyFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xsltKeyFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xsltKeyFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltRegisterExtPrefix(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xsltStylesheetPtr style;
    PyObject *pyobj_style;
    xmlChar * prefix;
    xmlChar * URI;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xsltRegisterExtPrefix", &pyobj_style, &prefix, &URI))
        return(NULL);
    style = (xsltStylesheetPtr) Pystylesheet_Get(pyobj_style);

    c_retval = xsltRegisterExtPrefix(style, prefix, URI);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxslt_xslDropCall(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xslDropCall();
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxslt_xsltVariableLookup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xsltTransformContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * name;
    xmlChar * ns_uri;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xsltVariableLookup", &pyobj_ctxt, &name, &ns_uri))
        return(NULL);
    ctxt = (xsltTransformContextPtr) PytransformCtxt_Get(pyobj_ctxt);

    c_retval = xsltVariableLookup(ctxt, name, ns_uri);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

