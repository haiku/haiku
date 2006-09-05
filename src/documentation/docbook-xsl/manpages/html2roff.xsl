<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:ng="http://docbook.org/docbook-ng"
                xmlns:db="http://docbook.org/ns/docbook"
                xmlns:exsl="http://exslt.org/common"
                exclude-result-prefixes="exsl db ng"
                version='1.0'>

<!-- ********************************************************************
     $Id: html2roff.xsl,v 1.3 2005/11/10 04:27:27 xmldoc Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- * Standalone stylesheet for doing "HTML to roff" transformation of a -->
<!-- * stylesheet; which currently just means that it transforms all -->
<!-- * <br/> instances into a line break, and all <pre></pre> instances -->
<!-- * into roff "no fill region" markup -->

<!-- ==================================================================== -->

  <xsl:output method="xml"
              encoding="UTF-8"
              indent="no"/>

  <xsl:template match="node() | @*">
    <xsl:copy>
      <xsl:apply-templates select="@* | node()"/>
    </xsl:copy>
  </xsl:template>

  <!-- ==================================================================== -->

  <xsl:template match="br">
    <xsl:element name="xsl:text">&#10;.&#10;</xsl:element>
  </xsl:template>

  <xsl:template match="pre">
    <xsl:element name="xsl:text">.sp&#10;</xsl:element>
    <xsl:element name="xsl:text">.nf&#10;</xsl:element>
    <xsl:copy>
      <xsl:apply-templates select="@* | node()"/>
    </xsl:copy>
    <xsl:element name="xsl:text">&#10;.fi&#10;</xsl:element>
  </xsl:template>

</xsl:stylesheet>
