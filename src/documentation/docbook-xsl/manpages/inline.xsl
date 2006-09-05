<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                version='1.0'>

<!-- ********************************************************************
     $Id: inline.xsl,v 1.17 2006/04/05 09:03:34 xmldoc Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="replaceable|varname">
  <xsl:if test="$man.hyphenate.computer.inlines = 0">
    <xsl:call-template name="suppress.hyphenation"/>
  </xsl:if>
  <xsl:apply-templates mode="italic" select="."/>
</xsl:template>

<xsl:template match="option|userinput|envar|errorcode|constant">
  <xsl:if test="$man.hyphenate.computer.inlines = 0">
    <xsl:call-template name="suppress.hyphenation"/>
  </xsl:if>
  <xsl:apply-templates mode="bold" select="."/>
</xsl:template>

<xsl:template match="classname">
  <xsl:if test="$man.hyphenate.computer.inlines = 0">
    <xsl:call-template name="suppress.hyphenation"/>
  </xsl:if>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="command">
  <xsl:if test="$man.hyphenate.computer.inlines = 0">
    <xsl:call-template name="suppress.hyphenation"/>
  </xsl:if>
  <xsl:apply-templates mode="bold" select="."/>
</xsl:template>

<xsl:template match="type[not(ancestor::cmdsynopsis) and
                     not(ancestor::funcsynopsis)]">
  <xsl:if test="$man.hyphenate.computer.inlines = 0">
    <xsl:call-template name="suppress.hyphenation"/>
  </xsl:if>
  <xsl:apply-templates mode="bold" select="."/>
</xsl:template>

<xsl:template match="function[not(ancestor::cmdsynopsis) and
                     not(ancestor::funcsynopsis)]">
  <xsl:if test="$man.hyphenate.computer.inlines = 0">
    <xsl:call-template name="suppress.hyphenation"/>
  </xsl:if>
  <xsl:apply-templates mode="bold" select="."/>
</xsl:template>

<xsl:template match="parameter[not(ancestor::cmdsynopsis) and
                     not(ancestor::funcsynopsis)]">
  <xsl:if test="$man.hyphenate.computer.inlines = 0">
    <xsl:call-template name="suppress.hyphenation"/>
  </xsl:if>
  <xsl:apply-templates mode="italic" select="."/>
</xsl:template>

<xsl:template match="filename">
  <!-- * add hyphenation suppression in Filename output only if -->
  <!-- * break.after.slash is also non-zero -->
  <xsl:if test="$man.hyphenate.filenames = 0 and
                $man.break.after.slash = 0">
    <xsl:call-template name="suppress.hyphenation"/>
  </xsl:if>
  <xsl:apply-templates mode="italic" select="."/>
</xsl:template>

<xsl:template match="emphasis">
  <xsl:choose>
    <xsl:when test="@role = 'bold' or
                    @role = 'strong' or
                    @remap = 'B'">
      <xsl:apply-templates mode="bold" select="."/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates mode="italic" select="."/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="optional">
  <xsl:value-of select="$arg.choice.opt.open.str"/>
  <xsl:apply-templates/>
  <xsl:value-of select="$arg.choice.opt.close.str"/>
</xsl:template>

<xsl:template name="do-citerefentry">
  <xsl:param name="refentrytitle" select="''"/>
  <xsl:param name="manvolnum" select="''"/>
  <xsl:variable name="title">
    <bold><xsl:value-of select="$refentrytitle"/></bold>
  </xsl:variable>
  <xsl:apply-templates mode="bold" select="exsl:node-set($title)"/>
  <xsl:text>(</xsl:text>
  <xsl:value-of select="$manvolnum"/>
  <xsl:text>)</xsl:text>
</xsl:template>

<xsl:template match="citerefentry">
  <xsl:call-template name="do-citerefentry">
    <xsl:with-param name="refentrytitle" select="refentrytitle"/>
    <xsl:with-param name="manvolnum" select="manvolnum"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="trademark|productname">
  <xsl:apply-templates/>
  <xsl:choose>
    <!-- * Just use true Unicode chars for copyright, trademark, etc., -->
    <!-- * symbols (by default, we later automatically translate them -->
    <!-- * with the apply-string-subst-map template, or with the -->
    <!-- * default character map, if man.charmap.enabled is true). -->
    <xsl:when test="@class = 'copyright'">
      <xsl:text>&#x00a9;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'registered'">
      <xsl:text>&#x00ae;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'service'">
      <xsl:text>&#x2120;</xsl:text>
    </xsl:when>
    <xsl:when test="@class = 'trade'">
      <xsl:text>&#x2122;</xsl:text>
    </xsl:when>
    <!-- * for Trademark element, render a trademark symbol by default -->
    <!-- * even if no "class" value is specified -->
    <xsl:when test="self::trademark" >
      <xsl:text>&#x2122;</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <!-- * otherwise we have a Productname with no value for the -->
      <!-- * "class" attribute, so don't render any symbol by default -->
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- * span in seems to sneak through into output sometimes, possibly due -->
<!-- * to failed Olink processing; so we need to catch it -->
<xsl:template match="span">
  <xsl:apply-templates/>
</xsl:template>

</xsl:stylesheet>
