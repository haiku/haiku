<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
                xmlns:exsl="http://exslt.org/common"
                xmlns:set="http://exslt.org/sets"
		version="1.0"
                exclude-result-prefixes="doc exsl set">

<!-- ********************************************************************
     $Id: htmlhelp.xsl,v 1.2 2004/09/20 21:00:13 kosek Exp $
     ******************************************************************** 

     This stylesheet can convert DocBook Slides document type into HTML Help.

     ******************************************************************** -->

<xsl:import href="../html/plain.xsl"/>
<xsl:include href="http://docbook.sourceforge.net/release/xsl/current/htmlhelp/htmlhelp-common.xsl"/>

<xsl:param name="keyboard.nav" select="0"/>
<xsl:param name="htmlhelp.default.topic" select="'index.html'"/>

<xsl:template match="slides" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:apply-templates select="(slidesinfo/title|title)[1]"
                       mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="slides|foilgroup" mode="hhc">
  <xsl:variable name="title">
    <xsl:if test="$htmlhelp.autolabel=1">
      <xsl:variable name="label.markup">
        <xsl:apply-templates select="." mode="label.markup"/>
      </xsl:variable>
      <xsl:if test="normalize-space($label.markup)">
        <xsl:value-of select="concat($label.markup,$autotoc.label.separator)"/>
      </xsl:if>
    </xsl:if>
    <xsl:call-template name="escape-attr">
      <xsl:with-param name="value">
        <xsl:apply-templates select="." mode="title.markup"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="$htmlhelp.hhc.show.root != 0 or parent::*">
    <xsl:text disable-output-escaping="yes">&lt;LI&gt; &lt;OBJECT type="text/sitemap"&gt;
      &lt;param name="Name" value="</xsl:text>
          <xsl:value-of select="normalize-space($title)"/>
      <xsl:text disable-output-escaping="yes">"&gt;
      &lt;param name="Local" value="</xsl:text>
          <xsl:apply-templates select="." mode="filename"/>
      <xsl:text disable-output-escaping="yes">"&gt;
    &lt;/OBJECT&gt;</xsl:text>
  </xsl:if>
  <xsl:if test="foil|foilgroup">
    <xsl:text disable-output-escaping="yes">&lt;UL&gt;</xsl:text>
      <xsl:apply-templates select="foil|foilgroup" mode="hhc"/>
    <xsl:text disable-output-escaping="yes">&lt;/UL&gt;</xsl:text>
  </xsl:if>
</xsl:template>

<xsl:template match="foil" mode="hhc">
  <xsl:variable name="title">
    <xsl:if test="$htmlhelp.autolabel=1">
      <xsl:variable name="label.markup">
        <xsl:apply-templates select="." mode="label.markup"/>
      </xsl:variable>
      <xsl:if test="normalize-space($label.markup)">
        <xsl:value-of select="concat($label.markup,$autotoc.label.separator)"/>
      </xsl:if>
    </xsl:if>
    <xsl:call-template name="escape-attr">
      <xsl:with-param name="value">
        <xsl:apply-templates select="." mode="title.markup"/>
      </xsl:with-param>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="$htmlhelp.hhc.show.root != 0 or parent::*">
    <xsl:text disable-output-escaping="yes">&lt;LI&gt; &lt;OBJECT type="text/sitemap"&gt;
      &lt;param name="Name" value="</xsl:text>
          <xsl:value-of select="normalize-space($title)"/>
      <xsl:text disable-output-escaping="yes">"&gt;
      &lt;param name="Local" value="</xsl:text>
          <xsl:apply-templates select="." mode="filename"/>
      <xsl:text disable-output-escaping="yes">"&gt;
    &lt;/OBJECT&gt;</xsl:text>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>
