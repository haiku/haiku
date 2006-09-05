<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
		xmlns:ng="http://docbook.org/docbook-ng"
		xmlns:db="http://docbook.org/ns/docbook"
                version="1.0"
                exclude-result-prefixes="doc ng db">

<xsl:import href="../html/chunk.xsl"/>

<xsl:output method="html"/>

<!-- ********************************************************************
     $Id: javahelp.xsl,v 1.13 2006/04/14 19:20:59 kosek Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->
<xsl:param name="javahelp.encoding" select="'ISO-8859-1'"/>

<doc:param name="javahelp.encoding" xmlns="">
<refpurpose>Character encoding to use in control files for Java Help.</refpurpose>
<refdescription>
<para>Java Help crashes on some characters when written as character
references. In that case you can select appropriate encoding here.</para>
</refdescription>
</doc:param>

<!-- ==================================================================== -->

<xsl:template match="/">
  <xsl:choose>
    <xsl:when test="$rootid != ''">
      <xsl:choose>
        <xsl:when test="count(key('id',$rootid)) = 0">
          <xsl:message terminate="yes">
            <xsl:text>ID '</xsl:text>
            <xsl:value-of select="$rootid"/>
            <xsl:text>' not found in document.</xsl:text>
          </xsl:message>
        </xsl:when>
        <xsl:otherwise>
          <xsl:message>Formatting from <xsl:value-of select="$rootid"/></xsl:message>
          <xsl:apply-templates select="key('id',$rootid)" mode="process.root"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="/" mode="process.root"/>
    </xsl:otherwise>
  </xsl:choose>

  <xsl:for-each select="/">    <!-- This is just a hook for building profiling stylesheets -->
    <xsl:call-template name="helpset"/>
    <xsl:call-template name="helptoc"/>
    <xsl:call-template name="helpmap"/>
    <xsl:call-template name="helpidx"/>
  </xsl:for-each>
</xsl:template>


<xsl:template name="header.navigation">
</xsl:template>

<xsl:template name="footer.navigation">
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="helpset">
  <xsl:call-template name="write.chunk.with.doctype">
    <xsl:with-param name="filename" select="concat($base.dir,'jhelpset.hs')"/>
    <xsl:with-param name="method" select="'xml'"/>
    <xsl:with-param name="indent" select="'yes'"/>
    <xsl:with-param name="doctype-public" select="'-//Sun Microsystems Inc.//DTD JavaHelp HelpSet Version 1.0//EN'"/>
    <xsl:with-param name="doctype-system" select="'http://java.sun.com/products/javahelp/helpset_1_0.dtd'"/>
    <xsl:with-param name="content">
      <xsl:call-template name="helpset.content"/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template name="helpset.content">
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <helpset version="1.0">
    <title>
      <xsl:value-of select="$title"/>
    </title>

    <!-- maps -->
    <maps>
      <homeID>top</homeID>
      <mapref location="jhelpmap.jhm"/>
    </maps>

    <!-- views -->
    <view>
      <name>TOC</name>
      <label>Table Of Contents</label>
      <type>javax.help.TOCView</type>
      <data>jhelptoc.xml</data>
    </view>

    <view>
      <name>Index</name>
      <label>Index</label>
      <type>javax.help.IndexView</type>
      <data>jhelpidx.xml</data>
    </view>

    <view>
      <name>Search</name>
      <label>Search</label>
      <type>javax.help.SearchView</type>
      <data engine="com.sun.java.help.search.DefaultSearchEngine">JavaHelpSearch</data>
    </view>
  </helpset>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="helptoc">
  <xsl:call-template name="write.chunk.with.doctype">
    <xsl:with-param name="filename" select="concat($base.dir,'jhelptoc.xml')"/>
    <xsl:with-param name="method" select="'xml'"/>
    <xsl:with-param name="indent" select="'yes'"/>
    <xsl:with-param name="doctype-public" select="'-//Sun Microsystems Inc.//DTD JavaHelp TOC Version 1.0//EN'"/>
    <xsl:with-param name="doctype-system" select="'http://java.sun.com/products/javahelp/toc_1_0.dtd'"/>
    <xsl:with-param name="encoding" select="$javahelp.encoding"/>
    <xsl:with-param name="content">
      <xsl:call-template name="helptoc.content"/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template name="helptoc.content">
  <toc version="1.0">
    <xsl:choose>
      <xsl:when test="$rootid != ''">
        <xsl:apply-templates select="key('id',$rootid)" mode="jhtoc"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="." mode="jhtoc"/>
      </xsl:otherwise>
    </xsl:choose>
  </toc>
</xsl:template>

<xsl:template match="set" mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="."/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
    <xsl:apply-templates select="book" mode="jhtoc"/>
  </tocitem>
</xsl:template>

<xsl:template match="book" mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
    <xsl:apply-templates select="part|reference|preface|chapter|appendix|article|colophon"
                         mode="jhtoc"/>
  </tocitem>
</xsl:template>

<xsl:template match="part|reference|preface|chapter|appendix|article"
              mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
    <xsl:apply-templates
      select="preface|chapter|appendix|refentry|section|sect1"
      mode="jhtoc"/>
  </tocitem>
</xsl:template>

<xsl:template match="section" mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
    <xsl:apply-templates select="section" mode="jhtoc"/>
  </tocitem>
</xsl:template>

<xsl:template match="sect1" mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
    <xsl:apply-templates select="sect2" mode="jhtoc"/>
  </tocitem>
</xsl:template>

<xsl:template match="sect2" mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
    <xsl:apply-templates select="sect3" mode="jhtoc"/>
  </tocitem>
</xsl:template>

<xsl:template match="sect3" mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
    <xsl:apply-templates select="sect4" mode="jhtoc"/>
  </tocitem>
</xsl:template>

<xsl:template match="sect4" mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
    <xsl:apply-templates select="sect5" mode="jhtoc"/>
  </tocitem>
</xsl:template>

<xsl:template match="sect5|colophon|refentry" mode="jhtoc">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="title">
    <xsl:apply-templates select="." mode="title.markup"/>
  </xsl:variable>

  <tocitem target="{$id}">
    <xsl:attribute name="text">
      <xsl:value-of select="$title"/>
    </xsl:attribute>
  </tocitem>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="helpmap">
  <xsl:call-template name="write.chunk.with.doctype">
    <xsl:with-param name="filename" select="concat($base.dir, 'jhelpmap.jhm')"/>
    <xsl:with-param name="method" select="'xml'"/>
    <xsl:with-param name="indent" select="'yes'"/>
    <xsl:with-param name="doctype-public" select="'-//Sun Microsystems Inc.//DTD JavaHelp Map Version 1.0//EN'"/>
    <xsl:with-param name="doctype-system" select="'http://java.sun.com/products/javahelp/map_1_0.dtd'"/>
    <xsl:with-param name="encoding" select="$javahelp.encoding"/>
    <xsl:with-param name="content">
      <xsl:call-template name="helpmap.content"/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template name="helpmap.content">
  <map version="1.0">
    <xsl:choose>
      <xsl:when test="$rootid != ''">
        <xsl:apply-templates select="key('id',$rootid)//set
                                     | key('id',$rootid)//book
                                     | key('id',$rootid)//part
                                     | key('id',$rootid)//reference
                                     | key('id',$rootid)//preface
                                     | key('id',$rootid)//chapter
                                     | key('id',$rootid)//appendix
                                     | key('id',$rootid)//article
                                     | key('id',$rootid)//colophon
                                     | key('id',$rootid)//refentry
                                     | key('id',$rootid)//section
                                     | key('id',$rootid)//sect1
                                     | key('id',$rootid)//sect2
                                     | key('id',$rootid)//sect3
                                     | key('id',$rootid)//sect4
                                     | key('id',$rootid)//sect5
                                     | key('id',$rootid)//indexterm
				     | key('id',$rootid)//*[@id]"
                             mode="map"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="//set
                                     | //book
                                     | //part
                                     | //reference
                                     | //preface
                                     | //chapter
                                     | //appendix
                                     | //article
                                     | //colophon
                                     | //refentry
                                     | //section
                                     | //sect1
                                     | //sect2
                                     | //sect3
                                     | //sect4
                                     | //sect5
                                     | //indexterm
				     | //*[@id]"
                             mode="map"/>
      </xsl:otherwise>
    </xsl:choose>
  </map>
</xsl:template>

<xsl:template match="set" mode="map">
  <xsl:variable name="id">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="."/>
    </xsl:call-template>
  </xsl:variable>

  <mapID target="{$id}">
    <xsl:attribute name="url">
      <xsl:call-template name="href.target.uri"/>
    </xsl:attribute>
  </mapID>
</xsl:template>

<xsl:template match="book" mode="map">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <mapID target="{$id}">
    <xsl:attribute name="url">
      <xsl:call-template name="href.target.uri"/>
    </xsl:attribute>
  </mapID>
</xsl:template>

<xsl:template match="part|reference|preface|chapter|appendix|refentry|article"
              mode="map">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <mapID target="{$id}">
    <xsl:attribute name="url">
      <xsl:call-template name="href.target.uri"/>
    </xsl:attribute>
  </mapID>
</xsl:template>

<xsl:template match="section|sect1|sect2|sect3|sect4|sect5|colophon" mode="map">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <mapID target="{$id}">
    <xsl:attribute name="url">
      <xsl:call-template name="href.target.uri"/>
    </xsl:attribute>
  </mapID>
</xsl:template>

<xsl:template match="indexterm[@class='endofrange']" mode="map"/>

<xsl:template match="indexterm" mode="map">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <mapID target="{$id}">
    <xsl:attribute name="url">
      <xsl:call-template name="href.target.uri"/>
    </xsl:attribute>
  </mapID>
</xsl:template>

<xsl:template match="*[@id]" mode="map">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <mapID target="{$id}">
    <xsl:attribute name="url">
      <xsl:call-template name="href.target.uri"/>
    </xsl:attribute>
  </mapID>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="helpidx">
  <xsl:call-template name="write.chunk.with.doctype">
    <xsl:with-param name="filename" select="concat($base.dir, 'jhelpidx.xml')"/>
    <xsl:with-param name="method" select="'xml'"/>
    <xsl:with-param name="indent" select="'yes'"/>
    <xsl:with-param name="doctype-public" select="'-//Sun Microsystems Inc.//DTD JavaHelp Index Version 1.0//EN'"/>
    <xsl:with-param name="doctype-system" select="'http://java.sun.com/products/javahelp/index_1_0.dtd'"/>
    <xsl:with-param name="encoding" select="$javahelp.encoding"/>
    <xsl:with-param name="content">
      <xsl:call-template name="helpidx.content"/>
    </xsl:with-param>
  </xsl:call-template>
</xsl:template>

<xsl:template name="helpidx.content">
  <index version="1.0">
    <xsl:choose>
      <xsl:when test="$rootid != ''">
        <xsl:apply-templates select="key('id',$rootid)//indexterm" mode="idx"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:apply-templates select="//indexterm" mode="idx"/>
      </xsl:otherwise>
    </xsl:choose>
  </index>
</xsl:template>

<xsl:template match="indexterm[@class='endofrange']" mode="idx"/>

<xsl:template match="indexterm" mode="idx">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="text">
    <xsl:value-of select="primary"/>
    <xsl:if test="secondary">
      <xsl:text>, </xsl:text>
      <xsl:value-of select="secondary"/>
    </xsl:if>
    <xsl:if test="tertiary">
      <xsl:text>, </xsl:text>
      <xsl:value-of select="tertiary"/>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="see">
      <xsl:variable name="see"><xsl:value-of select="see"/></xsl:variable>
      <indexitem text="{$text} see '{$see}'"/>
    </xsl:when>
    <xsl:otherwise>
      <indexitem text="{$text}" target="{$id}"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->
<!-- Kludge for Xalan outputting &trade; which fails in javahelp -->
<xsl:template name="dingbat.characters">
  <!-- now that I'm using the real serializer, all that dingbat malarky -->
  <!-- isn't necessary anymore... -->
  <xsl:param name="dingbat">bullet</xsl:param>

  <xsl:choose>
    <xsl:when test="$dingbat='bullet'">&#x2022;</xsl:when>
    <xsl:when test="$dingbat='copyright'">&#x00A9;</xsl:when>
    <xsl:when test="$dingbat='trademark' or $dingbat='trade'">
      <xsl:choose>
        <xsl:when test="contains(system-property('xsl:vendor'),
                                 'Apache Software Foundation')">
          <sup>TM</sup>
        </xsl:when>
        <xsl:otherwise>&#x2122;</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
    <xsl:when test="$dingbat='registered'">&#x00AE;</xsl:when>
    <xsl:when test="$dingbat='service'">(SM)</xsl:when>
    <xsl:when test="$dingbat='nbsp'">&#x00A0;</xsl:when>
    <xsl:when test="$dingbat='ldquo'">&#x201C;</xsl:when>
    <xsl:when test="$dingbat='rdquo'">&#x201D;</xsl:when>
    <xsl:when test="$dingbat='lsquo'">&#x2018;</xsl:when>
    <xsl:when test="$dingbat='rsquo'">&#x2019;</xsl:when>
    <xsl:when test="$dingbat='em-dash'">&#x2014;</xsl:when>
    <xsl:when test="$dingbat='mdash'">&#x2014;</xsl:when>
    <xsl:when test="$dingbat='en-dash'">&#x2013;</xsl:when>
    <xsl:when test="$dingbat='ndash'">&#x2013;</xsl:when>
    <xsl:otherwise>
      <xsl:text>&#x2022;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>
